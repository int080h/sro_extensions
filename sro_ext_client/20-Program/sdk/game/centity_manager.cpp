#include "pch.hpp"
#include "sdk/game/centity_manager.hpp"

#include "sdk/game/ci_charactor.hpp"
#include "sdk/game/ci_entity.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/memory.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::off::as_fn;
  using ext_client::off::global_at;
  using ext_client::utils::memory::is_game_ptr;
} // namespace

// ===========================================================================
// 1. Singleton & Runtime Inspection
// ===========================================================================
auto centity_manager::get_singleton() -> centity_manager* {
  auto* mgr = global_at<centity_manager*>(k_singleton_addr);
  if (is_game_ptr(mgr)) {
    return mgr;
  }
  static centity_manager s_fallback;
  return &s_fallback;
}

auto centity_manager::is_instance(const void* ptr) -> bool {
  if (!is_game_ptr(ptr)) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(ptr, "CEntityManager");
}

auto centity_manager::cast(void* ptr) -> centity_manager* {
  return is_instance(ptr) ? static_cast<centity_manager*>(ptr) : nullptr;
}

auto centity_manager::cast(const void* ptr) -> const centity_manager* {
  return is_instance(ptr) ? static_cast<const centity_manager*>(ptr) : nullptr;
}

// ===========================================================================
// 2. Entity Lookups
// ===========================================================================
auto centity_manager::lookup_by_slot(std::uint32_t slot_id) const -> ci_charactor* {
  if (!is_game_ptr(this)) {
    return nullptr;
  }
  using lookup_fn = ci_charactor*(__thiscall*)(const centity_manager*, std::uint32_t);
  const auto fn = as_fn<lookup_fn>(0x00793E80);
  return fn ? fn(this, slot_id) : nullptr;
}

auto centity_manager::lookup_entity(std::uint32_t slot_id) const -> ci_entity* {
  return reinterpret_cast<ci_entity*>(lookup_by_slot(slot_id));
}

// ===========================================================================
// 3. Entity Collections
// ===========================================================================
auto centity_manager::get_entity_begin() const -> ci_charactor* const* {
  if (!is_game_ptr(this)) {
    return nullptr;
  }
  auto* begin = ext_client::off::field_at<ci_charactor**>(this, 0x3A8);
  return is_game_ptr(begin) ? begin : nullptr;
}

auto centity_manager::get_entity_end() const -> ci_charactor* const* {
  if (!is_game_ptr(this)) {
    return nullptr;
  }
  auto* end = ext_client::off::field_at<ci_charactor**>(this, 0x3AC);
  return is_game_ptr(end) ? end : nullptr;
}

auto centity_manager::get_entity_count() const -> std::size_t {
  // 1. Canonical entity count in g_sHashGID (dword_1199128) via map_view
  const auto map = gid_map_view();
  if (const auto count = map.size(); count > 0) {
    return count;
  }

  // 2. Contiguous vector fallback (0x3A8 / 0x3AC) via vector_view
  const auto vec = entity_vector_view();
  return vec.size();
}

