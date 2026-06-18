#include "centity_manager_client.hpp"

#include "cg_interface.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::field_at;

} // namespace

auto centity_manager_client::is_instance(const void* ptr) -> bool {
  if (!ptr || !ext_client::msvc9::is_readable_ptr(ptr, sizeof(void*))) {
    return false;
  }
  return *reinterpret_cast<const std::uint32_t*>(ptr) == ext_client::offsets::centity_manager_client::vtable::address;
}

auto centity_manager_client::from_wrapper(void* wrapper) -> centity_manager_client* {
  if (!wrapper || !ext_client::msvc9::is_readable_ptr(wrapper, ext_client::offsets::entity_manager::fields::wrapper_ptr + sizeof(void*))) {
    return nullptr;
  }

  using get_mgr_fn = centity_manager_client*(__thiscall*)(void*);
  const auto fn = as_fn<get_mgr_fn>(ext_client::offsets::entity_manager::functions::from_wrapper);
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
  const auto get_wrapper = as_fn<get_wrapper_fn>(ext_client::offsets::cg_interface::functions::get_component);
  void* wrapper = get_wrapper(iface);
  return from_wrapper(wrapper);
}

auto centity_manager_client::lookup_entity_by_slot(std::uint32_t slot_id) const -> ci_charactor* {
  using lookup_fn = ci_charactor*(__thiscall*)(const centity_manager_client*, std::uint32_t);
  const auto fn = as_fn<lookup_fn>(ext_client::offsets::entity_manager::functions::lookup_entity_by_slot);
  return fn(this, slot_id);
}
