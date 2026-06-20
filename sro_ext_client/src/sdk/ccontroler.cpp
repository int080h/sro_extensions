#include "sdk/ccontroler.hpp"

#include "sdk/cprocess.hpp"

#include <cstring>

namespace {

  auto safe_read_u32(const void* ptr) -> std::uint32_t {
    std::uint32_t val = 0;
    if (!ext_client::msvc9::try_read_u32(ptr, &val)) {
      return 0;
    }
    return val;
  }

} // namespace

auto ccontroler::get() -> ccontroler* {
  const auto addr = ext_client::offsets::ccontroler::address;
  if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(addr), sizeof(void*))) {
    return nullptr;
  }
  auto* mgr = ext_client::offsets::global_at<ccontroler*>(addr);
  if (!mgr || !ext_client::msvc9::is_readable_ptr(mgr, sizeof(ccontroler))) {
    return nullptr;
  }
  return mgr;
}

auto ccontroler::active_child() -> cprocess* {
  auto* mgr = get();
  if (!mgr) {
    return nullptr;
  }
  auto* child = mgr->m_process;
  if (!child || !ext_client::msvc9::is_readable_ptr(child, sizeof(void*))) {
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

auto ccontroler::factory_entry(const void* ptr) -> void* {
  if (!ptr || !ext_client::msvc9::is_readable_ptr(ptr, sizeof(void*))) {
    return nullptr;
  }
  // vtable slot 0 is get_factory_entry (VFN_CDECL — no 'this' parameter)
  const auto vft = *reinterpret_cast<void* const*>(ptr);
  if (!vft || !ext_client::msvc9::is_readable_ptr(vft, sizeof(void*))) {
    return nullptr;
  }
  using get_factory_entry_fn = void* (__cdecl*)();
  const auto fn = reinterpret_cast<get_factory_entry_fn>(*reinterpret_cast<void* const*>(vft));
  if (!fn) {
    return nullptr;
  }
  return fn();
}

auto ccontroler::factory_entry_name(const void* ptr) -> const char* {
  void* entry = factory_entry(ptr);
  if (!entry || !ext_client::msvc9::is_readable_ptr(entry, sizeof(char*))) {
    return nullptr;
  }
  // Factory entry offset 0x00 is char* name
  const auto name_ptr = *reinterpret_cast<char**>(entry);
  if (!name_ptr || !ext_client::msvc9::is_readable_ptr(name_ptr, 1)) {
    return nullptr;
  }
  return name_ptr;
}

auto ccontroler::cached_active_child() -> void*& {
  static void* ptr = nullptr;
  return ptr;
}

auto ccontroler::is_child_readable(void* child) -> bool {
  if (!child || !ext_client::msvc9::is_readable_ptr(child, sizeof(void*) * 4)) {
    return false;
  }
  std::uint32_t vft = 0;
  if (!ext_client::msvc9::try_read_u32(child, &vft)) {
    return false;
  }
  return ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(vft)), sizeof(void*) * 4);
}

auto ccontroler::resolved_active_child() -> void* {
  if (auto* live = active_child()) {
    cached_active_child() = live;
    return live;
  }
  if (is_child_readable(cached_active_child())) {
    return cached_active_child();
  }
  cached_active_child() = nullptr;
  return nullptr;
}

auto ccontroler::note_set_child_process(void* child, int activate) -> void {
  if (activate != 0 && child != nullptr && is_child_readable(child)) {
    cached_active_child() = child;
  }
}

auto ccontroler::set_child_process(void* current_process, int factory_entry_ptr, bool activate) -> int {
  if (!current_process) {
    return 0;
  }
  using set_child_process_fn = int(__thiscall*)(void* current_process, int factory_entry_ptr, int activate);
  const auto fn = ext_client::offsets::as_fn<set_child_process_fn>(ext_client::offsets::ccontroler::functions::set_child_process);
  return fn(current_process, factory_entry_ptr, activate ? 1 : 0);
}

auto ccontroler::quit_process(int factory_entry_ptr) -> void {
  const auto fn = ext_client::offsets::as_fn<void(__cdecl*)(int)>(ext_client::offsets::ccontroler::functions::quit_process);
  fn(factory_entry_ptr);
}
