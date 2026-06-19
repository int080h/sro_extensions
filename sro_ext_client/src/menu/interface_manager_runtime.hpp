#pragma once

#include <cstdint>

namespace ext_client::menu::interface_manager_runtime {

  auto apply_saved_hides() -> void;
  auto tick() -> void;

  auto add_hidden_widget(int res_key, bool ingame_map, const char* label) -> bool;
  auto remove_hidden_widget(int res_key, bool ingame_map) -> bool;
  auto is_hidden_widget(int res_key, bool ingame_map) -> bool;

} // namespace ext_client::menu::interface_manager_runtime
