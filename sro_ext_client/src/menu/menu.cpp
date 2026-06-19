#include "ext_client.hpp"
#include "menu/menu.hpp"
#include "menu/menu_tools.hpp"
#include "menu/settings_panels.hpp"
#include "menu/debug_profiler.hpp"

#include "utils/client_config.hpp"
#include "utils/vectorf.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>

#ifndef EXT_CLIENT_DEV_BUILD
#define EXT_CLIENT_DEV_BUILD 0
#endif

// ---------------------------------------------------------------------------
// menu_theme
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// menu_ui helpers
// ---------------------------------------------------------------------------

namespace ext_client::menu::ui {

  auto checkbox_column(bool* left, const char* left_label, bool* right, const char* right_label) -> bool {
    bool changed = false;
    if (ImGui::BeginTable("##checkbox_row", 2, ImGuiTableFlags_SizingStretchSame)) {
      ImGui::TableNextColumn();
      if (ImGui::Checkbox(left_label, left)) {
        changed = true;
      }
      ImGui::TableNextColumn();
      if (ImGui::Checkbox(right_label, right)) {
        changed = true;
      }
      ImGui::EndTable();
    }
    return changed;
  }

  auto setting_changed(bool sync_runtime) -> void {
    if (sync_runtime) {
      ext_client::config::sync_to_runtime();
    }
    ext_client::config::mark_dirty();
  }

  auto checkbox_setting(const char* label, bool* value, bool sync_runtime) -> bool {
    if (!ImGui::Checkbox(label, value)) {
      return false;
    }
    setting_changed(sync_runtime);
    return true;
  }

  auto section_header(const char* title, bool default_open) -> bool {
    const ImGuiTreeNodeFlags flags = default_open ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None;
    return ImGui::CollapsingHeader(title, flags);
  }

