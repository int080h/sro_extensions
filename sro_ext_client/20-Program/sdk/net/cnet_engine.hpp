#pragma once

#include "sdk/types/ibsnet_init_config.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CNetEngine / IBSNet — Joymax Winsock socket layer interface
// ---------------------------------------------------------------------------
class cnet_engine {
public:
  virtual ~cnet_engine() = default;
};
