#include "menu/menu_ui.hpp"

#include "config/client_config.hpp"

#include <imgui.h>

namespace ext_client::menu::ui {

  auto checkbox_column(bool* left, const char* left_label, bool* right, const char* right_label) -> bool {
    bool changed = false;
    if (ImGui::Checkbox(left_label, left)) {
      changed = true;
    }
    ImGui::SameLine(210.0f);
    if (ImGui::Checkbox(right_label, right)) {
      changed = true;
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

} // namespace ext_client::menu::ui