  auto live_state_header(const char* title) -> bool {
    return ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_None);
  }

  auto section_card_begin(const char* id) -> bool {
    return ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border);
  }

  auto section_card_end() -> void {
    ImGui::EndChild();
  }

  auto set_full_width() -> void {
    ImGui::SetNextItemWidth(-1.0f);
  }

  auto offset_input2(const char* label, vector2f& value) -> bool {
    int values[2] = {static_cast<int>(value.x), static_cast<int>(value.y)};
    set_full_width();
    if (!ImGui::InputInt2(label, values)) {
      return false;
    }
    value.x = static_cast<float>(values[0]);
    value.y = static_cast<float>(values[1]);
    return true;
  }

  auto sro_color_edit(const char* label, std::uint32_t& sro_color) -> bool {
    ImVec4 color = ImGui::ColorConvertU32ToFloat4(ext_client::utils::color::to_imgui(sro_color));
    set_full_width();
    if (!ImGui::ColorEdit4(label, &color.x, ImGuiColorEditFlags_NoInputs)) {
      return false;
    }
    sro_color = ext_client::utils::color::from_imgui(ImGui::ColorConvertFloat4ToU32(color));
    return true;
  }

  auto apply_button(const char* label) -> bool {
    if (!ImGui::Button(label)) {
      return false;
    }
    setting_changed();
    return true;
  }

  auto int_input_setting(const char* label, int* value, bool sync_runtime) -> bool {
    set_full_width();
    if (!ImGui::InputInt(label, value)) {
      return false;
    }
    if (sync_runtime) {
      setting_changed();
    } else {
      ext_client::config::mark_dirty();
    }
    return true;
  }

  auto int_slider_setting(const char* label, int* value, int min_v, int max_v, const char* fmt, bool sync_runtime) -> bool {
    set_full_width();
    if (!ImGui::SliderInt(label, value, min_v, max_v, fmt)) {
      return false;
    }
    if (sync_runtime) {
      setting_changed();
    } else {
      ext_client::config::mark_dirty();
    }
    return true;
  }

  auto text_input_setting(const char* label, char* buffer, std::size_t buffer_size, bool sync_runtime) -> bool {
    set_full_width();
    if (!ImGui::InputText(label, buffer, buffer_size)) {
      return false;
    }
    if (sync_runtime) {
      setting_changed();
    } else {
      ext_client::config::mark_dirty();
    }
    return true;
  }

  auto two_column_table(const char* id, int flags, const std::function<void()>& draw_cells) -> void {
    if (ImGui::BeginTable(id, 2, flags)) {
      draw_cells();
      ImGui::EndTable();
    }
  }

  auto checkbox_pair_table(const char* id, bool* left, const char* left_label, bool* right, const char* right_label) -> bool {
    bool changed = false;
    two_column_table(id, ImGuiTableFlags_SizingStretchSame, [&]() {
      ImGui::TableNextColumn();
      if (ImGui::Checkbox(left_label, left)) {
        changed = true;
      }
      ImGui::TableNextColumn();
      if (ImGui::Checkbox(right_label, right)) {
        changed = true;
      }
    });
    if (changed) {
      setting_changed();
    }
    return changed;
  }

  auto avail_width(float min_w) -> float {
    return std::max(ImGui::GetContentRegionAvail().x, min_w);
  }

  auto avail_height(float reserve, float min_h) -> float {
    return std::max(ImGui::GetContentRegionAvail().y - reserve, min_h);
  }

  auto constrain_window(ImGuiWindow* window) -> void {
    if (window == nullptr || window->Collapsed) {
      return;
    }
    if ((window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoMove)) != 0) {
      return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 size = window->SizeFull;
    const float vp_x0 = viewport->WorkPos.x;
    const float vp_y0 = viewport->WorkPos.y;
    const float vp_x1 = viewport->WorkPos.x + viewport->WorkSize.x;
    const float vp_y1 = viewport->WorkPos.y + viewport->WorkSize.y;

    float min_x = vp_x0;
    float max_x = vp_x1 - size.x;
    float min_y = vp_y0;
    float max_y = vp_y1 - size.y;
    if (size.x >= viewport->WorkSize.x) {
      min_x = max_x = vp_x0;
    }
    if (size.y >= viewport->WorkSize.y) {
      min_y = max_y = vp_y0;
    }
    if (min_x > max_x) {
      min_x = max_x = vp_x0;
    }
    if (min_y > max_y) {
      min_y = max_y = vp_y0;
    }

    const ImVec2 clamped = {std::clamp(window->Pos.x, min_x, max_x), std::clamp(window->Pos.y, min_y, max_y)};
    if (clamped.x != window->Pos.x || clamped.y != window->Pos.y) {
      ImGui::SetWindowPos(window, clamped, ImGuiCond_Always);
    }
  }

  auto constrain_windows_to_viewport() -> void {
    ImGuiContext& ctx = *GImGui;
    if (ctx.MovingWindow != nullptr && ctx.MovingWindow->RootWindow != nullptr) {
      constrain_window(ctx.MovingWindow->RootWindow);
    }
    for (ImGuiWindow* window : ctx.Windows) {
      constrain_window(window);
    }
  }

  auto tool_window_begin(const char* title, bool* open, ImVec2 default_size, ImGuiWindowFlags extra_flags) -> bool {
    if (!open || !*open) {
      return false;
    }

    if (ImGuiWindow* existing = ImGui::FindWindowByName(title)) {
      constrain_window(existing);
      ImGui::SetNextWindowPos(existing->Pos, ImGuiCond_Always);
    }

    ImGui::SetNextWindowSize(default_size, ImGuiCond_FirstUseEver);
    constexpr ImGuiWindowFlags base_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    const ImGuiWindowFlags flags = base_flags | extra_flags;
    if (!ImGui::Begin(title, open, flags)) {
      ImGui::End();
      return false;
    }

    constrain_window(ImGui::GetCurrentWindow());
    return true;
  }

  auto tool_window_end() -> void {
    ImGui::End();
  }

} // namespace ext_client::menu::ui

// ---------------------------------------------------------------------------
// settings_combat (internal to menu.cpp — only called by menu_shell)
// ---------------------------------------------------------------------------

