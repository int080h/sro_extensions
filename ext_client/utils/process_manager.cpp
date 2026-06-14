#include "utils/process_manager.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

namespace ext_client::utils::process_manager {

  auto instance() -> void* {
    if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::process_manager::address), sizeof(void*))) {
      return nullptr;
    }
    return ext_client::offsets::global_at<void*>(ext_client::offsets::process_manager::address);
  }

  auto active_child() -> void* {
    auto* mgr = instance();
    if (!mgr || !ext_client::msvc9::is_readable_ptr(
                  reinterpret_cast<const std::uint8_t*>(mgr) + ext_client::offsets::process_manager::fields::active_child, sizeof(void*))) {
      return nullptr;
    }
    return ext_client::offsets::field_at<void*>(mgr, ext_client::offsets::process_manager::fields::active_child);
  }

  auto active_child_vftable() -> std::uint32_t {
    const void* child = active_child();
    std::uint32_t vft = 0;
    if (!child || !ext_client::msvc9::try_read_u32(child, &vft)) {
      return 0;
    }
    return vft;
  }

  auto cached_active_child() -> void*& {
    static void* ptr = nullptr;
    return ptr;
  }

  auto is_child_readable(void* child) -> bool {
    if (!child || !ext_client::msvc9::is_readable_ptr(child, sizeof(void*) * 4)) {
      return false;
    }
    std::uint32_t vft = 0;
    if (!ext_client::msvc9::try_read_u32(child, &vft)) {
      return false;
    }
    return ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(vft)), sizeof(void*) * 4);
  }

  auto resolved_active_child() -> void* {
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

  auto note_set_child_process(void* child, int activate) -> void {
    if (activate != 0 && child != nullptr && is_child_readable(child)) {
      cached_active_child() = child;
    }
  }

  auto set_child_process(int process_type, bool activate) -> int {
    auto* mgr = instance();
    if (!mgr) {
      return 0;
    }
    using set_child_process_fn = int(__thiscall*)(void* process_mgr, int process_type, int activate);
    const auto fn = ext_client::offsets::as_fn<set_child_process_fn>(ext_client::offsets::process_manager::functions::set_child_process);
    return fn(mgr, process_type, activate ? 1 : 0);
  }

  auto quit_process() -> void {
    const auto fn = ext_client::offsets::as_fn<void(__cdecl*)()>(ext_client::offsets::process_manager::functions::quit_process);
    fn();
  }

} // namespace ext_client::utils::process_manager
