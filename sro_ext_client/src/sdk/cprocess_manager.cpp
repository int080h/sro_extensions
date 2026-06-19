#include "sdk/cprocess_manager.hpp"

#include "sdk/cprocess.hpp"

namespace {

  auto safe_read_u32(const void* ptr) -> std::uint32_t {
    std::uint32_t val = 0;
    if (!ext_client::msvc9::try_read_u32(ptr, &val)) {
      return 0;
    }
    return val;
  }

} // namespace

auto cprocess_manager::get() -> cprocess_manager* {
  const auto addr = ext_client::offsets::process_manager::address;
  if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(addr), sizeof(void*))) {
    return nullptr;
  }
  auto* mgr = ext_client::offsets::global_at<cprocess_manager*>(addr);
  if (!mgr || !ext_client::msvc9::is_readable_ptr(mgr, sizeof(cprocess_manager))) {
    return nullptr;
  }
  return mgr;
}

auto cprocess_manager::active_child() -> cprocess* {
  auto* mgr = get();
  if (!mgr) {
    return nullptr;
  }
  auto* child = mgr->m_process_child;
  if (!child || !ext_client::msvc9::is_readable_ptr(child, sizeof(void*))) {
    return nullptr;
  }
  return child;
}

auto cprocess_manager::active_child_vftable() -> std::uint32_t {
  auto* child = active_child();
  if (!child) {
    return 0;
  }
  return safe_read_u32(child);
}

auto cprocess_manager::is_ingame() -> bool {
  const auto vft = active_child_vftable();
  if (vft == 0) {
    return false;
  }
  // In-game = active screen is NOT one of the pre-game screens.
  return vft != ext_client::offsets::cps_title::vtable::address &&
         vft != ext_client::offsets::cps_character_select::vtable::address &&
         vft != ext_client::offsets::cps_version_check::vtable::address;
}

auto cprocess_manager::is_title() -> bool {
  return active_child_vftable() == ext_client::offsets::cps_title::vtable::address;
}

auto cprocess_manager::is_character_select() -> bool {
  return active_child_vftable() == ext_client::offsets::cps_character_select::vtable::address;
}

auto cprocess_manager::is_version_check() -> bool {
  return active_child_vftable() == ext_client::offsets::cps_version_check::vtable::address;
}
