#pragma once

#include "utils/offsets.hpp"

#include <cstdint>

class cobj_child;
class cgobj;

// Root of the Silkroad process object diamond (virtual, mdisp=0).
class cobj {
public:
  [[nodiscard]] auto get_vftable() const -> const std::uintptr_t* {
    return ext_client::off::raw_vftable(this);
  }
};
