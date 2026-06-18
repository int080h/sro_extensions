#include "menu/menu.hpp"

#include "config/client_config.hpp"
#include "menu/menu_shell.hpp"
#include "menu/menu_theme.hpp"
#include "menu/menu_ui.hpp"
#include "features/combat_overlay/combat_overlay.hpp"
#include "features/combat_overlay/target_window_hp.hpp"

#ifndef EXT_CLIENT_DEV_BUILD
#define EXT_CLIENT_DEV_BUILD 0
#endif

#include "menu/interface_manager.hpp"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

namespace ext_client::menu {
  namespace {

    bool g_initialized = false;
    bool g_visible = false;
    int g_capture_input_frames = 0;
    bool g_wants_capture_mouse = false;
    bool g_wants_capture_keyboard = false;

    auto render_imgui_draw_data() -> void {
      ImDrawData* draw_data = ImGui::GetDrawData();
      if (draw_data == nullptr || draw_data->TotalVtxCount <= 0) {
        return;
      }
      if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f) {
        return;
      }
      ImGui_ImplDX9_RenderDrawData(draw_data);
    }

  } // namespace

  auto on_create(HWND hwnd, IDirect3DDevice9* device) -> void {
    if (g_initialized || !device) {
      return;
    }

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad);
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr;
    io.MouseDrawCursor = false;

    theme::apply_style();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);
    g_initialized = true;
  }

  auto shutdown() -> void {
    if (!g_initialized) {
      return;
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_initialized = false;
    g_visible = false;
  }

  auto render() -> void {
    if (!g_initialized || !g_visible) {
      return;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ui::constrain_windows_to_viewport();

    if (g_capture_input_frames > 0) {
      if (g_capture_input_frames == 2) {
        ImGui::SetNextWindowFocus();
      }
      --g_capture_input_frames;
    }

    shell::draw(&g_visible);

    combat_overlay::render_imgui();

    if (::ext_client::config::data().combat_overlay.enabled && ::ext_client::config::data().combat_overlay.show_hp_percent) {
      ::ext_client::target_window_hp::render_overlay();
    }

    interface_manager::draw_overlay();

    g_wants_capture_mouse = ImGui::GetIO().WantCaptureMouse;
    g_wants_capture_keyboard = ImGui::GetIO().WantCaptureKeyboard;

    ImGui::EndFrame();
    ImGui::Render();
    render_imgui_draw_data();
  }

  auto is_initialized() -> bool {
    return g_initialized;
  }

  auto render_external(void (*draw_fn)()) -> void {
    if (!g_initialized || !draw_fn) {
      return;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ui::constrain_windows_to_viewport();

    draw_fn();

    ImGui::EndFrame();
    ImGui::Render();
    render_imgui_draw_data();
  }

  auto is_visible() -> bool {
    return g_visible;
  }

  auto set_visible(bool visible) -> void {
    if (g_visible == visible) {
      return;
    }

    g_visible = visible;
    if (!g_initialized) {
      return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.ClearEventsQueue();
    io.ClearInputKeys();
    io.ClearInputMouse();

    if (visible) {
      g_capture_input_frames = 2;
      shell::on_shown();
    } else {
      g_capture_input_frames = 0;
      g_wants_capture_mouse = false;
      g_wants_capture_keyboard = false;
    }
  }

  auto wants_capture_mouse() -> bool {
    return g_visible && g_wants_capture_mouse;
  }

  auto wants_capture_keyboard() -> bool {
    return g_visible && g_wants_capture_keyboard;
  }

} // namespace ext_client::menu
