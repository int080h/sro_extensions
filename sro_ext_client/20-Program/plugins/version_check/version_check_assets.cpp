#include "pch.hpp"
#include "plugins/version_check/version_check_assets.hpp"

#include "core/core_config.hpp"
#include "utils/log.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <Windows.h>
#include <d3d9.h>
#include <gdiplus.h>
#include <cstring>

using ext_client::utils::log_msg;

namespace ext_client::plugins::version_check {

auto ensure_gdiplus_initialized() -> void {
  if (!g_version_check_gdiplus_token) {
    Gdiplus::GdiplusStartupInput gdiplus_input{};
    if (Gdiplus::GdiplusStartup(&g_version_check_gdiplus_token, &gdiplus_input, nullptr) != Gdiplus::Ok) {
      g_version_check_gdiplus_token = 0;
    }
  }
}

auto query_d3d_texture(void* candidate) -> IDirect3DTexture9* {
  if (!candidate) {
    return nullptr;
  }

  IDirect3DTexture9* texture = nullptr;
  __try {
    auto* unknown = reinterpret_cast<IUnknown*>(candidate);
    if (SUCCEEDED(unknown->QueryInterface(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&texture)))) {
      return texture;
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    texture = nullptr;
  }
  return nullptr;
}

auto find_d3d_texture_in_resource(void* resource) -> IDirect3DTexture9* {
  if (auto* direct = query_d3d_texture(resource)) {
    return direct;
  }
  if (!resource) {
    return nullptr;
  }

  auto* words = reinterpret_cast<void**>(resource);
  for (int i = 0; i < 0x100 / static_cast<int>(sizeof(void*)); ++i) {
    void* candidate = words[i];
    if (!candidate) {
      continue;
    }
    if (auto* texture = query_d3d_texture(candidate)) {
      if (ext_client::core::config::data().version_check.log_events) {
        log_msg("[version_check_plugin] found D3D texture inside resource=%p at +0x%X -> %p", resource, i * 4, candidate);
      }
      return texture;
    }
  }
  return nullptr;
}

auto read_loading_banner_state(cif_static* banner) -> loading_banner_state {
  loading_banner_state state{};
  if (!banner) {
    return state;
  }
  state.widget_vftable = *reinterpret_cast<std::uint32_t*>(banner);
  if (auto* tb = banner->get_textboard()) {
    state.image_vftable = *reinterpret_cast<std::uint32_t*>(tb);
    state.texture = tb->get_texture();
    state.path_read = tb->copy_texture_path(state.path, sizeof(state.path));
  }
  return state;
}

auto convert_banner_texture_to_bitmap(cif_static* frame) -> Gdiplus::Bitmap* {
  if (!frame) {
    return nullptr;
  }

  ensure_gdiplus_initialized();
  if (!g_version_check_gdiplus_token) {
    return nullptr;
  }

  const auto state = read_loading_banner_state(frame);
  if (!state.texture) {
    return nullptr;
  }

  com_ptr<IDirect3DTexture9> texture(find_d3d_texture_in_resource(state.texture));
  if (!texture) {
    if (ext_client::core::config::data().version_check.log_events) {
      log_msg("[version_check_plugin] convert failed: no IDirect3DTexture9 in resource=%p path=%s", state.texture, state.path_read ? state.path : "<unreadable>");
    }
    return nullptr;
  }

  com_ptr<IDirect3DDevice9> device;
  HRESULT hr = texture->GetDevice(&device);
  if (FAILED(hr) || !device) {
    return nullptr;
  }

  com_ptr<IDirect3DSurface9> src_surface;
  hr = texture->GetSurfaceLevel(0, &src_surface);
  if (FAILED(hr) || !src_surface) {
    return nullptr;
  }

  D3DSURFACE_DESC desc{};
  src_surface->GetDesc(&desc);

  com_ptr<IDirect3DSurface9> dest_surface;
  hr = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &dest_surface, nullptr);
  if (FAILED(hr) || !dest_surface) {
    return nullptr;
  }

  using d3dx_load_surface_from_surface_fn = HRESULT(WINAPI*)(
    LPDIRECT3DSURFACE9, const PALETTEENTRY*, const RECT*, LPDIRECT3DSURFACE9, const PALETTEENTRY*, const RECT*, DWORD, D3DCOLOR);

  static d3dx_load_surface_from_surface_fn load_surface_fn = nullptr;
  static bool tried_load = false;
  if (!tried_load) {
    tried_load = true;
    const char* dlls[] = {
      "d3dx9_43.dll",
      "d3dx9_42.dll",
      "d3dx9_41.dll",
      "d3dx9_40.dll",
      "d3dx9_39.dll",
      "d3dx9_38.dll",
      "d3dx9_37.dll",
      "d3dx9_36.dll",
      "d3dx9_35.dll",
      "d3dx9_34.dll",
      "d3dx9_33.dll",
      "d3dx9_32.dll",
      "d3dx9_31.dll",
      "d3dx9_30.dll",
      "d3dx9_29.dll",
      "d3dx9_28.dll",
      "d3dx9_27.dll",
      "d3dx9_26.dll",
      "d3dx9_25.dll",
      "d3dx9_24.dll",
    };
    for (const char* dll : dlls) {
      HMODULE mod = GetModuleHandleA(dll);
      if (!mod) {
        mod = LoadLibraryA(dll);
      }
      if (!mod) {
        continue;
      }
      load_surface_fn = reinterpret_cast<d3dx_load_surface_from_surface_fn>(GetProcAddress(mod, "D3DXLoadSurfaceFromSurface"));
      if (load_surface_fn) {
        break;
      }
    }
  }

  if (!load_surface_fn) {
    return nullptr;
  }

  hr = load_surface_fn(dest_surface, nullptr, nullptr, src_surface, nullptr, nullptr, 1, 0);
  if (FAILED(hr)) {
    return nullptr;
  }

  D3DLOCKED_RECT locked_rect{};
  hr = dest_surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY);
  if (FAILED(hr)) {
    return nullptr;
  }

  Gdiplus::Bitmap* bmp = new Gdiplus::Bitmap(desc.Width, desc.Height, PixelFormat32bppARGB);
  if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) {
    Gdiplus::BitmapData bmp_data{};
    Gdiplus::Rect rect(0, 0, desc.Width, desc.Height);
    if (bmp->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bmp_data) == Gdiplus::Ok) {
      if (bmp_data.Scan0 && locked_rect.pBits) {
        auto* src = reinterpret_cast<const std::uint8_t*>(locked_rect.pBits);
        auto* dst = reinterpret_cast<std::uint8_t*>(bmp_data.Scan0);
        for (UINT y = 0; y < desc.Height; ++y) {
          std::memcpy(dst + y * bmp_data.Stride, src + y * locked_rect.Pitch, desc.Width * 4);
        }
      }
      bmp->UnlockBits(&bmp_data);
    } else {
      delete bmp;
      bmp = nullptr;
    }
  } else {
    if (bmp) {
      delete bmp;
      bmp = nullptr;
    }
  }

  dest_surface->UnlockRect();
  return bmp;
}

auto shutdown_gdiplus() -> void {
  if (g_version_check_gdiplus_token) {
    Gdiplus::GdiplusShutdown(g_version_check_gdiplus_token);
    g_version_check_gdiplus_token = 0;
  }
}

} // namespace ext_client::plugins::version_check
