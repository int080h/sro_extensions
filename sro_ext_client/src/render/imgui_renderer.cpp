#include "render/imgui_renderer.hpp"

#include "menu/menu.hpp"
#include "utils/log.hpp"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

using ext_client::utils::log_msg;

namespace ext_client::render {

  imgui_renderer::~imgui_renderer() {
    shutdown();
  }

  auto imgui_renderer::initialize(HWND hwnd, IDirect3DDevice9* device) -> bool {
    if (m_initialized || !device || !hwnd) {
      return false;
    }

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad);
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr;
    io.MouseDrawCursor = false;

    ext_client::menu::theme::apply_style();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);

    m_hwnd = hwnd;
    m_initialized = true;
    return true;
  }

  auto imgui_renderer::shutdown() -> void {
    if (!m_initialized) {
      return;
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
    m_hwnd = nullptr;
  }

  auto imgui_renderer::is_initialized() const -> bool {
    return m_initialized;
  }

  auto imgui_renderer::rebind_hwnd(HWND new_hwnd) -> void {
    if (!m_initialized || !new_hwnd || new_hwnd == m_hwnd) {
      return;
    }

    const HWND old_hwnd = m_hwnd;
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplWin32_Init(new_hwnd);
    m_hwnd = new_hwnd;
    log_msg("[imgui_renderer] rebound hwnd %p -> %p", old_hwnd, new_hwnd);
  }

  auto imgui_renderer::on_device_lost() -> void {
    if (!m_initialized) {
      return;
    }
    ImGui_ImplDX9_InvalidateDeviceObjects();
  }

  auto imgui_renderer::on_device_restored() -> void {
    if (!m_initialized) {
      return;
    }
    ImGui_ImplDX9_CreateDeviceObjects();
  }

  auto imgui_renderer::begin_frame() -> void {
    if (!m_initialized) {
      return;
    }
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
  }

  auto imgui_renderer::end_frame() -> void {
    if (!m_initialized) {
      return;
    }
    ImGui::EndFrame();
    ImGui::Render();

    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data == nullptr || draw_data->TotalVtxCount <= 0) {
      return;
    }
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f) {
      return;
    }
    ImGui_ImplDX9_RenderDrawData(draw_data);
  }

  auto imgui_renderer::hwnd() const -> HWND {
    return m_hwnd;
  }

} // namespace ext_client::render
