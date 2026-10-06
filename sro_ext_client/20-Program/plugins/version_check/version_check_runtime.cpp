#include "pch.hpp"
#include "plugins/version_check/version_check_runtime.hpp"
#include "plugins/version_check/version_check_assets.hpp"

#include "core/config.hpp"
#include "core/event_bus.hpp"
#include "render/loading_splash_overlay.hpp"
#include "sdk/game/ccontroler.hpp"
#include "sdk/game/sworld.hpp"
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
  const auto cfg = ext_client::core::config::runtime();
  return cfg->version_check.banner_count > 0 ? cfg->version_check.banner_count : 0;
}

auto banner_path_for_index(int index, char* dst, std::size_t dst_size) -> bool {
  if (!dst || dst_size == 0 || index <= 0) {
    return false;
  }

  const auto cfg = ext_client::core::config::runtime();
  const char* fmt = cfg->version_check.banner_path_fmt;
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
      current->set_visible(false);
    }
    frame->set_visible(true);
    g_current_banner_index = index;
    request_loading_window_redraw();
    if (ext_client::core::config::runtime()->version_check.log_events) {
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
  rect.x = 0;
  rect.y = 0;
  rect.width = 1024;
  rect.height = 768;

  auto* frame = cif_static::create_outer_wnd(
    self, cif_static::loading_banner_res(), rect, 1, 0);
  if (!frame) {
    log_msg("[version_check_plugin] create_loading_banner_frame #%d: create_outer_wnd failed", index);
    return nullptr;
  }

  char path_buf[256]{};
  if (!banner_path_for_index(index, path_buf, sizeof(path_buf)) || !frame->set_texture_path(path_buf)) {
    log_msg("[version_check_plugin] create_loading_banner_frame #%d: set_texture_path failed (%s)", index, path_buf);
    frame->set_visible(false);
    return frame;
  }

  frame->set_position(0, 0);
  frame->set_visible(false);
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
  const auto cfg_snap = ext_client::core::config::runtime();
  log_msg("[version_check_plugin] setup_preloaded_banner_frames (self=%p original=%p count=%d cycle=%d)",
          self, original, count, cfg_snap->version_check.banner_cycle);
  if (!self || !original || !cfg_snap->version_check.banner_cycle || count <= 1) {
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
      log_msg("[version_check_plugin] frame #1 bitmap converted OK (%dx%d)", bmp->GetWidth(), bmp->GetHeight());
    } else {
      log_msg("[version_check_plugin] frame #1 bitmap conversion returned NULL");
    }
  }
  original->set_visible(false);

  for (int i = 2; i <= capped_count; ++i) {
    auto* frame = create_loading_banner_frame(self, i);
    if (!frame) {
      log_msg("[version_check_plugin] frame #%d creation failed, aborting preload loop", i);
      break;
    }
    g_banner_frames[g_banner_frame_count++] = frame;
    Gdiplus::Bitmap* bmp = convert_banner_texture_to_bitmap(frame);
    if (bmp) {
      g_overlay_bitmaps[g_overlay_bitmap_count++] = bmp;
      log_msg("[version_check_plugin] frame #%d bitmap converted OK (%dx%d)", i, bmp->GetWidth(), bmp->GetHeight());
    } else {
      log_msg("[version_check_plugin] frame #%d bitmap conversion returned NULL", i);
    }
  }
  log_msg("[version_check_plugin] preload finished: %d frames, %d bitmaps", g_banner_frame_count, g_overlay_bitmap_count);
}

