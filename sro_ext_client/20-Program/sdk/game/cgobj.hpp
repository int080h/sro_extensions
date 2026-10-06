#pragma once

#include "utils/msvc9_stl.hpp"

// ---------------------------------------------------------------------------
// CGObj — Compact game-object base used by in-game net processes (CNetProcessIn)
// ---------------------------------------------------------------------------
class cgobj {
public:
  virtual ~cgobj() = default;
};
