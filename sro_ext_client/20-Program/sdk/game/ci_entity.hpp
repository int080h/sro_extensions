#pragma once

#include "sdk/game/cobj_child.hpp"

#include <cstdint>

class ci_entity : public cobj_child {
public:
  static auto is_instance(const void* ptr) -> bool;
};