#pragma once

#include "sdk/game/cobj.hpp"

class ccompound_obj;

// CObjChild — shared lifecycle base (size 0x1C).
class cobj_child : public virtual cobj {
public:
  auto get_compound_obj() -> ccompound_obj*;
  auto get_field_0c() -> int;
  auto get_list_prev() -> void*;
  auto get_list_next() -> void*;
  auto set_compound_obj(ccompound_obj* val) -> void;
  auto set_field_0c(int val) -> void;
  auto set_list_prev(void* val) -> void;
  auto set_list_next(void* val) -> void;
};
