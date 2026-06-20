#include "centity_manager.hpp"

#include "cicharactor.hpp"

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
  const auto vtable = *reinterpret_cast<const std::uint32_t*>(ptr);
  return vtable == ext_client::offsets::centity_manager::vtable::address;
}

auto centity_manager::lookup_by_slot(std::uint32_t slot_id) const -> ci_charactor* {
  using lookup_fn = ci_charactor*(__thiscall*)(const centity_manager*, std::uint32_t);
  const auto fn = as_fn<lookup_fn>(ext_client::offsets::centity_manager::functions::lookup_by_slot);
  return fn(this, slot_id);
}

auto centity_manager::entity_begin() const -> ci_charactor* const* {
  return m_entity_begin;
}

auto centity_manager::entity_end() const -> ci_charactor* const* {
  return m_entity_end;
}

auto centity_manager::entity_count() const -> std::size_t {
  if (!m_entity_begin || !m_entity_end || m_entity_end < m_entity_begin) {
    return 0;
  }
  return static_cast<std::size_t>(m_entity_end - m_entity_begin);
}
