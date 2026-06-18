#include "menu/menu_theme.hpp"

#include <imgui.h>

namespace ext_client::menu::theme {

  auto apply_style() -> void {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 5.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;

    ImVec4* colors = style.Colors;
    const ImVec4 accent{0.35f, 0.72f, 0.92f, 1.0f};
    const ImVec4 accent_dim{0.22f, 0.48f, 0.62f, 1.0f};
    const ImVec4 panel_bg{0.12f, 0.13f, 0.16f, 1.0f};
    const ImVec4 panel_bg_hover{0.16f, 0.18f, 0.22f, 1.0f};

    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.13f, 0.98f);
    colors[ImGuiCol_ChildBg] = panel_bg;
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.12f, 0.15f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.28f, 0.32f, 0.38f, 0.55f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.26f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.13f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.14f, 0.18f, 1.0f);
    colors[ImGuiCol_Header] = accent_dim;
    colors[ImGuiCol_HeaderHovered] = accent;
    colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.58f, 0.76f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.24f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = accent_dim;
    colors[ImGuiCol_ButtonActive] = accent;
    colors[ImGuiCol_Tab] = panel_bg;
    colors[ImGuiCol_TabHovered] = panel_bg_hover;
    colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.22f, 0.28f, 1.0f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.20f, 0.26f, 0.34f, 1.0f);
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent_dim;
    colors[ImGuiCol_SliderGrabActive] = accent;
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.32f, 0.38f, 0.65f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.12f, 0.15f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.54f, 0.60f, 1.0f);
    colors[ImGuiCol_NavHighlight] = accent;
  }

} // namespace ext_client::menu::theme
