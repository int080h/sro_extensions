#pragma once

#include "sdk/cicharactor.hpp"

#include <cstdint>

namespace ext_client::entity_manager {

  auto get() -> void*;
  auto lookup_by_slot(void* mgr, std::uint32_t slot_id) -> ci_charactor*;
  auto lookup_by_uid(void* mgr, std::uint32_t uid) -> ci_charactor*;
  auto unique_id(const ci_charactor* entity) -> std::uint32_t;
  auto display_name(const ci_charactor* entity) -> const wchar_t*;

} // namespace ext_client::entity_manager
