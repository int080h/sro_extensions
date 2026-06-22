#include "centity_manager.hpp"

#include "cicharactor.hpp"
#include "utils/rtti.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

auto centity_manager::get_singleton() -> centity_manager* {
  return global_at<centity_manager*>(ext_client::offsets::centity_manager::globals::singleton);
}

auto centity_manager::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(ptr, "CEntityManager");
}

auto centity_manager::lookup_by_slot(std::uint32_t slot_id) const -> ci_charactor* {
  using lookup_fn = ci_charactor*(__thiscall*)(const centity_manager*, std::uint32_t);
  const auto fn = as_fn<lookup_fn>(ext_client::offsets::centity_manager::functions::lookup_by_slot);
  return fn(this, slot_id);
}

auto centity_manager::entity_begin() const -> ci_charactor* const* {
  return m_entities.begin();
}

auto centity_manager::entity_end() const -> ci_charactor* const* {
  return m_entities.end();
}

auto centity_manager::entity_count() const -> std::size_t {
  return m_entities.size();
}

