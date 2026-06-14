#pragma once

namespace ext_client::menu::ui {

  auto checkbox_column(bool* left, const char* left_label, bool* right, const char* right_label) -> bool;
  auto setting_changed(bool sync_runtime = true) -> void;
  auto checkbox_setting(const char* label, bool* value, bool sync_runtime = true) -> bool;

} // namespace ext_client::menu::ui
