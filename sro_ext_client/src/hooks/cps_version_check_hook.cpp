#include "hooks/cps_version_check_hook.hpp"

#include <Windows.h>

#include "sdk/cmsg_stream_buffer.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cif_static.hpp"
#include "sdk/cif_manager.hpp"
#include "sdk/cps_character_select.hpp"
#include "sdk/cps_title.hpp"
#include "sdk/sworld.hpp"
#include "sdk/sro_hierarchy.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/window_style.hpp"
#include "hooks/cps_title_hook.hpp"
#include "render/loading_splash_overlay.hpp"
#include "sdk/ccontroler.hpp"
#include "sdk/cgfx_video3d.hpp"

#include <d3d9.h>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks {
  namespace {

    auto control_panel() -> cps_version_check_hook::control& {
      return cps_version_check_hook::control_panel();
    }

    // Alternate vftable of CIFStatic used specifically for the loading banner widget
    constexpr std::uint32_t k_cif_static_loading_banner_vftable = ext_client::offsets::cif_static::vtable::loading_banner;

    // Resource layout descriptor ID for loading banner frames
    constexpr std::uint32_t k_loading_banner_descriptor = 0x01179A58;

    // RAII helper to manage the lifetime of COM/Direct3D interfaces
    template<typename T> class com_ptr {
      T* m_ptr = nullptr;

    public:
      com_ptr() = default;
      explicit com_ptr(T* ptr)
        : m_ptr(ptr) {}
      ~com_ptr() { reset(); }

      com_ptr(const com_ptr&) = delete;
      com_ptr& operator=(const com_ptr&) = delete;

      com_ptr(com_ptr&& other) noexcept
        : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
      }
      com_ptr& operator=(com_ptr&& other) noexcept {
        if (this != &other) {
          reset();
          m_ptr = other.m_ptr;
          other.m_ptr = nullptr;
        }
        return *this;
      }

      auto get() const -> T* { return m_ptr; }
      auto operator->() const -> T* { return m_ptr; }
      auto operator&() -> T** { return &m_ptr; }
      operator T*() const { return m_ptr; }

      auto reset(T* ptr = nullptr) -> void {
        if (m_ptr) {
          m_ptr->Release();
        }
        m_ptr = ptr;
      }

      auto release() -> T* {
        T* temp = m_ptr;
        m_ptr = nullptr;
        return temp;
      }
    };

    make_hook<convention_type::cdecl_t, int> g_create_instance;
    make_hook<convention_type::cdecl_t, int> g_connect_gateway;
    make_hook<convention_type::thiscall_t, char, cps_version_check*, int> g_on_create;
    make_hook<convention_type::thiscall_t, void, cps_version_check*> g_on_update;
    make_hook<convention_type::thiscall_t, int, cps_version_check*, cmsg_stream_buffer*> g_on_msg_recv;
    make_hook<convention_type::thiscall_t, int, void*, int, int> g_set_child_process;
    make_hook<convention_type::thiscall_t, char, void*> g_load_textdata;

    hook_group g_hooks;

    auto force_version_response_result(cmsg_stream_buffer* packet, int result) -> bool {
      if (!packet || packet->payload_size() < sizeof(std::uint32_t)) {
        return false;
      }

      auto payload = packet->extract_payload();
      if (payload.size() < sizeof(std::uint32_t)) {
        return false;
      }

      const auto value = static_cast<std::uint32_t>(result);
      payload[0] = static_cast<std::uint8_t>(value & 0xFFu);
      payload[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
      payload[2] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
      payload[3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
      return packet->replace_payload(payload.data(), payload.size());
    }

    auto __cdecl create_instance_detour() -> int {
      const int instance = g_create_instance.call_original();
      cps_version_check::set_current(reinterpret_cast<cps_version_check*>(instance));
      if (control_panel().log_events && instance) {
        log_msg("[version_check] instance=%p", reinterpret_cast<void*>(instance));
      }
      return instance;
    }

    auto __cdecl connect_gateway_detour() -> int {
      if (control_panel().bypass_gateway_connect) {
        if (control_panel().log_events) {
          log_msg("[version_check] gateway connect bypassed");
        }
        return 1;
      }
      const int result = g_connect_gateway.call_original();
      if (control_panel().log_events) {
        log_msg("[version_check] gateway connect -> %d", result);
      }
      return result;
    }

    auto is_version_check_active_process() -> bool {
      if (cps_version_check::current() == nullptr) {
        return false;
      }
      const char* name = ccontroler::active_child_process_name();
      if (name != nullptr && std::strcmp(name, "CPSVersionCheck") != 0) {
        return false;
      }
      return true;
    }

    auto find_loading_banner_widget(cps_version_check* self) -> cif_static* {
      if (!self) {
        return nullptr;
      }
      cgwnd* gwnd_self = ext_client::sdk::hierarchy::as_gwnd(self);
      if (!gwnd_self) {
        return nullptr;
      }
      void* child_list = gwnd_self->m_child_list;
      if (!child_list) {
        return nullptr;
      }
      cif_static* found = nullptr;
      ext_client::msvc9::child_list_ref::from_sentinel(child_list).for_each([&](std::uint32_t, void* value) {
        auto* child = reinterpret_cast<cgwnd*>(value);
        if (child && ext_client::msvc9::is_game_ptr(child)) {
          std::uint32_t vft = *reinterpret_cast<std::uint32_t*>(child);
          if (vft == k_cif_static_loading_banner_vftable) {
            found = reinterpret_cast<cif_static*>(child);
          }
        }
      });
      return found;
    }

    std::uint32_t g_last_banner_switch_time = 0;
    int g_current_banner_index = -1;
    cif_static* g_banner_widget = nullptr;

    enum class async_textdata_state : int {
      idle = 0,
      running = 1,
      complete = 2,
    };

    std::atomic<int> g_async_textdata_state{static_cast<int>(async_textdata_state::idle)};
    std::atomic<int> g_async_textdata_result{0};
    HANDLE g_async_textdata_thread = nullptr;
    cps_version_check* g_async_textdata_owner = nullptr;
    bool g_async_textdata_completion_logged = false;
    constexpr int k_max_banner_frames = 32;
    cif_static* g_banner_frames[k_max_banner_frames]{};
    int g_banner_frame_count = 0;
    Gdiplus::Bitmap* g_overlay_bitmaps[k_max_banner_frames]{};
    int g_overlay_bitmap_count = 0;

    struct string_pod {
      std::uint32_t words[7];
    };

    struct loading_banner_state {
      char path[256]{};
      std::uint32_t widget_vftable = 0;
      std::uint32_t image_vftable = 0;
      void* texture = nullptr;
      bool path_read = false;
    };

    auto update_banner_cycle(cps_version_check* self) -> void;
    auto banner_count() -> int;
    auto read_loading_banner_state(cif_static* banner) -> loading_banner_state;

    auto current_async_textdata_state() -> async_textdata_state {
      return static_cast<async_textdata_state>(g_async_textdata_state.load(std::memory_order_acquire));
    }

    auto async_textdata_belongs_to(cps_version_check* self) -> bool {
      return self && g_async_textdata_owner == self;
    }

    auto close_completed_async_textdata_thread() -> void {
      if (g_async_textdata_thread && current_async_textdata_state() == async_textdata_state::complete) {
        WaitForSingleObject(g_async_textdata_thread, 0);
        CloseHandle(g_async_textdata_thread);
        g_async_textdata_thread = nullptr;
      }
    }

    auto wait_for_async_textdata(cps_version_check* self, const char* reason) -> void {
      if (!async_textdata_belongs_to(self) || current_async_textdata_state() != async_textdata_state::running || !g_async_textdata_thread) {
        return;
      }

      if (control_panel().log_events) {
        log_msg("[version_check] waiting for async textdata load before %s", reason ? reason : "continue");
      }
      WaitForSingleObject(g_async_textdata_thread, INFINITE);
      close_completed_async_textdata_thread();
    }

    auto reset_async_textdata_state(bool wait_for_running) -> void {
      const auto state = current_async_textdata_state();
      if (g_async_textdata_thread) {
        if (state == async_textdata_state::running && wait_for_running) {
          WaitForSingleObject(g_async_textdata_thread, INFINITE);
        }
        if (state != async_textdata_state::running || wait_for_running) {
          CloseHandle(g_async_textdata_thread);
          g_async_textdata_thread = nullptr;
        }
      }

      if (state != async_textdata_state::running || wait_for_running) {
        g_async_textdata_owner = nullptr;
        g_async_textdata_result.store(0, std::memory_order_release);
        g_async_textdata_state.store(static_cast<int>(async_textdata_state::idle), std::memory_order_release);
        g_async_textdata_completion_logged = false;
      }
    }

    DWORD WINAPI async_textdata_thread_proc(LPVOID param) {
      int result = 0;
      __try {
        result = g_load_textdata.call_original(param) ? 1 : 0;
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        log_msg("[version_check] async CPS_LoadGameTextData crashed, exception=0x%08X", GetExceptionCode());
        result = 0;
      }

      g_async_textdata_result.store(result, std::memory_order_release);
      g_async_textdata_state.store(static_cast<int>(async_textdata_state::complete), std::memory_order_release);
      return 0;
    }

    auto start_async_textdata_load(cps_version_check* self) -> bool {
      if (!self || !g_banner_widget || control_panel().skip_textdata_load || !control_panel().banner_cycle || banner_count() <= 1) {
        return false;
      }

      int expected = static_cast<int>(async_textdata_state::idle);
      if (!g_async_textdata_state.compare_exchange_strong(expected,
                                                          static_cast<int>(async_textdata_state::running),
                                                          std::memory_order_acq_rel)) {
        return async_textdata_belongs_to(self);
      }

      auto* iface = cg_interface::get();
      if (!iface) {
        g_async_textdata_state.store(static_cast<int>(async_textdata_state::idle), std::memory_order_release);
        return false;
      }

      g_async_textdata_owner = self;
      g_async_textdata_result.store(0, std::memory_order_release);
      g_async_textdata_completion_logged = false;
      self->set_data_load_started(true);

      DWORD thread_id = 0;
      g_async_textdata_thread = CreateThread(nullptr, 0, async_textdata_thread_proc, iface, 0, &thread_id);
      if (!g_async_textdata_thread) {
        self->set_data_load_started(false);
        g_async_textdata_owner = nullptr;
        g_async_textdata_state.store(static_cast<int>(async_textdata_state::idle), std::memory_order_release);
        if (control_panel().log_events) {
          log_msg("[version_check] failed to start async textdata loader, gle=%lu", GetLastError());
        }
        return false;
      }

      if (control_panel().log_events) {
        log_msg("[version_check] async textdata loader started: self=%p, cg_interface=%p, thread_id=%lu", self, iface, thread_id);
      }
      return true;
    }



    auto note_async_textdata_complete(cps_version_check* self) -> bool {
      if (!async_textdata_belongs_to(self) || current_async_textdata_state() != async_textdata_state::complete) {
        return false;
      }

      close_completed_async_textdata_thread();
      if (control_panel().log_events && !g_async_textdata_completion_logged) {
        log_msg("[version_check] async textdata loader complete, result=%d", g_async_textdata_result.load(std::memory_order_acquire));
        g_async_textdata_completion_logged = true;
      }
      return true;
    }

    auto banner_count() -> int {
      return control_panel().banner_count > 0 ? control_panel().banner_count : 0;
    }

    auto banner_frame_at(int index) -> cif_static* {
      if (index <= 0 || index > g_banner_frame_count || index > k_max_banner_frames) {
        return nullptr;
      }
      return g_banner_frames[index - 1];
    }

    auto query_d3d_texture(void* candidate) -> IDirect3DTexture9* {
      if (!candidate || !ext_client::msvc9::is_game_ptr(candidate)) {
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
      if (!resource || !ext_client::msvc9::is_readable_ptr(resource, 0x100)) {
        return nullptr;
      }

      auto* words = reinterpret_cast<void**>(resource);
      for (int i = 0; i < 0x100 / static_cast<int>(sizeof(void*)); ++i) {
        void* candidate = words[i];
        if (!ext_client::msvc9::is_game_ptr(candidate) || !ext_client::msvc9::is_readable_ptr(candidate, sizeof(void*))) {
          continue;
        }
        if (auto* texture = query_d3d_texture(candidate)) {
          if (control_panel().log_events) {
            log_msg("[version_check] found D3D texture inside resource=%p at +0x%X -> %p", resource, i * 4, candidate);
          }
          return texture;
        }
      }
      return nullptr;
    }

    ULONG_PTR g_version_check_gdiplus_token = 0;

    auto ensure_gdiplus_initialized() -> void {
      if (!g_version_check_gdiplus_token) {
        Gdiplus::GdiplusStartupInput gdiplus_input{};
        if (Gdiplus::GdiplusStartup(&g_version_check_gdiplus_token, &gdiplus_input, nullptr) != Gdiplus::Ok) {
          g_version_check_gdiplus_token = 0;
        }
      }
    }

    auto shutdown_gdiplus() -> void {
      if (g_version_check_gdiplus_token) {
        Gdiplus::GdiplusShutdown(g_version_check_gdiplus_token);
        g_version_check_gdiplus_token = 0;
      }
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
        if (control_panel().log_events) {
          log_msg("[version_check] convert failed: no IDirect3DTexture9 in resource=%p path=%s",
                  state.texture,
                  state.path_read ? state.path : "<unreadable>");
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

      // Resolve D3DXLoadSurfaceFromSurface dynamically
      using d3dx_load_surface_from_surface_fn = HRESULT(WINAPI*)(
        LPDIRECT3DSURFACE9, const PALETTEENTRY*, const RECT*, LPDIRECT3DSURFACE9, const PALETTEENTRY*, const RECT*, DWORD, D3DCOLOR);

      static d3dx_load_surface_from_surface_fn load_surface_fn = nullptr;
      static bool tried_load = false;
      if (!tried_load) {
        tried_load = true;
        const char* dlls[] = {
          "d3dx9_43.dll", "d3dx9_42.dll", "d3dx9_41.dll", "d3dx9_40.dll", "d3dx9_39.dll", "d3dx9_38.dll", "d3dx9_37.dll",
          "d3dx9_36.dll", "d3dx9_35.dll", "d3dx9_34.dll", "d3dx9_33.dll", "d3dx9_32.dll", "d3dx9_31.dll", "d3dx9_30.dll",
          "d3dx9_29.dll", "d3dx9_28.dll", "d3dx9_27.dll", "d3dx9_26.dll", "d3dx9_25.dll", "d3dx9_24.dll",
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
            if (control_panel().log_events) {
              log_msg("[version_check] using %s!D3DXLoadSurfaceFromSurface", dll);
            }
            break;
          }
        }
      }

      if (!load_surface_fn) {
        return nullptr;
      }

      // 1 is D3DX_FILTER_NONE
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

    auto banner_path_for_index(int index, char* dst, std::size_t dst_size) -> bool {
      if (!dst || dst_size == 0 || index <= 0) {
        return false;
      }

      const char* fmt = control_panel().banner_path_fmt;
      if (!fmt || fmt[0] == '\0') {
        fmt = "interface\\loading\\start_loading_%02d.ddj";
      }

      const int written = std::snprintf(dst, dst_size, fmt, index);
      return written > 0 && static_cast<std::size_t>(written) < dst_size;
    }

    auto set_loading_banner_texture(cif_static* banner, const char* path) -> bool {
      if (!banner || !path || path[0] == '\0') {
        return false;
      }

      auto* image_sub =
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(banner) + ext_client::offsets::cif_static::fields::image_subobject);
      if (!ext_client::msvc9::is_readable_ptr(image_sub, sizeof(void*) * 4)) {
        return false;
      }

      const auto* image_vftable = *reinterpret_cast<void***>(image_sub);
      if (!ext_client::msvc9::is_readable_ptr(image_vftable, 0x38)) {
        return false;
      }

      using set_texture_fn = void(__thiscall*)(void*, string_pod, int, int);
      const auto set_tex_fn =
        reinterpret_cast<set_texture_fn>(image_vftable[ext_client::offsets::cif_static::cif_image_renderer::vtable::set_texture]);
      if (!set_tex_fn) {
        return false;
      }

      ext_client::msvc9::string s(path);
      string_pod pod{};
      std::memcpy(&pod, s.raw(), sizeof(pod));

      // MSVC9 string constructor allocates memory on the heap if the path exceeds SSO capacity (15 chars).
      // The detour function set_tex_fn takes the string parameter by-value (which is why we pass a 28-byte POD on stack).
      // The game function will call the MSVC9 string destructor on this parameter, taking ownership of and freeing the heap buffer.
      // To prevent double-free, we must clear/reset the local string object 's' before it goes out of scope and is destructed.
      ext_client::msvc9::string empty_s;
      std::memcpy(s.raw(), empty_s.raw(), sizeof(pod));

      set_tex_fn(image_sub, pod, 0, 0);
      return true;
    }

    auto read_loading_banner_state(cif_static* banner) -> loading_banner_state {
      loading_banner_state state{};
      if (!banner ||
          !ext_client::msvc9::is_readable_ptr(banner, ext_client::offsets::cif_static::fields::image_subobject + sizeof(void*))) {
        return state;
      }

      state.widget_vftable = *reinterpret_cast<std::uint32_t*>(banner);
      auto* image_sub = reinterpret_cast<std::uint8_t*>(banner) + ext_client::offsets::cif_static::fields::image_subobject;
      if (!ext_client::msvc9::is_readable_ptr(image_sub, 0x104)) {
        return state;
      }

      state.image_vftable = *reinterpret_cast<std::uint32_t*>(image_sub);
      state.texture = ext_client::offsets::field_at<void*>(image_sub, ext_client::offsets::cif_static::cif_image_renderer::fields::texture);
      state.path_read = ext_client::msvc9::string_ref::from(image_sub + ext_client::offsets::cif_static::cif_image_renderer::fields::path)
                          .copy_to(state.path, sizeof(state.path));
      return state;
    }

    auto request_loading_window_redraw() -> void {
      HWND hwnd = FindWindowA(nullptr, "SRO_CLIENT");
      if (!hwnd) {
        auto* app = cgfx_video3d::get();
        if (app) {
          hwnd = app->hwnd();
          if (!hwnd) {
            hwnd = app->hwnd_device();
          }
        }
      }
      if (hwnd) {
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
      }
    }

    auto choose_next_banner_index(int count) -> int {
      if (count <= 0) {
        return -1;
      }
      if (count == 1) {
        return 1;
      }

      int next = rand() % count + 1;
      if (next == g_current_banner_index) {
        next = (next % count) + 1;
      }
      return next;
    }

    auto apply_banner_index(int index, const char* reason) -> bool {
      if (auto* frame = banner_frame_at(index)) {
        if (auto* current = banner_frame_at(g_current_banner_index)) {
          cgwnd::set_visible(current, false);
        }
        cgwnd::set_visible(frame, true);
        g_current_banner_index = index;
        request_loading_window_redraw();
        if (control_panel().log_events) {
          const auto state = read_loading_banner_state(frame);
          log_msg("[version_check] %s preloaded loading banner #%d (widget=%p vft=0x%08X image=0x%08X texture=%p path=%s)",
                  reason ? reason : "show",
                  index,
                  frame,
                  state.widget_vftable,
                  state.image_vftable,
                  state.texture,
                  state.path_read && state.path[0] ? state.path : "<unreadable>");
        }
        return true;
      }

      char path_buf[256]{};
      if (!banner_path_for_index(index, path_buf, sizeof(path_buf))) {
        return false;
      }

      if (!set_loading_banner_texture(g_banner_widget, path_buf)) {
        if (control_panel().log_events) {
          log_msg("[version_check] failed to set loading banner: %s", path_buf);
        }
        return false;
      }

      g_current_banner_index = index;
      request_loading_window_redraw();
      if (control_panel().log_events) {
        const auto state = read_loading_banner_state(g_banner_widget);
        if (state.path_read && state.path[0] != '\0') {
          log_msg("[version_check] %s loading banner #%d: %s (widget=0x%08X image=0x%08X texture=%p path=%s)",
                  reason ? reason : "set",
                  index,
                  path_buf,
                  state.widget_vftable,
                  state.image_vftable,
                  state.texture,
                  state.path);
        } else {
          log_msg("[version_check] %s loading banner #%d: %s (widget=0x%08X image=0x%08X texture=%p path unreadable)",
                  reason ? reason : "set",
                  index,
                  path_buf,
                  state.widget_vftable,
                  state.image_vftable,
                  state.texture);
        }
      }
      return true;
    }

    auto update_banner_cycle(cps_version_check* self) -> void {
      (void)self;

      const int count = banner_count();
      if (!control_panel().banner_cycle || count <= 1 || !g_banner_widget) {
        return;
      }

      const std::uint32_t now = GetTickCount();
      if (g_last_banner_switch_time == 0) {
        g_last_banner_switch_time = now;
        if (control_panel().log_events) {
          log_msg("[version_check] cycle initialized, start time = %u", now);
        }
        return;
      }

      const std::uint32_t interval =
        control_panel().banner_cycle_interval_ms > 0 ? static_cast<std::uint32_t>(control_panel().banner_cycle_interval_ms) : 1u;
      if (now - g_last_banner_switch_time >= interval) {
        if (control_panel().log_events) {
          log_msg("[version_check] cycle trigger: now=%u, last=%u, diff=%u, interval=%u",
                  now,
                  g_last_banner_switch_time,
                  now - g_last_banner_switch_time,
                  interval);
        }
        g_last_banner_switch_time = now;

        apply_banner_index(choose_next_banner_index(count), "cycled");
      }
    }

    auto create_loading_banner_frame(cps_version_check* self, int index) -> cif_static* {
      if (!self || index <= 0) {
        return nullptr;
      }

      cgwnd_create_rect rect{};
      rect.type = 0;
      rect.x = 0;
      rect.y = 1024;
      rect.width = 768;

      auto* frame = cif_static::create_outer_wnd(
        ext_client::sdk::hierarchy::as_gwnd(self), reinterpret_cast<void*>(k_loading_banner_descriptor), rect, 1, 0);
      if (!frame) {
        return nullptr;
      }

      char path_buf[256]{};
      if (!banner_path_for_index(index, path_buf, sizeof(path_buf)) || !set_loading_banner_texture(frame, path_buf)) {
        cgwnd::set_visible(frame, false);
        return frame;
      }

      cgwnd::set_position(frame, 0, 0);
      cgwnd::set_visible(frame, false);
      if (control_panel().log_events) {
        const auto state = read_loading_banner_state(frame);
        log_msg("[version_check] preloaded loading banner #%d widget=%p vft=0x%08X image=0x%08X texture=%p path=%s",
                index,
                frame,
                state.widget_vftable,
                state.image_vftable,
                state.texture,
                state.path_read && state.path[0] ? state.path : path_buf);
      }
      return frame;
    }

    auto setup_preloaded_banner_frames(cps_version_check* self, cif_static* original) -> void {
      for (auto*& frame : g_banner_frames) {
        frame = nullptr;
      }
      g_banner_frame_count = 0;

      for (int i = 0; i < g_overlay_bitmap_count; ++i) {
        if (g_overlay_bitmaps[i]) {
          delete g_overlay_bitmaps[i];
          g_overlay_bitmaps[i] = nullptr;
        }
      }
      g_overlay_bitmap_count = 0;

      const int count = banner_count();
      if (!self || !original || !control_panel().banner_cycle || count <= 1) {
        return;
      }

      const int capped_count = count < k_max_banner_frames ? count : k_max_banner_frames;
      g_banner_frames[0] = original;
      g_banner_frame_count = 1;

      char path_buf[256]{};
      if (banner_path_for_index(1, path_buf, sizeof(path_buf))) {
        set_loading_banner_texture(original, path_buf);
        Gdiplus::Bitmap* bmp = convert_banner_texture_to_bitmap(original);
        if (bmp) {
          g_overlay_bitmaps[g_overlay_bitmap_count++] = bmp;
        }
      }
      cgwnd::set_visible(original, false);

      for (int i = 2; i <= capped_count; ++i) {
        auto* frame = create_loading_banner_frame(self, i);
        if (!frame) {
          break;
        }
        g_banner_frames[g_banner_frame_count++] = frame;
        Gdiplus::Bitmap* bmp = convert_banner_texture_to_bitmap(frame);
        if (bmp) {
          g_overlay_bitmaps[g_overlay_bitmap_count++] = bmp;
        }
      }

      if (control_panel().log_events) {
        log_msg("[version_check] preloaded %d loading banner frames from Media.pk2 (%d bitmaps converted)",
                g_banner_frame_count,
                g_overlay_bitmap_count);
      }
    }

    LONG g_orig_style = 0;
    LONG g_orig_ex_style = 0;
    bool g_style_saved = false;

    auto resolve_loading_hwnd() -> HWND {
      HWND hwnd = FindWindowA(nullptr, "SRO_CLIENT");
      if (!hwnd) {
        auto* app = cgfx_video3d::get();
        if (app) {
          hwnd = app->hwnd();
          if (!hwnd) {
            hwnd = app->hwnd_device();
          }
        }
      }
      return hwnd;
    }

    auto start_overlay_for_window(HWND hwnd) -> void {
      if (!control_panel().banner_overlay || !control_panel().banner_cycle || banner_count() <= 1 || !hwnd) {
        return;
      }

      RECT rect{};
      if (!GetWindowRect(hwnd, &rect)) {
        return;
      }

      ext_client::render::loading_splash_overlay::config cfg{};
      cfg.x = rect.left;
      cfg.y = rect.top;
      cfg.width = rect.right - rect.left;
      cfg.height = rect.bottom - rect.top;
      cfg.interval_ms = control_panel().banner_cycle_interval_ms;
      cfg.frame_count = g_overlay_bitmap_count;
      cfg.log_events = control_panel().log_events;
      cfg.frames = reinterpret_cast<void**>(g_overlay_bitmaps);

      if (!ext_client::render::loading_splash_overlay::start(hwnd, cfg) && control_panel().log_events) {
        log_msg("[version_check] failed to start loading splash overlay");
      }
    }

    auto restore_window_style() -> void {
      ext_client::render::loading_splash_overlay::stop();

      g_banner_widget = nullptr;
      g_last_banner_switch_time = 0;
      g_current_banner_index = -1;
      for (auto*& frame : g_banner_frames) {
        frame = nullptr;
      }
      g_banner_frame_count = 0;

      for (int i = 0; i < g_overlay_bitmap_count; ++i) {
        if (g_overlay_bitmaps[i]) {
          delete g_overlay_bitmaps[i];
          g_overlay_bitmaps[i] = nullptr;
        }
      }
      g_overlay_bitmap_count = 0;

      shutdown_gdiplus();

      reset_async_textdata_state(true);

      if (!g_style_saved) {
        return;
      }
      HWND hwnd = resolve_loading_hwnd();
      if (hwnd) {
        if (control_panel().log_events) {
          log_msg("[version_check] Restoring original window style: %08X, ex: %08X", g_orig_style, g_orig_ex_style);
        }
        SetLastError(0);
        LONG prev_style = SetWindowLongA(hwnd, GWL_STYLE, g_orig_style);
        LONG prev_ex = SetWindowLongA(hwnd, GWL_EXSTYLE, g_orig_ex_style);
        if ((prev_style == 0 || prev_ex == 0) && GetLastError() != 0) {
          if (control_panel().log_events) {
            log_msg("[version_check] SetWindowLongA failed during restore, gle=%lu", GetLastError());
          }
        }
        if (!SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)) {
          if (control_panel().log_events) {
            log_msg("[version_check] SetWindowPos failed during restore, gle=%lu", GetLastError());
          }
        }
        ext_client::utils::window_style::ensure_minimize_button(hwnd);
      }
      g_style_saved = false;
    }

    auto __fastcall on_create_detour(cps_version_check* self, void*, int mode) -> char {
      if (!control_panel().enabled) {
        return g_on_create.call_original(self, mode);
      }



      reset_async_textdata_state(true);
      if (control_panel().log_events) {
        log_msg("[version_check] OnCreate(self=%p mode=%d)", self, mode);
      }
      if (control_panel().skip_loading_splash) {
        self->set_data_load_started(true);
      }
      char res = g_on_create.call_original(self, mode);

      if (control_panel().log_events) {
        log_msg("[version_check] OnCreate original returned %d. Custom size: %d, cycle: %d, width: %d, height: %d",
                res,
                control_panel().banner_custom_size,
                control_panel().banner_cycle,
                control_panel().banner_width,
                control_panel().banner_height);
      }

      if (res) {
        auto* banner = find_loading_banner_widget(self);
        g_banner_widget = banner;
        if (banner) {
          const int count = banner_count();
          setup_preloaded_banner_frames(self, banner);
          if (count > 0) {
            const int idx = choose_next_banner_index(count);
            apply_banner_index(idx, control_panel().banner_cycle && count > 1 ? "initial cycle" : "initial");
            g_last_banner_switch_time = GetTickCount();
          }

          if (control_panel().banner_custom_size) {
            int w = control_panel().banner_width;
            int h = control_panel().banner_height;

            HWND hwnd = resolve_loading_hwnd();

            if (hwnd) {
              LONG style = GetWindowLongA(hwnd, GWL_STYLE);
              LONG ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);

              if (!g_style_saved) {
                g_orig_style = style;
                g_orig_ex_style = ex_style;
                g_style_saved = true;
              }

              // Recreate style as borderless popup
              style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
              style |= WS_POPUP;
              SetLastError(0);
              if (SetWindowLongA(hwnd, GWL_STYLE, style) == 0 && GetLastError() != 0) {
                if (control_panel().log_events) {
                  log_msg("[version_check] SetWindowLongA(style) failed, gle=%lu", GetLastError());
                }
              }

              ex_style &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
              if (SetWindowLongA(hwnd, GWL_EXSTYLE, ex_style) == 0 && GetLastError() != 0) {
                if (control_panel().log_events) {
                  log_msg("[version_check] SetWindowLongA(ex_style) failed, gle=%lu", GetLastError());
                }
              }

              int x = control_panel().banner_x;
              int y = control_panel().banner_y;
              if (control_panel().banner_center) {
                int screen_w = GetSystemMetrics(SM_CXSCREEN);
                int screen_h = GetSystemMetrics(SM_CYSCREEN);
                x = (screen_w - w) / 2;
                y = (screen_h - h) / 2;
              }

              if (control_panel().log_events) {
                log_msg("[version_check] Recreating window frame for HWND %p style to borderless, size=%dx%d at (%d,%d)", hwnd, w, h, x, y);
              }

              if (!SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW)) {
                if (control_panel().log_events) {
                  log_msg("[version_check] SetWindowPos failed, gle=%lu", GetLastError());
                }
              }
            }

            auto* app = cgfx_video3d::get();
            if (app) {
              if (control_panel().log_events) {
                log_msg("[version_check] Resetting D3D viewport size via cgfx_video3d::set_size to %dx%d", w, h);
              }
              app->set_size(w, h);
            }

            cgwnd* gwnd_self = ext_client::sdk::hierarchy::as_gwnd(self);
            if (gwnd_self) {
              cgwnd::set_size(gwnd_self, w, h);
              cgwnd::set_position(gwnd_self, 0, 0);
            }

            if (control_panel().log_events) {
              log_msg("[version_check] customizing loading banner size to %dx%d at (0,0)", w, h);
            }
            for (int i = 1; i <= g_banner_frame_count; ++i) {
              if (auto* frame = banner_frame_at(i)) {
                cgwnd::set_size(frame, w, h);
                cgwnd::set_position(frame, 0, 0);
              }
            }
          }

          start_overlay_for_window(resolve_loading_hwnd());
        }
      }

      return res;
    }

    auto __fastcall on_update_detour(cps_version_check* self) -> void {
      if (!control_panel().enabled) {
        g_on_update.call_original(self);
        return;
      }

      if (control_panel().log_events) {
        static std::uint32_t last_log = 0;
        std::uint32_t now = GetTickCount();
        if (now - last_log >= 1000) {
          log_msg("[version_check] on_update_detour self=%p, net_state=%d", self, self->net_state());
          last_log = now;
        }
      }

      if (control_panel().skip_textdata_load && self->net_state() == ext_client::offsets::cps_version_check::net_state::ready_for_textdata) {
        self->set_data_load_started(true);
      }

      update_banner_cycle(self);

      g_on_update.call_original(self);
    }

    auto __fastcall on_msg_recv_detour(cps_version_check* self, void*, cmsg_stream_buffer* packet) -> int {
      if (!control_panel().enabled) {
        return g_on_msg_recv.call_original(self, packet);
      }

      const auto opcode = packet->opcode();
      if (control_panel().log_events) {
        log_msg("[version_check] OnMsgRecv opcode=0x%04X", opcode);
      }

      if (control_panel().force_version_result && opcode == ext_client::offsets::cps_version_check::packets::version_check_response) {
        if (control_panel().log_events) {
          log_msg("[version_check] forcing version result=%d", control_panel().forced_version_result);
        }
        force_version_response_result(packet, control_panel().forced_version_result);
        const int rc = g_on_msg_recv.call_original(self, packet);
        if (control_panel().forced_version_result == ext_client::offsets::cps_version_check::version_result::success) {
          cps_version_check::set_version_active(false);
        }
        return rc;
      }

      return g_on_msg_recv.call_original(self, packet);
    }

    auto __fastcall set_child_process_detour(void* mgr, void*, int process_type, int activate) -> int {
      ext_client::hooks::cps_title_hook::on_set_child_process(process_type, activate);

      if (control_panel().enabled && control_panel().block_process_transition) {
        if (control_panel().log_events) {
          log_msg("[version_check] blocked SetChildProcess(type=%d activate=%d)", process_type, activate);
        }
        return 0;
      }

      if (ext_client::hooks::cps_title_hook::control_panel().enabled && ext_client::hooks::cps_title_hook::control_panel().block_process_transition) {
        if (ext_client::hooks::cps_title_hook::control_panel().log_events) {
          log_msg("[cps_title] blocked SetChildProcess(type=%d activate=%d)", process_type, activate);
        }
        return 0;
      }

      if (control_panel().enabled && activate && process_type != 1) {
        restore_window_style();
        cps_version_check::set_current(nullptr);
      }

      const int result = g_set_child_process.call_original(mgr, process_type, activate);
      void* child = ccontroler::active_child();
      if (child == nullptr && result != 0) {
        child = reinterpret_cast<void*>(result);
      }
      ccontroler::note_set_child_process(child, activate);

      const char* active_name = ccontroler::active_child_process_name();
      if (control_panel().log_events) {
        log_msg("[version_check] SetChildProcess(type=%d activate=%d) -> result=0x%08X active_child=%p active_name=%s",
                process_type,
                activate,
                result,
                ccontroler::active_child(),
                active_name ? active_name : "(null)");
      }

      return result;
    }

    auto __fastcall load_textdata_detour(void* cg_interface) -> char {
      if (control_panel().enabled && control_panel().skip_textdata_load) {
        if (control_panel().log_events) {
          log_msg("[version_check] skipped CPS_LoadGameTextData");
        }
        return 1;
      }
      return g_load_textdata.call_original(cg_interface);
    }

    // ---------------------------------------------------------------------------
    // Intro render pipeline (merged from intro_hook)
    // ---------------------------------------------------------------------------

    namespace intro_renderer = ext_client::offsets::cps_version_check::intro_renderer;

    enum class intro_render_stage : int {
      d3d_setup = 1,
      wire_stages = 2,
    };

    struct intro_renderer_state {
      void* vftable;
    };

    bool g_intro_render_pipeline_ready = false;

    make_hook<convention_type::cdecl_t, int, int, int, char> g_load_intro_camera;

    auto intro_render_stage_callback_address() -> int(__cdecl*)(int) {
      return reinterpret_cast<int(__cdecl*)(int)>(ext_client::offsets::cps_version_check::functions::intro_render_stage_callback);
    }

    auto invoke_stage_callback(intro_render_stage stage) -> int {
      const auto callback = intro_render_stage_callback_address();
      if (!callback) {
        return 0;
      }
      return callback(static_cast<int>(stage));
    }

    auto intro_renderer_instance() -> intro_renderer_state* {
      using get_intro_renderer_fn = intro_renderer_state*(__cdecl*)();
      const auto fn = ext_client::offsets::as_fn<get_intro_renderer_fn>(ext_client::offsets::cps_version_check::functions::get_intro_renderer);
      return fn();
    }

    auto register_intro_stage_callback(intro_renderer_state* renderer, int(__cdecl* callback)(int)) -> bool {
      if (!callback || !ext_client::msvc9::is_readable_ptr(renderer, sizeof(void*))) {
        return false;
      }

      auto** vtable = *reinterpret_cast<void***>(renderer);
      const auto vtbl_index = intro_renderer::vtbl_register_stage_callback / sizeof(void*);
      if (!vtable || !ext_client::msvc9::is_readable_ptr(vtable, intro_renderer::vtbl_register_stage_callback + sizeof(void*))) {
        return false;
      }

      const auto register_fn_addr = vtable[vtbl_index];
      if (!register_fn_addr || !ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(register_fn_addr), 1)) {
        return false;
      }

      using register_stage_callback_fn = void(__thiscall*)(intro_renderer_state*, int(__cdecl*)(int));
      reinterpret_cast<register_stage_callback_fn>(register_fn_addr)(renderer, callback);
      return true;
    }

    auto setup_login_render_pipeline() -> bool {
      if (g_intro_render_pipeline_ready) {
        return true;
      }

      auto* renderer = intro_renderer_instance();
      if (!renderer) {
        return false;
      }

      if (!register_intro_stage_callback(renderer, intro_render_stage_callback_address())) {
        return false;
      }

      invoke_stage_callback(intro_render_stage::d3d_setup);
      invoke_stage_callback(intro_render_stage::wire_stages);

      g_intro_render_pipeline_ready = true;
      return true;
    }

    auto __cdecl load_intro_camera_detour(int camera_state, int pack, char run_scripts) -> int {
      if (!setup_login_render_pipeline()) {
        log_msg("[version_check] intro render pipeline setup failed");
      }

      return g_load_intro_camera.call_original(camera_state, pack, run_scripts);
    }





  } // namespace

  auto cps_version_check_hook::control_panel() -> control& {
    return ext_client::config::data().version_check;
  }

  auto cps_version_check_hook::install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace ext_client::offsets::cps_version_check;
    if (!g_hooks.install(g_create_instance, functions::create_instance, &create_instance_detour, "version_check", "create_instance") ||
        !g_hooks.install(g_connect_gateway, functions::connect_gateway, &connect_gateway_detour, "version_check", "connect_gateway") ||
        !g_hooks.install(g_on_create, functions::on_create, &on_create_detour, "version_check", "on_create") ||
        !g_hooks.install(g_on_update, functions::on_update, &on_update_detour, "version_check", "on_update") ||
        !g_hooks.install(g_on_msg_recv, functions::on_msg_recv, &on_msg_recv_detour, "version_check", "on_msg_recv") ||
        !g_hooks.install(g_set_child_process, functions::set_child_process, &set_child_process_detour, "version_check", "set_child_process") ||
        !g_hooks.install(g_load_textdata, functions::load_game_textdata, &load_textdata_detour, "version_check", "load_textdata") ||
        !g_hooks.install(g_load_intro_camera, functions::load_intro_camera, &load_intro_camera_detour, "version_check", "load_intro_camera")) {
      return false;
    }

    log_msg("[version_check] hooks installed");
    return true;
  }

  auto cps_version_check_hook::uninstall() -> void {
    if (!g_hooks.is_installed()) {
      return;
    }

    reset_async_textdata_state(true);

    for (int i = 0; i < g_overlay_bitmap_count; ++i) {
      if (g_overlay_bitmaps[i]) {
        delete g_overlay_bitmaps[i];
        g_overlay_bitmaps[i] = nullptr;
      }
    }
    g_overlay_bitmap_count = 0;
    g_banner_frame_count = 0;
    g_banner_widget = nullptr;
    g_current_banner_index = -1;
    g_last_banner_switch_time = 0;

    shutdown_gdiplus();
    restore_window_style();

    g_hooks.uninstall();
    g_intro_render_pipeline_ready = false;
    cps_version_check::set_current(nullptr);
    log_msg("[version_check] hooks removed");
  }

  auto cps_version_check_hook::is_installed() -> bool {
    return g_hooks.is_installed();
  }

  auto cps_version_check_hook::tick() -> void {
    if (!control_panel().enabled) {
      return;
    }

    auto* self = cps_version_check::current();
    if (!self || !is_version_check_active_process()) {
      return;
    }

    if (control_panel().log_events) {
      static std::uint32_t last_log = 0;
      std::uint32_t now = GetTickCount();
      if (now - last_log >= 1000) {
        log_msg("[version_check] tick: cycling banner check. self=%p", self);
        last_log = now;
      }
    }

    update_banner_cycle(self);
  }

  auto cps_version_check_hook::apply_from_control() -> void {
    if (auto* self = cps_version_check::current()) {
      update_banner_cycle(self);
    }
  }

} // namespace ext_client::hooks
