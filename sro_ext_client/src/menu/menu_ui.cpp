#include "ext_client.hpp"



#include "menu/menu_ui.hpp"



#include "utils/sro_color.hpp"



#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>



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

