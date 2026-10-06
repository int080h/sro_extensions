#pragma once

#include "sdk/game/cobj_child.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CIEntity — Base Interactive Entity class in game engine
// RTTI: "CIEntity" | Extends CObjChild
// ---------------------------------------------------------------------------
class ci_entity : public cobj_child {
public:
  // 1. Type Inspection & Casting
  static auto is_instance(const void* ptr) -> bool;
  static auto cast(void* ptr) -> ci_entity*;
  static auto cast(const void* ptr) -> const ci_entity*;
};
