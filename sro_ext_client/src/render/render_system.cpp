#include "render/render_system.hpp"
#include <imgui.h>

#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "core/core_hooks.hpp"
#include "render/imgui_renderer.hpp"
#include "render/input_handler.hpp"
#include "sdk/cgfx_video3d.hpp"
#include "utils/log.hpp"
#include "utils/process.hpp"

#include <d3d9.h>
#include <mutex>

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::render {

  namespace {
    imgui_renderer g_imgui;
    input_handler g_input;
    bool g_installed = false;

    auto resolve_hwnd(cgfx_video3d* app) -> HWND {
      if (!app) {
        return nullptr;
      }
      if (HWND hwnd = app->hwnd()) {
        return hwnd;
      }
      if (HWND hwnd = app->hwnd_device()) {
        return hwnd;
      }
      return FindWindowA(nullptr, "SRO_CLIENT");
    }
    auto install_input(HWND hwnd) -> void {
      g_input.install(
        hwnd,
        []() { event_manager::get().dispatch(event_type::on_menu_toggle, nullptr); },
        [](const char* reason) { ext_client::utils::process::shutdown_guard::arm(reason); });
      g_input.set_imgui_ready(true);
    }
  } // namespace

  auto render_system::get() -> render_system& {
    static render_system instance;
    return instance;
  }

  auto render_system::install() -> bool {
    g_installed = true;
    log_msg("[render_system] installed via event bindings");
    return true;
  }

  auto render_system::uninstall() -> void {
    g_input.uninstall();
    g_imgui.shutdown();

    m_imgui_ready = false;
    g_installed = false;
    log_msg("[render_system] uninstalled");
  }

  auto render_system::is_installed() const -> bool {
    return g_installed;
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

    install_input(hwnd);

    m_imgui_ready = true;
    log_msg("[render_system] imgui ready (device=%p hwnd=%p)", device, hwnd);
  }

  auto render_system::toggle_menu() -> void {
    set_menu_visible(!m_menu_visible);
  }

  auto render_system::set_menu_visible(bool visible) -> void {
    if (m_menu_visible == visible) {
      return;
    }
    m_menu_visible = visible;

    ImGuiIO& io = ImGui::GetIO();
    io.ClearEventsQueue();
    io.ClearInputKeys();
    io.ClearInputMouse();

    if (visible) {
      m_capture_input_frames = 2;
    } else {
      m_capture_input_frames = 0;
    }
  }

  auto render_system::on_end_scene(IDirect3DDevice9* device) -> void {
    // Tick the core hooks every frame on the render thread
    std::call_once(m_init_once, [&]() { init_imgui(device); });

    if (m_imgui_ready) {
      auto* app = cgfx_video3d::get();
      const HWND hwnd = resolve_hwnd(app);
      if (hwnd && hwnd != g_input.hwnd()) {
        g_imgui.rebind_hwnd(hwnd);
        g_input.uninstall();
        install_input(hwnd);
      }
    }

    if (!m_imgui_ready) {
      return;
    }

    g_imgui.begin_frame();

    if (m_capture_input_frames > 0) {
      if (m_capture_input_frames == 2) {
        ImGui::SetNextWindowFocus();
      }
      --m_capture_input_frames;
    }
    menu_draw_context ctx{};
    ctx.menu_visible = m_menu_visible;

    if (m_menu_visible) {
      ImGui::SetNextWindowSize(ImVec2(800.0f, 500.0f), ImGuiCond_FirstUseEver);

      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.12f, 0.94f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.72f, 0.92f, 0.40f));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

      bool open = m_menu_visible;
      if (ImGui::Begin("Antigravity Client Panel", &open, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("AntigravityTabs")) {
          event_manager::get().dispatch(event_type::on_menu, &ctx);
          ImGui::EndTabBar();
        }
      }
      ImGui::End();

      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor(2);

      if (!open) {
        set_menu_visible(false);
        ctx.menu_visible = false;
      }
    } else {
      event_manager::get().dispatch(event_type::on_menu, &ctx);
    }

    const ImGuiIO& io = ImGui::GetIO();
    ctx.wants_capture_mouse = m_menu_visible && io.WantCaptureMouse;
    ctx.wants_capture_keyboard = m_menu_visible && io.WantCaptureKeyboard;
    g_input.set_wants_capture_mouse(ctx.wants_capture_mouse);
    g_input.set_wants_capture_keyboard(ctx.wants_capture_keyboard);
    g_imgui.end_frame();

    ext_client::core::hooks::core_hooks::tick();
  }

  // =========================================================================
  // D3D Event Subscriptions
  // =========================================================================

  ADD_EVENT(event_type::on_d3d_device_created, [](void* raw_ctx) {
    auto* ctx = static_cast<d3d_device_created_context*>(raw_ctx);
    if (ctx) {
      render_system::get().init_imgui(ctx->device);
    }
  });

  ADD_EVENT(event_type::on_d3d_end_scene, [](void* raw_ctx) {
    auto* ctx = static_cast<d3d_end_scene_context*>(raw_ctx);
    if (ctx) {
      render_system::get().on_end_scene(ctx->device);
    }
  });

  ADD_EVENT(event_type::on_d3d_pre_reset, [](void*) {
    g_imgui.on_device_lost();
  });

  ADD_EVENT(event_type::on_d3d_post_reset, [](void*) {
    g_imgui.on_device_restored();
  });

  ADD_EVENT(event_type::on_menu_toggle, [](void*) {
    render_system::get().toggle_menu();
  });

} // namespace ext_client::render
