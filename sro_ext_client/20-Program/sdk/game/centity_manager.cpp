#include "pch.hpp"
#include "sdk/game/centity_manager.hpp"

#include "sdk/game/ci_charactor.hpp"
#include "sdk/game/ci_entity.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;

} // namespace

auto centity_manager::get_singleton() -> centity_manager* {
  return global_at<centity_manager*>(0x013BAE08);
}

auto centity_manager::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(ptr, "CEntityManager");
}

auto centity_manager::lookup_by_slot(std::uint32_t slot_id) const -> ci_charactor* {
  using lookup_fn = ci_charactor*(__thiscall*)(const centity_manager*, std::uint32_t);
  const auto fn = as_fn<lookup_fn>(0x00793E80);
  return fn(this, slot_id);
}

auto centity_manager::lookup_entity(std::uint32_t slot_id) const -> ci_entity* {
  return reinterpret_cast<ci_entity*>(lookup_by_slot(slot_id));
}

auto centity_manager::get_entity_begin() const -> ci_charactor* const* {
  return &ext_client::off::field_at<ci_charactor*>(this, 0x3A8);
}

auto centity_manager::get_entity_end() const -> ci_charactor* const* {
  return &ext_client::off::field_at<ci_charactor*>(this, 0x3AC);
}

auto centity_manager::get_entity_count() const -> std::size_t {
  return static_cast<std::size_t>(
    ext_client::off::field_at<ci_charactor*>(this, 0x3AC) -
    ext_client::off::field_at<ci_charactor*>(this, 0x3A8)
  ) / sizeof(ci_charactor*);
}

