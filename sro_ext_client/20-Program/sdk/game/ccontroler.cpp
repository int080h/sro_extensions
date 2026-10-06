#include "pch.hpp"
#include "sdk/game/ccontroler.hpp"

#include "sdk/process/cprocess.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/memory.hpp"
#include "utils/offsets.hpp"

#include <cstring>

namespace {
  auto safe_read_u32(const void* ptr) -> std::uint32_t {
    if (!ptr) {
      return 0;
    }
    return *reinterpret_cast<const std::uint32_t*>(ptr);
  }
} // namespace

// ===========================================================================
// 1. Singleton & Runtime Inspection
// ===========================================================================
auto ccontroler::get() -> ccontroler* {
  const auto addr = k_singleton_addr;
  auto* mgr = ext_client::off::global_at<ccontroler*>(addr);
  if (!mgr) {
    return nullptr;
  }
  return mgr;
}

auto ccontroler::is_process(const void* ptr, const char* expected_name) -> bool {
  const char* name = factory_entry_name(ptr);
  if (!name || !expected_name) {
    return false;
  }
  if (std::strcmp(name, expected_name) == 0) {
    return true;
  }
  // Tolerant aliases for Silkroad game world processes
  if ((std::strcmp(expected_name, "CPSSilkroad") == 0 || std::strcmp(expected_name, "CPSilkroad") == 0) &&
      (std::strcmp(name, "CPSilkroad") == 0 || std::strcmp(name, "CPSOuterInterface") == 0)) {
    return true;
  }
  return false;
}

// ===========================================================================
// 2. Active Process Queries
// ===========================================================================
auto ccontroler::active_child() -> cprocess* {
  auto* mgr = get();
  if (!mgr) {
    return nullptr;
  }
  auto* child = ext_client::off::field_at<cprocess*>(mgr, 0x024);
  if (!child) {
    return nullptr;
  }
  return child;
}

auto ccontroler::active_child_factory_entry() -> void* {
  return factory_entry(active_child());
}

auto ccontroler::active_child_process_name() -> const char* {
  return factory_entry_name(active_child());
}

// ===========================================================================
// 3. Process Factory & Reflection
// ===========================================================================
auto ccontroler::factory_entry(const void* ptr) -> void* {
  if (!ptr || !ext_client::utils::memory::is_valid_ptr(ptr)) {
    return nullptr;
  }
  void* vft = nullptr;
  if (!ext_client::utils::memory::safe_read(ptr, vft) || !ext_client::utils::memory::is_valid_ptr(vft)) {
    return nullptr;
  }
  void* fn_ptr = nullptr;
  if (!ext_client::utils::memory::safe_read(vft, fn_ptr) || !ext_client::utils::memory::is_code_ptr(fn_ptr)) {
    return nullptr;
  }
  using get_factory_entry_fn = void* (__cdecl*)();
  const auto fn = reinterpret_cast<get_factory_entry_fn>(fn_ptr);
  return fn();
}

auto ccontroler::factory_entry_name(const void* ptr) -> const char* {
  void* entry = factory_entry(ptr);
  if (!entry || !ext_client::utils::memory::is_valid_ptr(entry)) {
    return nullptr;
  }
  char* name_ptr = nullptr;
  if (!ext_client::utils::memory::safe_read(entry, name_ptr) || !ext_client::utils::memory::is_valid_ptr(name_ptr)) {
    return nullptr;
  }
  return name_ptr;
}

// ===========================================================================
// 4. Process Lifecycle & Detours
// ===========================================================================

auto ccontroler::set_child_process(void* current_process, int factory_entry_ptr, bool activate) -> int {
  if (!current_process) {
    return 0;
  }
  using set_child_process_fn = int(__thiscall*)(void* current_process, int factory_entry_ptr, int activate);
  const auto fn = ext_client::off::as_fn<set_child_process_fn>(0x00D729E0);
  return fn(current_process, factory_entry_ptr, activate ? 1 : 0);
}

auto ccontroler::quit_process(int factory_entry_ptr) -> void {
  const auto fn = ext_client::off::as_fn<void(__cdecl*)(int)>(0x00D72440);
  fn(factory_entry_ptr);
}
