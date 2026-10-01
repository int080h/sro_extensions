#include "pch.hpp"
#include "sdk/game/centity_manager_client.hpp"

#include "sdk/render/cg_interface.hpp"
#include "sdk/runtime/rtti.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::field_at;

} // namespace

auto centity_manager_client::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(ptr, "CEntityManagerClient");
}

auto centity_manager_client::from_wrapper(void* wrapper) -> centity_manager_client* {
  if (!wrapper) {
    return nullptr;
  }

  using get_mgr_fn = centity_manager_client*(__thiscall*)(void*);
  const auto fn = as_fn<get_mgr_fn>(0x00781E10);
  centity_manager_client* mgr = fn(wrapper);
  if (!mgr || !is_instance(mgr)) {
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
  void* wrapper = get_wrapper(iface);
  return from_wrapper(wrapper);
}

auto centity_manager_client::lookup_entity_by_slot(std::uint32_t slot_id) const -> ci_charactor* {
  using lookup_fn = ci_charactor*(__thiscall*)(const centity_manager_client*, std::uint32_t);
  const auto fn = as_fn<lookup_fn>(0x00B24030);
  return fn(this, slot_id);
}
