#pragma once

#include "sdk/game/cobj.hpp"
#include "utils/offsets.hpp"

class ccompound_obj;

// ---------------------------------------------------------------------------
// CObjChild — Shared lifecycle object base (size 0x1C)
// Inherits virtually from CObj
// ---------------------------------------------------------------------------
class cobj_child : public virtual cobj {
public:
  // 1. Compound Object Relationship
  [[nodiscard]] auto compound_obj() const -> ccompound_obj* {
    return ext_client::off::field_at<ccompound_obj*>(this, 0x004);
  }
};
