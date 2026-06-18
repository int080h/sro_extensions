#include "features/combat_overlay/combat_target.hpp"

#include "features/combat_overlay/entity_manager.hpp"
#include "cg_interface.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

namespace ext_client::combat_target {
  namespace {

    auto lookup_entity_by_slot(std::uint32_t slot_id) -> ci_charactor* {
      if (slot_id == 0) {
        return nullptr;
      }

      // sub_7E8090 / sub_7E8EB0: sub_B24030(slot) — single cdecl arg.
      using lookup_fn = ci_charactor*(__cdecl*)(int);
      const auto lookup = ext_client::offsets::as_fn<lookup_fn>(ext_client::offsets::entity_manager::functions::lookup_entity_by_slot);
      ci_charactor* entity = lookup(static_cast<int>(slot_id));
      if (!entity || !ext_client::msvc9::is_readable_ptr(entity, ext_client::offsets::ci_charactor::fields::max_hp + sizeof(std::uint32_t))) {
        return nullptr;
      }
      return entity;
    }

  } // namespace

  auto selected_slot_id() -> std::uint32_t {
    auto* iface = cg_interface::get();
    if (!iface) {
      return 0;
    }

    using get_selected_fn = std::uint32_t(__thiscall*)(cg_interface*);
    const auto fn = ext_client::offsets::as_fn<get_selected_fn>(ext_client::offsets::cg_interface::functions::get_selected_target_id);
    return fn(iface);
  }

  auto get_by_slot(std::uint32_t slot_id) -> ci_charactor* {
    if (slot_id == 0) {
      return nullptr;
    }

    if (ci_charactor* entity = lookup_entity_by_slot(slot_id)) {
      return entity;
    }

    if (void* mgr = entity_manager::get()) {
      return entity_manager::lookup_by_slot(mgr, slot_id);
    }
    return nullptr;
  }

  auto get_selected() -> ci_charactor* {
    const std::uint32_t slot_id = selected_slot_id();
    if (slot_id == 0) {
      return nullptr;
    }

    if (ci_charactor* entity = lookup_entity_by_slot(slot_id)) {
      return entity;
    }

    if (void* mgr = entity_manager::get()) {
      return entity_manager::lookup_by_slot(mgr, slot_id);
    }
    return nullptr;
  }

} // namespace ext_client::combat_target
