#include "pch.hpp"
#include "sdk/game/ci_entity.hpp"

#include "sdk/runtime/entity_runtime.hpp"

// ===========================================================================
// 1. Type Inspection & Casting
// ===========================================================================
auto ci_entity::is_instance(const void* ptr) -> bool {
  return ext_client::entity_runtime::is_class_name_match(ptr, "CIEntity");
}

auto ci_entity::cast(void* ptr) -> ci_entity* {
  return is_instance(ptr) ? static_cast<ci_entity*>(ptr) : nullptr;
}

auto ci_entity::cast(const void* ptr) -> const ci_entity* {
  return is_instance(ptr) ? static_cast<const ci_entity*>(ptr) : nullptr;
}
