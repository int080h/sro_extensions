#include "render/render_system.hpp"

#include "utils/client_config.hpp"
#include "ext_client.hpp"
#include "hooks/cif_target_window_hook.hpp"
#include "hooks/net_hook.hpp"
#include "menu/menu.hpp"
#include "menu/menu_tools.hpp"
#include "menu/debug_profiler.hpp"
#include "render/d3d_hooks.hpp"
#include "render/imgui_renderer.hpp"
#include "render/input_handler.hpp"
#include "sdk/cgfx_video3d.hpp"
#include "utils/log.hpp"
#include "utils/shutdown_guard.hpp"

#include <d3d9.h>
#include <mutex>

using ext_client::utils::log_msg;

namespace ext_client::render {

  namespace {
    imgui_renderer g_imgui;
    input_handler g_input;
    d3d_hooks g_d3d;
  } // namespace

  auto render_system::get() -> render_system& {
    static render_system instance;
    return instance;
  }

  auto render_system::install() -> bool {
    if (is_installed()) {
      return true;
    }

    device_callbacks callbacks;
    callbacks.on_end_scene = [this](IDirect3DDevice9* device) { on_end_scene(device); };
    callbacks.on_pre_reset = []() { g_imgui.on_device_lost(); };
    callbacks.on_post_reset = []() { g_imgui.on_device_restored(); };

    if (!g_d3d.install(callbacks)) {
      log_msg("[render_system] d3d hooks failed to install");
      return false;
    }

    log_msg("[render_system] installed");
    return true;
  }

  auto render_system::uninstall() -> void {
    g_input.uninstall();
    g_imgui.shutdown();
    g_d3d.uninstall();

    m_imgui_ready = false;
    log_msg("[render_system] uninstalled");
  }

  auto render_system::is_installed() const -> bool {
    return g_d3d.is_installed();
  }

  auto render_system::is_imgui_ready() const -> bool {
    return m_imgui_ready;
  }

  auto render_system::game_hwnd() const -> HWND {
    return g_input.hwnd();
  }

  auto render_system::client_mouse_pos(int& x, int& y) const -> bool {
    return g_input.client_mouse_pos(x, y);
  }

  auto render_system::apply_from_control() -> void {
  }

  auto render_system::init_imgui(IDirect3DDevice9* device) -> void {
    if (m_imgui_ready || !device) {
      return;
    }

    auto* app = cgfx_video3d::get();
    const HWND hwnd = resolve_hwnd(app);
    if (!hwnd) {
      return;
    }

    if (!g_imgui.initialize(hwnd, device)) {
      return;
    }

    g_input.install(
      hwnd,
      []() { menu::menu_controller::get().toggle(); },
      [](const char* reason) { ext_client::utils::shutdown_guard::arm(reason); }
    );
    g_input.set_imgui_ready(true);

    m_imgui_ready = true;
    log_msg("[render_system] imgui ready (device=%p hwnd=%p)", device, hwnd);
  }

  auto render_system::on_end_scene(IDirect3DDevice9* device) -> void {
    menu::debug_profiler::begin_phase(menu::debug_profiler::phase_id::init_imgui);
    std::call_once(m_init_once, [&]() { init_imgui(device); });
    menu::debug_profiler::end_phase(menu::debug_profiler::phase_id::init_imgui);

    menu::debug_profiler::begin_phase(menu::debug_profiler::phase_id::hwnd_rebind);
    if (m_imgui_ready) {
      auto* app = cgfx_video3d::get();
      const HWND hwnd = resolve_hwnd(app);
      if (hwnd && hwnd != g_input.hwnd()) {
        g_imgui.rebind_hwnd(hwnd);
        g_input.uninstall();
        g_input.install(
          hwnd,
          []() { menu::menu_controller::get().toggle(); },
          [](const char* reason) { ext_client::utils::shutdown_guard::arm(reason); }
        );
        g_input.set_imgui_ready(true);
      }
    }
    menu::debug_profiler::end_phase(menu::debug_profiler::phase_id::hwnd_rebind);

    menu::debug_profiler::begin_phase(menu::debug_profiler::phase_id::core_tick);
    if (menu::debug_profiler::is_phase_enabled(menu::debug_profiler::phase_id::core_tick)) {
      ext_client::core::get().tick();
    }
    menu::debug_profiler::end_phase(menu::debug_profiler::phase_id::core_tick);

    if (!m_imgui_ready) {
      return;
    }

    auto& menu = menu::menu_controller::get();
    const auto& target_cfg = ext_client::config::data().target_window;

    const bool need_render = menu.is_visible() ||
                             (target_cfg.enabled && target_cfg.show_hp_percent);

    menu::debug_profiler::begin_phase(menu::debug_profiler::phase_id::flush_log);
    if (menu::debug_profiler::is_phase_enabled(menu::debug_profiler::phase_id::flush_log)) {
      ext_client::hooks::net::flush_log();
    }
    menu::debug_profiler::end_phase(menu::debug_profiler::phase_id::flush_log);

    if (!need_render) {
      return;
    }

    menu::debug_profiler::begin_phase(menu::debug_profiler::phase_id::imgui_begin);
    g_imgui.begin_frame();
    menu::ui::constrain_windows_to_viewport();
    menu::debug_profiler::end_phase(menu::debug_profiler::phase_id::imgui_begin);

    menu::debug_profiler::begin_phase(menu::debug_profiler::phase_id::menu_draw);
    if (menu.is_visible()) {
      menu.draw_shell();
      if (target_cfg.enabled && target_cfg.show_hp_percent) {
        ext_client::hooks::cif_target_window_hook::render_hp_overlay();
      }
      menu::interface_manager::get().draw_overlay();
      menu.sync_capture_state();
    } else {
      if (target_cfg.enabled && target_cfg.show_hp_percent) {
        ext_client::hooks::cif_target_window_hook::render_hp_overlay();
      }
    }
    menu::debug_profiler::end_phase(menu::debug_profiler::phase_id::menu_draw);

    g_input.set_wants_capture_mouse(menu.wants_capture_mouse());
    g_input.set_wants_capture_keyboard(menu.wants_capture_keyboard());

    menu::debug_profiler::begin_phase(menu::debug_profiler::phase_id::imgui_end);
    g_imgui.end_frame();
    menu::debug_profiler::end_phase(menu::debug_profiler::phase_id::imgui_end);
  }

} // namespace ext_client::render
