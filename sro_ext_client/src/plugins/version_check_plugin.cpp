#include "plugins/version_check_plugin.hpp"

#include "core/core_plugin_manager.hpp"
#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cif_static.hpp"
#include "sdk/cps_character_select.hpp"
#include "sdk/cps_title.hpp"
#include "sdk/cps_version_check.hpp"
#include "sdk/sworld.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/imgui_helpers.hpp"
#include "utils/log.hpp"
#include "render/loading_splash_overlay.hpp"
#include "sdk/ccontroler.hpp"
#include "sdk/cgfx_video3d.hpp"
#include "sdk/cgwnd.hpp"

#pragma comment(lib, "gdiplus.lib")

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::version_check {

  // =========================================================================
  // 1. Global Variables Definition
  // =========================================================================
  std::uint32_t g_last_banner_switch_time = 0;
  int g_current_banner_index = -1;
  cif_static* g_banner_widget = nullptr;

  cif_static* g_banner_frames[k_max_banner_frames]{};
  int g_banner_frame_count = 0;
  Gdiplus::Bitmap* g_overlay_bitmaps[k_max_banner_frames]{};
  int g_overlay_bitmap_count = 0;

  ULONG_PTR g_version_check_gdiplus_token = 0;

  LONG g_orig_style = 0;
  LONG g_orig_ex_style = 0;
  bool g_style_saved = false;
  RECT g_orig_rect{};

  int g_target_width = 0;
  int g_target_height = 0;
  bool g_target_saved = false;

  bool g_intro_render_pipeline_ready = false;

  IDirect3DDevice9* g_active_d3d_device = nullptr;
  std::vector<loading_banner_state> g_loading_banners;
  int g_loading_banner_count = 0;
  intro_renderer_state* g_intro_renderer = nullptr;

  std::uint32_t g_active_d3d_device_reset_conn = 0;

  // =========================================================================
  // 2. Helper Functions Implementation
  // =========================================================================
  auto control_panel() -> ext_client::core::config::core_config::version_check_t& {
    return ext_client::core::config::data().version_check;
  }

  auto is_version_check_active_process() -> bool {
    const char* name = ccontroler::active_child_process_name();
    if (name != nullptr) {
      return std::strcmp(name, "CPSVersionCheck") == 0;
    }
    return cps_version_check::current() != nullptr;
  }

  auto find_loading_banner_widget(cps_version_check* self) -> cif_static* {
    if (!self) {
      return nullptr;
    }
    cgwnd* gwnd_self = self->as_gwnd();
    if (!gwnd_self) {
      return nullptr;
    }
    cif_static* found = nullptr;
    if (!gwnd_self->m_child_list.empty()) {
      gwnd_self->m_child_list.for_each([&](cgwnd* child) {
        if (child) {
          std::uint32_t vft = *reinterpret_cast<std::uint32_t*>(child);
          if (vft == k_cif_static_loading_banner_vftable) {
            found = reinterpret_cast<cif_static*>(child);
          }
        }
      });
    }
    return found;
  }

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

  auto ensure_minimize_button(HWND hwnd) -> void {
    if (!hwnd || !IsWindow(hwnd)) {
      return;
    }
    constexpr LONG k_minimize_style = WS_SYSMENU | WS_MINIMIZEBOX;
    const LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    const LONG wanted_style = style | k_minimize_style;
    if (style == wanted_style) {
      return;
    }
    SetWindowLongA(hwnd, GWL_STYLE, wanted_style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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
        if (control_panel().log_events) {
          log_msg("[version_check_plugin] found D3D texture inside resource=%p at +0x%X -> %p", resource, i * 4, candidate);
        }
        return texture;
      }
    }
    return nullptr;
  }

  namespace {
    auto ensure_gdiplus_initialized() -> void {
      if (!g_version_check_gdiplus_token) {
        Gdiplus::GdiplusStartupInput gdiplus_input{};
        if (Gdiplus::GdiplusStartup(&g_version_check_gdiplus_token, &gdiplus_input, nullptr) != Gdiplus::Ok) {
          g_version_check_gdiplus_token = 0;
        }
      }
    }

    auto read_loading_banner_state(cif_static* banner) -> loading_banner_state {
      loading_banner_state state{};
      if (!banner) {
        return state;
      }
      state.widget_vftable = *reinterpret_cast<std::uint32_t*>(banner);
      auto* image_sub = reinterpret_cast<std::uint8_t*>(banner) + ext_client::offsets::cif_static::fields::image_subobject;
      state.image_vftable = *reinterpret_cast<std::uint32_t*>(image_sub);
      state.texture = ext_client::offsets::field_at<void*>(image_sub, ext_client::offsets::cif_static::cif_image_renderer::fields::texture);
      state.path_read = ext_client::msvc9::string_ref::from(image_sub + ext_client::offsets::cif_static::cif_image_renderer::fields::path).copy_to(state.path, sizeof(state.path));
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
        if (control_panel().log_events) {
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

    auto banner_count() -> int {
      return control_panel().banner_count > 0 ? control_panel().banner_count : 0;
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
      if (!image_sub) {
        return false;
      }

      const auto* image_vftable = *reinterpret_cast<void***>(image_sub);
      if (!image_vftable) {
        return false;
      }

      using set_texture_fn = void(__thiscall*)(void*, ext_client::msvc9::string_pod, int, int);
      const auto set_tex_fn =
        reinterpret_cast<set_texture_fn>(image_vftable[ext_client::offsets::cif_static::cif_image_renderer::vtable::set_texture]);
      if (!set_tex_fn) {
        return false;
      }

      ext_client::msvc9::string s(path);
      ext_client::msvc9::string_pod pod{};
      std::memcpy(&pod, s.raw(), sizeof(pod));

      ext_client::msvc9::string empty_s;
      std::memcpy(s.raw(), empty_s.raw(), sizeof(pod));

      set_tex_fn(image_sub, pod, 0, 0);
      return true;
    }

    auto request_loading_window_redraw() -> void {
      HWND hwnd = resolve_loading_hwnd();
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
          log_msg("[version_check_plugin] %s preloaded loading banner #%d",
                  reason ? reason : "show",
                  index);
        }
        return true;
      }

      char path_buf[256]{};
      if (!banner_path_for_index(index, path_buf, sizeof(path_buf))) {
        return false;
      }

      if (!set_loading_banner_texture(g_banner_widget, path_buf)) {
        return false;
      }

      g_current_banner_index = index;
      request_loading_window_redraw();
      return true;
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
        self->as_gwnd(), reinterpret_cast<void*>(k_loading_banner_descriptor), rect, 1, 0);
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
    }

    namespace intro_renderer = ext_client::offsets::cps_version_check::intro_renderer;

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
      const auto fn =
        ext_client::offsets::as_fn<get_intro_renderer_fn>(ext_client::offsets::cps_version_check::functions::get_intro_renderer);
      return fn();
    }

    auto register_intro_stage_callback(intro_renderer_state* renderer, int(__cdecl* callback)(int)) -> bool {
      if (!callback || !renderer) {
        return false;
      }

      auto** vtable = *reinterpret_cast<void***>(renderer);
      const auto vtbl_index = intro_renderer::vtbl_register_stage_callback / sizeof(void*);
      if (!vtable) {
        return false;
      }

      const auto register_fn_addr = vtable[vtbl_index];
      if (!register_fn_addr) {
        return false;
      }

      using register_stage_callback_fn = void(__thiscall*)(intro_renderer_state*, int(__cdecl*)(int));
      reinterpret_cast<register_stage_callback_fn>(register_fn_addr)(renderer, callback);
      return true;
    }
  } // namespace

  // =========================================================================
  // 3. API Functions Implementation
  // =========================================================================
  auto load_ddj_as_gdi_bitmap(const char* path) -> Gdiplus::Bitmap* {
    (void)path;
    return nullptr;
  }

  auto load_banner_overlay_images() -> void {
  }
  auto free_banner_overlays() -> void {
  }
  auto load_all_banners_config() -> void {
  }
  auto free_all_banners_config() -> void {
  }
  auto update_loading_banner_texture(cps_version_check* self) -> void {
    (void)self;
  }
  auto draw_overlay_on_device(IDirect3DDevice9* device) -> void {
    (void)device;
  }
  auto apply_loading_window_styles(HWND hwnd) -> void {
    (void)hwnd;
  }
  auto restore_loading_window_styles(HWND hwnd) -> void {
    (void)hwnd;
  }

  auto start_overlay_for_window(HWND hwnd) -> void {
    if (!control_panel().banner_overlay || !control_panel().banner_cycle || banner_count() <= 1 || !hwnd) {
      return;
    }
    RECT rect{};
    if (GetWindowRect(hwnd, &rect)) {
      ext_client::render::loading_splash_overlay::config cfg{};
      cfg.x = rect.left;
      cfg.y = rect.top;
      cfg.width = rect.right - rect.left;
      cfg.height = rect.bottom - rect.top;
      cfg.interval_ms = control_panel().banner_cycle_interval_ms;
      cfg.frame_count = g_overlay_bitmap_count;
      cfg.log_events = control_panel().log_events;
      cfg.frames = reinterpret_cast<void**>(g_overlay_bitmaps);
      ext_client::render::loading_splash_overlay::start(hwnd, cfg);
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

    if (!g_style_saved) {
      return;
    }
    HWND hwnd = resolve_loading_hwnd();
    if (hwnd) {
      SetLastError(0);

      auto* app = cgfx_video3d::get();
      LONG target_style = g_orig_style;
      LONG target_ex_style = g_orig_ex_style;

      if (app && app->windowed()) {
        target_style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        target_style &= ~WS_POPUP;
      }

      LONG prev_style = SetWindowLongA(hwnd, GWL_STYLE, target_style);
      LONG prev_ex = SetWindowLongA(hwnd, GWL_EXSTYLE, target_ex_style);

      int target_w = 0;
      int target_h = 0;

      int width = 0;
      int height = 0;

      if (g_target_saved && g_target_width > 0 && g_target_height > 0) {
        width = g_target_width;
        height = g_target_height;
      } else {
        width = cgwnd::screen_width();
        height = cgwnd::screen_height();
        if (width <= 0 || height <= 0) {
          if (app) {
            width = static_cast<int>(app->creation_width());
            height = static_cast<int>(app->creation_height());
          }
        }
      }

      if (width > 0 && height > 0) {
        RECT rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
        AdjustWindowRectEx(&rect, target_style, FALSE, target_ex_style);
        target_w = rect.right - rect.left;
        target_h = rect.bottom - rect.top;
      } else {
        int orig_w = g_orig_rect.right - g_orig_rect.left;
        int orig_h = g_orig_rect.bottom - g_orig_rect.top;
        if (orig_w > 0 && orig_h > 0) {
          target_w = orig_w;
          target_h = orig_h;
        }
      }

      if (control_panel().log_events) {
        log_msg("[version_check_plugin] restore_window_style: hwnd=%p, app=%p, resolution=%dx%d, orig=%dx%d, target=%dx%d",
                hwnd,
                app,
                width,
                height,
                g_orig_rect.right - g_orig_rect.left,
                g_orig_rect.bottom - g_orig_rect.top,
                target_w,
                target_h);
      }

      if (target_w > 0 && target_h > 0) {
        int screen_w = GetSystemMetrics(SM_CXSCREEN);
        int screen_h = GetSystemMetrics(SM_CYSCREEN);
        int x = (screen_w - target_w) / 2;
        int y = (screen_h - target_h) / 2;
        if (x < 0)
          x = 0;
        if (y < 0)
          y = 0;
        SetWindowPos(hwnd, HWND_TOP, x, y, target_w, target_h, SWP_NOZORDER | SWP_FRAMECHANGED);
      } else {
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
      }
      if (control_panel().ensure_minimize_button) {
        ensure_minimize_button(hwnd);
      }

      if (app) {
        app->set_size(width, height);
      }
    }
    g_style_saved = false;
    g_target_saved = false;
    g_target_width = 0;
    g_target_height = 0;
  }

  auto update_banner_cycle(cps_version_check* self) -> void {
    const int count = banner_count();
    if (!control_panel().banner_cycle || count <= 1 || !g_banner_widget) {
      return;
    }

    const std::uint32_t now = GetTickCount();
    if (g_last_banner_switch_time == 0) {
      g_last_banner_switch_time = now;
      return;
    }

    const std::uint32_t interval =
      control_panel().banner_cycle_interval_ms > 0 ? static_cast<std::uint32_t>(control_panel().banner_cycle_interval_ms) : 1u;
    if (now - g_last_banner_switch_time >= interval) {
      g_last_banner_switch_time = now;
      apply_banner_index(choose_next_banner_index(count), "cycled");
    }
  }

  auto banner_frame_at(int idx) -> cif_static* {
    if (idx <= 0 || idx > g_banner_frame_count || idx > k_max_banner_frames) {
      return nullptr;
    }
    return g_banner_frames[idx - 1];
  }

  auto setup_login_render_pipeline() -> void {
    if (g_intro_render_pipeline_ready) {
      return;
    }

    auto* renderer = intro_renderer_instance();
    if (!renderer) {
      return;
    }

    if (!register_intro_stage_callback(renderer, intro_render_stage_callback_address())) {
      return;
    }

    invoke_stage_callback(intro_render_stage::d3d_setup);
    invoke_stage_callback(intro_render_stage::wire_stages);

    g_intro_render_pipeline_ready = true;
  }

  auto shutdown_gdiplus() -> void {
    if (g_version_check_gdiplus_token) {
      Gdiplus::GdiplusShutdown(g_version_check_gdiplus_token);
      g_version_check_gdiplus_token = 0;
    }
  }

  // =========================================================================
  // 4. Named Event Handlers Implementation
  // =========================================================================
  auto handle_version_check_create(void* raw_ctx) -> void {
    auto* ctx = static_cast<version_check_create_context*>(raw_ctx);
    auto* self = ctx->self;
    if (!control_panel().enabled || !ctx->result) {
      return;
    }

    auto* banner = find_loading_banner_widget(self);
    g_banner_widget = banner;
    if (banner) {
      const int count = banner_count();
      setup_preloaded_banner_frames(self, banner);
      if (count > 0) {
        apply_banner_index(choose_next_banner_index(count), "initial");
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
            GetWindowRect(hwnd, &g_orig_rect);
            g_style_saved = true;
          }

          style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
          style |= WS_POPUP;
          SetWindowLongA(hwnd, GWL_STYLE, style);
          ex_style &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
          SetWindowLongA(hwnd, GWL_EXSTYLE, ex_style);

          int x = control_panel().banner_x;
          int y = control_panel().banner_y;
          if (control_panel().banner_center) {
            int screen_w = GetSystemMetrics(SM_CXSCREEN);
            int screen_h = GetSystemMetrics(SM_CYSCREEN);
            x = (screen_w - w) / 2;
            y = (screen_h - h) / 2;
          }
          SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }

        if (!g_target_saved) {
          int sw = cgwnd::screen_width();
          int sh = cgwnd::screen_height();
          if (sw <= 0 || sh <= 0) {
            auto* app = cgfx_video3d::get();
            if (app) {
              sw = static_cast<int>(app->creation_width());
              sh = static_cast<int>(app->creation_height());
            }
          }
          g_target_width = sw;
          g_target_height = sh;
          g_target_saved = true;
        }

        auto* app = cgfx_video3d::get();
        if (app) {
          app->set_size(w, h);
        }

        cgwnd* gwnd_self = self->as_gwnd();
        if (gwnd_self) {
          gwnd_self->set_size(w, h);
          gwnd_self->set_position(0, 0);
        }

        for (int i = 1; i <= g_banner_frame_count; ++i) {
          if (auto* frame = banner_frame_at(i)) {
            frame->set_size(w, h);
            frame->set_position(0, 0);
          }
        }
      }
      start_overlay_for_window(resolve_loading_hwnd());
    }
  }

  auto handle_version_check_update(void* raw_ctx) -> void {
    auto* ctx = static_cast<version_check_update_context*>(raw_ctx);
    if (!control_panel().enabled) {
      return;
    }
    update_banner_cycle(ctx->self);
  }

  auto handle_set_child_process(void* raw_ctx) -> void {
    auto* ctx = static_cast<set_child_process_context*>(raw_ctx);
    if (control_panel().enabled && ctx->activate && g_style_saved) {
      restore_window_style();
    }
  }

  auto handle_load_intro_camera(void*) -> void {
    setup_login_render_pipeline();
  }

  auto handle_shutdown(void*) -> void {
    ext_client::render::loading_splash_overlay::stop();
    shutdown_gdiplus();
  }

  auto handle_menu(void* raw_ctx) -> void {
    auto* ctx = static_cast<menu_draw_context*>(raw_ctx);
    if (!ctx || !ctx->menu_visible)
      return;

    if (ImGui::BeginTabItem("Loading Screens")) {
      ext_client::utils::imgui_helpers::section_header("Loading Splash Slideshow Settings");

      auto& vc = ext_client::core::config::data().version_check;
      ext_client::utils::imgui_helpers::checkbox_dirty("Enable Splash Slideshow", &vc.enabled);
      ext_client::utils::imgui_helpers::checkbox_dirty("Banner Cycle Animation", &vc.banner_cycle);
      ext_client::utils::imgui_helpers::checkbox_dirty("Banner GDI+ Splash Overlay", &vc.banner_overlay);
      ext_client::utils::imgui_helpers::checkbox_dirty("Ensure Minimize Button", &vc.ensure_minimize_button);
      ext_client::utils::imgui_helpers::slider_int_dirty("Cycle Duration (ms)", &vc.banner_cycle_interval_ms, 100, 5000);
      ImGui::EndTabItem();
    }
  }

  auto handle_tick(void* raw_ctx) -> void {
    (void)raw_ctx;
    if (!control_panel().enabled) {
      return;
    }
    if (!is_version_check_active_process()) {
      if (g_style_saved) {
        if (control_panel().log_events) {
          log_msg("[version_check_plugin] safety check: restoring window style (saved=%d)", g_style_saved);
        }
        restore_window_style();
      }
      if (control_panel().ensure_minimize_button) {
        ensure_minimize_button(resolve_loading_hwnd());
      }
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("version_check", "Loading Slideshow", "Customizes loading banner dimensions, centering, and slideshow images.");

    ADD_PLUGIN_EVENT("version_check", event_type::on_version_check_create, handle_version_check_create);
    ADD_PLUGIN_EVENT("version_check", event_type::on_version_check_update, handle_version_check_update);
    ADD_PLUGIN_EVENT("version_check", event_type::on_set_child_process, handle_set_child_process);
    ADD_PLUGIN_EVENT("version_check", event_type::on_load_intro_camera, handle_load_intro_camera);
    ADD_PLUGIN_EVENT("version_check", event_type::on_shutdown, handle_shutdown);
    ADD_PLUGIN_EVENT("version_check", event_type::on_menu, handle_menu);
    ADD_PLUGIN_EVENT("version_check", event_type::on_tick, handle_tick);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::version_check
