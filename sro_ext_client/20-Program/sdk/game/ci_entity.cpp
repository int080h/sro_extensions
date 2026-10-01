#include "pch.hpp"
#include "sdk/game/ci_entity.hpp"

#include "sdk/runtime/entity_runtime.hpp"

auto ci_entity::is_instance(const void* ptr) -> bool {
  return ext_client::entity_runtime::is_class_name_match(ptr, "CIEntity");
}
