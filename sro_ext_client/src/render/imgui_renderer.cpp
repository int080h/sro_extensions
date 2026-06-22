#include "render/imgui_renderer.hpp"
#include "utils/log.hpp"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

using ext_client::utils::log_msg;

namespace ext_client::render {

  namespace {
    auto apply_style() -> void {
      ImGui::StyleColorsDark();
      ImGuiStyle& style = ImGui::GetStyle();

      style.WindowPadding = ImVec2(12.0f, 12.0f);
      style.FramePadding = ImVec2(8.0f, 4.0f);
      style.ItemSpacing = ImVec2(8.0f, 6.0f);
      style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
      style.CellPadding = ImVec2(6.0f, 4.0f);
      style.IndentSpacing = 18.0f;
      style.ScrollbarSize = 14.0f;
      style.GrabMinSize = 10.0f;

      style.WindowRounding = 8.0f;
      style.ChildRounding = 6.0f;
      style.FrameRounding = 6.0f;
      style.PopupRounding = 6.0f;
      style.ScrollbarRounding = 6.0f;
      style.GrabRounding = 4.0f;
      style.TabRounding = 6.0f;

      ImVec4* colors = style.Colors;
      const ImVec4 accent{0.35f, 0.72f, 0.92f, 1.0f};
      const ImVec4 accent_dim{0.22f, 0.48f, 0.62f, 1.0f};
      const ImVec4 panel_bg{0.12f, 0.13f, 0.16f, 1.0f};
      const ImVec4 panel_bg_hover{0.16f, 0.18f, 0.22f, 1.0f};

      colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.96f);
      colors[ImGuiCol_ChildBg] = panel_bg;
      colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.12f, 0.15f, 0.98f);
      colors[ImGuiCol_Border] = ImVec4(0.24f, 0.27f, 0.32f, 0.50f);
      colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.20f, 1.0f);
      colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.26f, 1.0f);
      colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.25f, 0.30f, 1.0f);
      colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
      colors[ImGuiCol_TitleBgActive] = ImVec4(0.11f, 0.12f, 0.15f, 1.0f);
      colors[ImGuiCol_Header] = accent_dim;
      colors[ImGuiCol_HeaderHovered] = accent;
      colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.58f, 0.76f, 1.0f);
      colors[ImGuiCol_Button] = ImVec4(0.18f, 0.22f, 0.28f, 1.0f);
      colors[ImGuiCol_ButtonHovered] = accent_dim;
      colors[ImGuiCol_ButtonActive] = accent;
      colors[ImGuiCol_Tab] = panel_bg;
      colors[ImGuiCol_TabHovered] = panel_bg_hover;
      colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.22f, 0.28f, 1.0f);
      colors[ImGuiCol_TabSelected] = ImVec4(0.20f, 0.26f, 0.34f, 1.0f);
      colors[ImGuiCol_CheckMark] = accent;
      colors[ImGuiCol_SliderGrab] = accent_dim;
      colors[ImGuiCol_SliderGrabActive] = accent;
      colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.27f, 0.32f, 0.60f);
      colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.0f);
      colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.54f, 0.60f, 1.0f);
      colors[ImGuiCol_NavHighlight] = accent;
    }
  } // namespace

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

    apply_style();

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