auto invoke_stage_callback(intro_render_stage stage) -> int {
  const auto callback = sworld::intro_render_stage_callback();
  if (!callback) {
    return 0;
  }
  return callback(static_cast<int>(stage));
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
  const auto cfg_snap = ext_client::core::config::runtime();
  const auto& vc = cfg_snap->version_check;
  if (!vc.banner_overlay || !vc.banner_cycle || banner_count() <= 1 || !hwnd) {
    return;
  }
  RECT rect{};
  if (GetWindowRect(hwnd, &rect)) {
    ext_client::render::loading_splash_overlay::config cfg{};
    cfg.x = rect.left;
    cfg.y = rect.top;
    cfg.width = rect.right - rect.left;
    cfg.height = rect.bottom - rect.top;
    cfg.interval_ms = vc.banner_cycle_interval_ms;
    cfg.frame_count = g_overlay_bitmap_count;
    cfg.log_events = vc.log_events;
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
  ext_client::render::loading_splash_overlay::stop();

  if (g_banner_widget) {
    g_banner_widget->set_visible(false);
  }
  g_banner_widget = nullptr;
  g_last_banner_switch_time = 0;
  g_current_banner_index = -1;
  for (auto*& frame : g_banner_frames) {
    if (frame) {
      frame->set_visible(false);
    }
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

    const auto cfg_snap = ext_client::core::config::runtime();
    const auto& vc = cfg_snap->version_check;
    if (vc.log_events) {
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
    if (vc.ensure_minimize_button) {
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
  const auto cfg_snap = ext_client::core::config::runtime();
  const auto& vc = cfg_snap->version_check;
  if (!vc.banner_cycle || count <= 1 || !g_banner_widget) {
    return;
  }

  const std::uint32_t now = GetTickCount();
  if (g_last_banner_switch_time == 0) {
    g_last_banner_switch_time = now;
    return;
  }

  const std::uint32_t interval =
    vc.banner_cycle_interval_ms > 0 ? static_cast<std::uint32_t>(vc.banner_cycle_interval_ms) : 1u;
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

  auto* world = sworld::instance();
  if (!world || !sworld::is_instance()) {
    return;
  }

  world->set_render_callback(sworld::intro_render_stage_callback());

  invoke_stage_callback(intro_render_stage::d3d_setup);
  invoke_stage_callback(intro_render_stage::wire_stages);

  g_intro_render_pipeline_ready = true;
}

auto handle_version_check_create(version_check_create_context& ctx) -> void {
  auto* self = ctx.self;
  const auto cfg_snap = ext_client::core::config::runtime();
  const auto& vc = cfg_snap->version_check;
  log_msg("[version_check_plugin] handle_version_check_create entered (self=%p enabled=%d result=%d)",
          self, vc.enabled, ctx.result);
  if (!vc.enabled || !ctx.result) {
    return;
  }

  auto* banner = self->find_loading_banner_widget();
  log_msg("[version_check_plugin] find_loading_banner_widget returned %p", banner);
  g_banner_widget = banner;
  if (banner) {
    const int count = banner_count();
    setup_preloaded_banner_frames(self, banner);
    if (count > 0) {
      apply_banner_index(choose_next_banner_index(count), "initial");
      g_last_banner_switch_time = GetTickCount();
    }

    if (vc.banner_custom_size) {
      int w = vc.banner_width;
      int h = vc.banner_height;
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

        int x = vc.banner_x;
        int y = vc.banner_y;
        if (vc.banner_center) {
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

      if (self) {
        self->set_size(w, h);
        self->set_position(0, 0);
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
  const auto cfg_snap = ext_client::core::config::runtime();
  if (!cfg_snap->version_check.enabled) {
    return;
  }
  update_banner_cycle(ctx.self);
}

auto handle_set_child_process(set_child_process_context& ctx) -> void {
  const auto cfg_snap = ext_client::core::config::runtime();
  const auto& vc = cfg_snap->version_check;
  if (vc.enabled && ctx.activate) {
    if (vc.log_events) {
      log_msg("[version_check_plugin] set_child_process activated (process_type=%d), stopping loading overlay", ctx.process_type);
    }
    restore_window_style();
  }
}

auto handle_load_intro_camera(load_intro_camera_context& ctx) -> void {
  (void)ctx;
  const auto cfg_snap = ext_client::core::config::runtime();
  if (cfg_snap->version_check.log_events) {
    log_msg("[version_check_plugin] load_intro_camera reached, stopping loading overlay");
  }
  setup_login_render_pipeline();
  restore_window_style();
}

auto handle_shutdown() -> void {
  ext_client::render::loading_splash_overlay::stop();
  release_overlay_bitmaps();
}

auto handle_tick() -> void {
  const auto cfg_snap = ext_client::core::config::runtime();
  const auto& vc = cfg_snap->version_check;
  if (!vc.enabled) {
    return;
  }
  if (!is_version_check_active_process()) {
    if (ext_client::render::loading_splash_overlay::is_running() || g_wnd_style.saved) {
      if (vc.log_events) {
        log_msg("[version_check_plugin] version_check is no longer active process, stopping overlay");
      }
      restore_window_style();
    }
    if (vc.ensure_minimize_button) {
      ensure_minimize_button(resolve_loading_hwnd());
    }
  }
}
} // namespace ext_client::plugins::version_check
