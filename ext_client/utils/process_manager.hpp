#pragma once

#include <cstdint>

namespace ext_client::utils::process_manager {

  auto instance() -> void*;

  auto active_child() -> void*;

  auto active_child_vftable() -> std::uint32_t;

  template<typename T> auto active_child_as(std::uint32_t expected_vftable) -> T* {
    if (active_child_vftable() != expected_vftable) {
      return nullptr;
    }
    return reinterpret_cast<T*>(active_child());
  }

  auto cached_active_child() -> void*&;

  auto is_child_readable(void* child) -> bool;

  auto resolved_active_child() -> void*;

  auto note_set_child_process(void* child, int activate) -> void;

  auto set_child_process(int process_type, bool activate) -> int;

  auto quit_process() -> void;

} // namespace ext_client::utils::process_manager
