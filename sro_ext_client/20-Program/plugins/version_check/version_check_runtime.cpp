#include "pch.hpp"
#include "plugins/version_check/version_check_runtime.hpp"
#include "plugins/version_check/version_check_assets.hpp"

#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "render/loading_splash_overlay.hpp"
#include "sdk/game/ccontroler.hpp"
#include "sdk/process/cps_version_check.hpp"
#include "sdk/render/cgfx_video3d.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/cif_static.hpp"
#include "utils/log.hpp"

#include <Windows.h>
#include <cstdlib>
#include <cstring>

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::version_check {

std::uint32_t g_last_banner_switch_time = 0;
int g_current_banner_index = -1;
cif_static* g_banner_widget = nullptr;

cif_static* g_banner_frames[k_max_banner_frames]{};
int g_banner_frame_count = 0;
Gdiplus::Bitmap* g_overlay_bitmaps[k_max_banner_frames]{};
int g_overlay_bitmap_count = 0;

ULONG_PTR g_version_check_gdiplus_token = 0;

window_style_state g_wnd_style;

int g_target_width = 0;
int g_target_height = 0;
bool g_target_saved = false;

bool g_intro_render_pipeline_ready = false;

namespace {

auto banner_count() -> int {
  return ext_client::core::config::data().version_check.banner_count > 0 ? ext_client::core::config::data().version_check.banner_count : 0;
}

auto banner_path_for_index(int index, char* dst, std::size_t dst_size) -> bool {
  if (!dst || dst_size == 0 || index <= 0) {
    return false;
  }

  const char* fmt = ext_client::core::config::data().version_check.banner_path_fmt;
  if (!fmt || fmt[0] == '\0') {
    fmt = "interface\\loading\\start_loading_%02d.ddj";
  }

  const int written = std::snprintf(dst, dst_size, fmt, index);
  return written > 0 && static_cast<std::size_t>(written) < dst_size;
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
    if (ext_client::core::config::data().version_check.log_events) {
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

  if (!g_banner_widget->set_texture_path(path_buf)) {
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
    self, reinterpret_cast<void*>(k_loading_banner_descriptor), rect, 1, 0);
  if (!frame) {
    return nullptr;
  }

  char path_buf[256]{};
  if (!banner_path_for_index(index, path_buf, sizeof(path_buf)) || !frame->set_texture_path(path_buf)) {
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
  if (!self || !original || !ext_client::core::config::data().version_check.banner_cycle || count <= 1) {
    return;
  }

  const int capped_count = count < k_max_banner_frames ? count : k_max_banner_frames;
  g_banner_frames[0] = original;
  g_banner_frame_count = 1;

  char path_buf[256]{};
  if (banner_path_for_index(1, path_buf, sizeof(path_buf))) {
    original->set_texture_path(path_buf);
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

auto intro_render_stage_callback_address() -> int(__cdecl*)(int) {
  return reinterpret_cast<int(__cdecl*)(int)>(0x0094D050);
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
    ext_client::off::as_fn<get_intro_renderer_fn>(0x00B523F0);
  return fn();
}

auto register_intro_stage_callback(intro_renderer_state* renderer, int(__cdecl* callback)(int)) -> bool {
  if (!callback || !renderer) {
    return false;
  }

  auto** vtable = *reinterpret_cast<void***>(renderer);
  const auto vtbl_index = 0xA8 / sizeof(void*);
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

auto is_version_check_active_process() -> bool {
  const char* name = ccontroler::active_child_process_name();
  if (name != nullptr) {
    return std::strcmp(name, "CPSVersionCheck") == 0;
  }
  return cps_version_check::current() != nullptr;
}

auto resolve_loading_hwnd() -> HWND {
  HWND hwnd = FindWindowA(nullptr, "SRO_CLIENT");
  if (!hwnd) {
    auto* app = cgfx_video3d::get();
    if (app) {
      hwnd = app->get_hwnd();
      if (!hwnd) {
        hwnd = app->get_hwnd_device();
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

auto start_overlay_for_window(HWND hwnd) -> void {
  if (!ext_client::core::config::data().version_check.banner_overlay || !ext_client::core::config::data().version_check.banner_cycle || banner_count() <= 1 || !hwnd) {
    return;
  }
  RECT rect{};
  if (GetWindowRect(hwnd, &rect)) {
    ext_client::render::loading_splash_overlay::config cfg{};
    cfg.x = rect.left;
    cfg.y = rect.top;
    cfg.width = rect.right - rect.left;
    cfg.height = rect.bottom - rect.top;
    cfg.interval_ms = ext_client::core::config::data().version_check.banner_cycle_interval_ms;
    cfg.frame_count = g_overlay_bitmap_count;
    cfg.log_events = ext_client::core::config::data().version_check.log_events;
    cfg.frames = reinterpret_cast<void**>(g_overlay_bitmaps);
    ext_client::render::loading_splash_overlay::start(hwnd, cfg);
  }
}

auto release_overlay_bitmaps() -> void {
  for (int i = 0; i < g_overlay_bitmap_count; ++i) {
    if (g_overlay_bitmaps[i]) {
      delete g_overlay_bitmaps[i];
      g_overlay_bitmaps[i] = nullptr;
    }
  }
  g_overlay_bitmap_count = 0;

  shutdown_gdiplus();
}

auto restore_window_style() -> void {
  if (!ext_client::render::loading_splash_overlay::stop()) return;

  g_banner_widget = nullptr;
  g_last_banner_switch_time = 0;
  g_current_banner_index = -1;
  for (auto*& frame : g_banner_frames) {
    frame = nullptr;
  }
  g_banner_frame_count = 0;
  release_overlay_bitmaps();

  if (!g_wnd_style.saved) {
    return;
  }
  HWND hwnd = resolve_loading_hwnd();
  if (hwnd) {
    SetLastError(0);

    auto* app = cgfx_video3d::get();
    LONG target_style = g_wnd_style.orig_style;
    LONG target_ex_style = g_wnd_style.orig_ex_style;

    if (app && app->is_windowed()) {
      target_style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
      target_style &= ~WS_POPUP;
    }

    SetWindowLongA(hwnd, GWL_STYLE, target_style);
    SetWindowLongA(hwnd, GWL_EXSTYLE, target_ex_style);

    int target_w = 0;
    int target_h = 0;

    int width = 0;
    int height = 0;

    if (g_target_saved && g_target_width > 0 && g_target_height > 0) {
      width = g_target_width;
      height = g_target_height;
    } else {
      width = cgwnd::get_screen_width();
      height = cgwnd::get_screen_height();
      if (width <= 0 || height <= 0) {
        if (app) {
          width = static_cast<int>(app->get_creation_width());
          height = static_cast<int>(app->get_creation_height());
        }
      }
    }

    if (width > 0 && height > 0) {
      RECT rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
      AdjustWindowRectEx(&rect, target_style, FALSE, target_ex_style);
      target_w = rect.right - rect.left;
      target_h = rect.bottom - rect.top;
    } else {
      int orig_w = g_wnd_style.orig_rect.right - g_wnd_style.orig_rect.left;
      int orig_h = g_wnd_style.orig_rect.bottom - g_wnd_style.orig_rect.top;
      if (orig_w > 0 && orig_h > 0) {
        target_w = orig_w;
        target_h = orig_h;
      }
    }

    if (ext_client::core::config::data().version_check.log_events) {
      log_msg("[version_check_plugin] restore_window_style: hwnd=%p, app=%p, resolution=%dx%d, orig=%dx%d, target=%dx%d",
              hwnd,
              app,
              width,
              height,
              g_wnd_style.orig_rect.right - g_wnd_style.orig_rect.left,
              g_wnd_style.orig_rect.bottom - g_wnd_style.orig_rect.top,
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
    if (ext_client::core::config::data().version_check.ensure_minimize_button) {
      ensure_minimize_button(hwnd);
    }

    if (app) {
      app->set_size(width, height);
    }
  }
  g_wnd_style.saved = false;
  g_target_saved = false;
  g_target_width = 0;
  g_target_height = 0;
}

auto update_banner_cycle(cps_version_check* self) -> void {
  (void)self;
  const int count = banner_count();
  if (!ext_client::core::config::data().version_check.banner_cycle || count <= 1 || !g_banner_widget) {
    return;
  }

  const std::uint32_t now = GetTickCount();
  if (g_last_banner_switch_time == 0) {
    g_last_banner_switch_time = now;
    return;
  }

  const std::uint32_t interval =
    ext_client::core::config::data().version_check.banner_cycle_interval_ms > 0 ? static_cast<std::uint32_t>(ext_client::core::config::data().version_check.banner_cycle_interval_ms) : 1u;
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

auto handle_version_check_create(version_check_create_context& ctx) -> void {
  auto* self = ctx.self;
  if (!ext_client::core::config::data().version_check.enabled || !ctx.result) {
    return;
  }

  auto* banner = self->find_loading_banner_widget();
  g_banner_widget = banner;
  if (banner) {
    const int count = banner_count();
    setup_preloaded_banner_frames(self, banner);
    if (count > 0) {
      apply_banner_index(choose_next_banner_index(count), "initial");
      g_last_banner_switch_time = GetTickCount();
    }

    if (ext_client::core::config::data().version_check.banner_custom_size) {
      int w = ext_client::core::config::data().version_check.banner_width;
      int h = ext_client::core::config::data().version_check.banner_height;
      HWND hwnd = resolve_loading_hwnd();

      if (hwnd) {
        LONG style = GetWindowLongA(hwnd, GWL_STYLE);
        LONG ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
        if (!g_wnd_style.saved) {
          g_wnd_style.orig_style = style;
          g_wnd_style.orig_ex_style = ex_style;
          GetWindowRect(hwnd, &g_wnd_style.orig_rect);
          g_wnd_style.saved = true;
        }

        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        style |= WS_POPUP;
        SetWindowLongA(hwnd, GWL_STYLE, style);
        ex_style &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
        SetWindowLongA(hwnd, GWL_EXSTYLE, ex_style);

        int x = ext_client::core::config::data().version_check.banner_x;
        int y = ext_client::core::config::data().version_check.banner_y;
        if (ext_client::core::config::data().version_check.banner_center) {
          int screen_w = GetSystemMetrics(SM_CXSCREEN);
          int screen_h = GetSystemMetrics(SM_CYSCREEN);
          x = (screen_w - w) / 2;
          y = (screen_h - h) / 2;
        }
        SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
      }

      if (!g_target_saved) {
        int sw = cgwnd::get_screen_width();
        int sh = cgwnd::get_screen_height();
        if (sw <= 0 || sh <= 0) {
          auto* app = cgfx_video3d::get();
          if (app) {
            sw = static_cast<int>(app->get_creation_width());
            sh = static_cast<int>(app->get_creation_height());
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

      auto* root = reinterpret_cast<cgwnd*>(self);
      if (root) {
        root->set_size(w, h);
        root->set_position(0, 0);
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

auto handle_version_check_update(version_check_update_context& ctx) -> void {
  if (!ext_client::core::config::data().version_check.enabled) {
    return;
  }
  update_banner_cycle(ctx.self);
}

auto handle_set_child_process(set_child_process_context& ctx) -> void {
  if (ext_client::core::config::data().version_check.enabled && ctx.activate && g_wnd_style.saved) {
    restore_window_style();
  }
}

auto handle_load_intro_camera(load_intro_camera_context& ctx) -> void {
  (void)ctx;
  setup_login_render_pipeline();
}

auto handle_shutdown() -> void {
  if (!ext_client::render::loading_splash_overlay::stop()) return;
  release_overlay_bitmaps();
}

auto handle_tick() -> void {
  if (!ext_client::core::config::data().version_check.enabled) {
    return;
  }
  if (!is_version_check_active_process()) {
    if (g_wnd_style.saved) {
      if (ext_client::core::config::data().version_check.log_events) {
        log_msg("[version_check_plugin] safety check: restoring window style (saved=%d)", g_wnd_style.saved);
      }
      restore_window_style();
    }
    if (ext_client::core::config::data().version_check.ensure_minimize_button) {
      ensure_minimize_button(resolve_loading_hwnd());
    }
  }
}

} // namespace ext_client::plugins::version_check
