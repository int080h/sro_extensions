#pragma once

#include <cstdint>
#include <cstring>

class cprocess;

// ---------------------------------------------------------------------------
// CControler — Global game process / state controller singleton
// Native Pointer: 0x013BAE38 / 0x013BAE00 | Native VTable: 0x0103282C
// Responsible for active process switching, factory resolution, and state routing.
// ---------------------------------------------------------------------------
class ccontroler {
public:
  static constexpr std::uint32_t k_singleton_addr = 0x013BAE00;

  // 1. Singleton & Runtime Inspection
  static auto get() -> ccontroler*;
  static auto is_process(const void* ptr, const char* expected_name) -> bool;

  // 2. Active Process Queries
  static auto active_child() -> cprocess*;
  static auto active_child_process_name() -> const char*;
  static auto active_child_factory_entry() -> void*;

  template<typename T>
  static auto active_child_as(const char* expected_name) -> T* {
    const char* name = active_child_process_name();
    if (!name || std::strcmp(name, expected_name) != 0) {
      return nullptr;
    }
    return reinterpret_cast<T*>(active_child());
  }

  // 3. Process Factory & Reflection
  static auto factory_entry(const void* ptr) -> void*;
  static auto factory_entry_name(const void* ptr) -> const char*;

  // 4. Process Lifecycle & Detours
  static auto set_child_process(void* current_process, int factory_entry_ptr, bool activate) -> int;
  static auto quit_process(int factory_entry_ptr) -> void;
};