namespace ext_client::menu::settings_combat {

  auto draw() -> void {
    ImGui::TextDisabled("Monster HP percent overlay.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("target_window_card")) {
      auto& tw = ext_client::config::data().target_window;
      bool changed = false;

      if (ImGui::Checkbox("Enable target window overlay", &tw.enabled)) {
        changed = true;
      }
      if (ImGui::Checkbox("Show monster HP % in target window", &tw.show_hp_percent)) {
        changed = true;
      }

      ImGui::TextDisabled("HP %% works on common + special mob target windows.");

      if (changed) {
        ext_client::config::mark_dirty();
      }

      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_combat

// ---------------------------------------------------------------------------
// menu_controller
// ---------------------------------------------------------------------------

namespace ext_client::menu {

  auto menu_controller::get() -> menu_controller& {
    static menu_controller instance;
    return instance;
  }

  auto menu_controller::is_visible() const -> bool {
    return m_visible;
  }

  auto menu_controller::set_visible(bool visible) -> void {
    if (m_visible == visible) {
      return;
    }

    m_visible = visible;

    ImGuiIO& io = ImGui::GetIO();
    io.ClearEventsQueue();
    io.ClearInputKeys();
    io.ClearInputMouse();

    if (visible) {
      m_capture_input_frames = 2;
      menu_shell::get().on_shown();
    } else {
      m_capture_input_frames = 0;
      m_wants_capture_mouse = false;
      m_wants_capture_keyboard = false;
    }
  }

  auto menu_controller::toggle() -> void {
    set_visible(!m_visible);
  }

  auto menu_controller::wants_capture_mouse() const -> bool {
    return m_visible && m_wants_capture_mouse;
  }

  auto menu_controller::wants_capture_keyboard() const -> bool {
    return m_visible && m_wants_capture_keyboard;
  }

  auto menu_controller::draw_shell() -> void {
    if (!m_visible) {
      return;
    }

    if (m_capture_input_frames > 0) {
      if (m_capture_input_frames == 2) {
        ImGui::SetNextWindowFocus();
      }
      --m_capture_input_frames;
    }

    menu_shell::get().draw(&m_visible);
  }

  auto menu_controller::sync_capture_state() -> void {
    const ImGuiIO& io = ImGui::GetIO();
    m_wants_capture_mouse = io.WantCaptureMouse;
    m_wants_capture_keyboard = io.WantCaptureKeyboard;
  }

} // namespace ext_client::menu

// ---------------------------------------------------------------------------
// menu_shell
// ---------------------------------------------------------------------------

namespace ext_client::menu {

  auto menu_shell::get() -> menu_shell& {
    static menu_shell instance;
    return instance;
  }

  auto menu_shell::on_shown() -> void {
    const bool any_open = m_windows.settings || m_windows.interface_mgr ||
                          m_windows.combat || m_windows.network || m_windows.debug || m_windows.widgets;
    if (!any_open) {
      m_windows.interface_mgr = true;
    }
  }

  auto menu_shell::draw(bool* visible) -> void {
    if (!visible || !*visible) {
      return;
    }

    draw_menu_bar(visible);
    if (!*visible) {
      return;
    }

    draw_settings_window();

    if (ui::tool_window_begin("Interface Manager##ext_client", &m_windows.interface_mgr, ImVec2(920.0f, 640.0f))) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      interface_manager::get().draw();
      ImGui::PopStyleVar();
      ui::tool_window_end();
    }

    if (ui::tool_window_begin("Combat##ext_client", &m_windows.combat, ImVec2(480.0f, 420.0f))) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      settings_combat::draw();
      ImGui::PopStyleVar();
      ui::tool_window_end();
    }

    if (ui::tool_window_begin("Network##ext_client", &m_windows.network, ImVec2(980.0f, 620.0f))) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      packet_inspector::draw();
      ImGui::PopStyleVar();
      ui::tool_window_end();
    }

    if (ui::tool_window_begin("Debug##ext_client", &m_windows.debug, ImVec2(640.0f, 360.0f))) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      debug_profiler::draw();
      ImGui::PopStyleVar();
      ui::tool_window_end();
    }

#if EXT_CLIENT_DEV_BUILD
    if (ui::tool_window_begin("Widgets##ext_client", &m_windows.widgets, ImVec2(900.0f, 640.0f))) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      widget_inspector::draw();
      ImGui::PopStyleVar();
      ui::tool_window_end();
    }
