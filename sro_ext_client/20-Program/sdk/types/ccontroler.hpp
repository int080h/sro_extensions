#pragma once

#include <cstring>

class cprocess;

struct ccontroler {
  auto get_process() -> cprocess*;
  auto set_process(cprocess* val) -> void;

  static auto get() -> ccontroler*;
  static auto active_child() -> cprocess*;
  static auto active_child_factory_entry() -> void*;
  static auto active_child_process_name() -> const char*;
  static auto factory_entry(const void* ptr) -> void*;
  static auto factory_entry_name(const void* ptr) -> const char*;
  static auto is_process(const void* ptr, const char* expected_name) -> bool;

  template<typename T> static auto active_child_as(const char* expected_name) -> T* {
    const char* name = active_child_process_name();
    if (!name || std::strcmp(name, expected_name) != 0) {
      return nullptr;
    }
    return reinterpret_cast<T*>(active_child());
  }

  static auto cached_active_child() -> void*&;
  static auto is_child_readable(void* child) -> bool;
  static auto resolved_active_child() -> void*;
  static auto note_set_child_process(void* child, int activate) -> void;
  static auto set_child_process(void* current_process, int factory_entry_ptr, bool activate) -> int;
  static auto quit_process(int factory_entry_ptr) -> void;
};
