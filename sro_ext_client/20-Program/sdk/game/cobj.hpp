#pragma once

#include "utils/offsets.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CObj — Root base of the Silkroad Online game object diamond (mdisp = 0)
// ---------------------------------------------------------------------------
class cobj {
public:
  [[nodiscard]] auto get_vftable() const -> const std::uintptr_t* {
    return ext_client::off::raw_vftable(this);
  }
};