#endif
  }

  auto menu_shell::draw_settings_window() -> void {
    if (!ui::tool_window_begin("Settings##ext_client", &m_windows.settings, ImVec2(720.0f, 560.0f))) {
      return;
    }

    if (ImGui::BeginTable("settings_layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
      ImGui::TableSetupColumn("subnav", ImGuiTableColumnFlags_WidthFixed, theme::k_settings_subnav_width);
      ImGui::TableSetupColumn("settings_content", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      const float subnav_h = ui::avail_height(0.0f, 120.0f);
      if (ImGui::BeginChild("settings_subnav", ImVec2(0.0f, subnav_h), ImGuiChildFlags_Border)) {
        settings_panels::draw_sub_nav(m_settings_page);
        ImGui::EndChild();
      }

      ImGui::TableSetColumnIndex(1);
      if (ImGui::BeginChild("settings_content", ImVec2(0.0f, subnav_h), ImGuiChildFlags_Border)) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
        settings_panels::draw_page(m_settings_page);
        ImGui::PopStyleVar();
        ImGui::EndChild();
      }

      ImGui::EndTable();
    }

    ui::tool_window_end();
  }

  auto menu_shell::draw_menu_bar(bool* visible) -> void {
    if (!ImGui::BeginMainMenuBar()) {
      return;
    }

    if (ext_client::config::is_dirty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "*");
      ImGui::SameLine(0.0f, 4.0f);
    }

    if (ImGui::BeginMenu("Settings")) {
      const auto& list = settings_panels::panels();
      for (int i = 0; i < static_cast<int>(list.size()); ++i) {
        if (ImGui::MenuItem(list[static_cast<std::size_t>(i)].nav_label)) {
          m_settings_page = i;
          m_windows.settings = true;
        }
      }
      ImGui::EndMenu();
    }

    ImGui::MenuItem("Interface", nullptr, &m_windows.interface_mgr);
    ImGui::MenuItem("Combat", nullptr, &m_windows.combat);
    ImGui::MenuItem("Network", nullptr, &m_windows.network);
    ImGui::MenuItem("Debug", nullptr, &m_windows.debug);
#if EXT_CLIENT_DEV_BUILD
    ImGui::MenuItem("Widgets", nullptr, &m_windows.widgets);
#endif

    if (ImGui::BeginMenu("Window")) {
      if (ImGui::MenuItem("Show all")) {
        m_windows.settings = true;
        m_windows.interface_mgr = true;
        m_windows.combat = true;
        m_windows.network = true;
        m_windows.debug = true;
#if EXT_CLIENT_DEV_BUILD
        m_windows.widgets = true;
#endif
      }
      if (ImGui::MenuItem("Hide all")) {
        m_windows.settings = false;
        m_windows.interface_mgr = false;
        m_windows.combat = false;
        m_windows.network = false;
        m_windows.debug = false;
        m_windows.widgets = false;
      }
      ImGui::EndMenu();
    }

    const char* config_path = ext_client::config::path();
    const float path_w = ImGui::CalcTextSize(config_path).x;
    const float close_w = ImGui::CalcTextSize("Close").x + ImGui::CalcTextSize("Insert").x + 48.0f;
    const float right_pad = path_w + close_w + 24.0f;
    const float spare = ui::avail_width() - right_pad;
    if (spare > 8.0f) {
      ImGui::SameLine(spare);
    }

    ImGui::TextDisabled("%s", config_path);
    ImGui::SameLine(0.0f, 16.0f);
    if (ImGui::MenuItem("Close", "Insert")) {
      *visible = false;
    }

    ImGui::EndMainMenuBar();
  }

} // namespace ext_client::menu
