#include "pch.hpp"
#include "sdk/game/centity_manager_client.hpp"

#include "sdk/render/cg_interface.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::off::as_fn;
  using ext_client::off::field_at;
} // namespace

// ===========================================================================
// 1. Singleton & Type Inspection
// ===========================================================================
auto centity_manager_client::is_instance(const void* ptr) -> bool {
  if (!ptr || !ext_client::utils::memory::is_game_ptr(ptr)) {
    return false;
  }
  return ext_client::rtti::is_kind_of(ptr, "CEntityManagerClient") ||
         ext_client::gfx_runtime::is_class_name_match(ptr, "CEntityManagerClient");
}

auto centity_manager_client::cast(void* ptr) -> centity_manager_client* {
  return is_instance(ptr) ? static_cast<centity_manager_client*>(ptr) : nullptr;
}

auto centity_manager_client::cast(const void* ptr) -> const centity_manager_client* {
  return is_instance(ptr) ? static_cast<const centity_manager_client*>(ptr) : nullptr;
}

auto centity_manager_client::from_wrapper(void* wrapper) -> centity_manager_client* {
  if (!wrapper || !ext_client::utils::memory::is_game_ptr(wrapper)) {
    return nullptr;
  }
  using get_mgr_fn = centity_manager_client*(__thiscall*)(void*);
  const auto fn = as_fn<get_mgr_fn>(0x00781E10);
  centity_manager_client* mgr = fn ? fn(wrapper) : nullptr;
  if (!mgr || !ext_client::utils::memory::is_game_ptr(mgr)) {
    return nullptr;
  }
  return mgr;
}

auto centity_manager_client::get() -> centity_manager_client* {
  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return nullptr;
  }
  using get_wrapper_fn = void*(__thiscall*)(cg_interface*);
  const auto get_wrapper = as_fn<get_wrapper_fn>(0x008831F0);
  void* wrapper = get_wrapper ? get_wrapper(iface) : nullptr;
  return from_wrapper(wrapper);
}

// ===========================================================================
// 2. Entity Queries
// ===========================================================================
auto centity_manager_client::lookup_entity_by_slot(std::uint32_t slot_id) const -> ci_charactor* {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return nullptr;
  }
  using lookup_fn = ci_charactor*(__thiscall*)(const centity_manager_client*, std::uint32_t);
  const auto fn = as_fn<lookup_fn>(0x00793E80); // CEntityManager_LookupBySlot
  return fn ? fn(this, slot_id) : nullptr;
}
