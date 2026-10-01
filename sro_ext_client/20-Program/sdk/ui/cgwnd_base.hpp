#pragma once

#include "sdk/game/cobj_child.hpp"
#include "sdk/types/cgwnd_bounds.hpp"

#include <cstdint>

// CGWndBase — extends CObjChild (+0x1C..+0x2B), vt @ 0x106874C (24 slots).
class cgwnd_base : public cobj_child {
public:
  auto get_field_1c() -> int;
  auto get_field_20() -> int;
  auto get_field_24() -> int;
  auto get_field_28() -> int;
  auto set_field_1c(int val) -> void;
  auto set_field_20(int val) -> void;
  auto set_field_24(int val) -> void;
  auto set_field_28(int val) -> void;
};
