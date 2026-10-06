#pragma once

#include "sdk/ui/cgwnd.hpp"

// ---------------------------------------------------------------------------
// CNIFSroInGameInfo — "Take Survey" browser window (map key 0x34)
// Extends CGWnd
// ---------------------------------------------------------------------------
class cnif_sro_ingame_info : public cgwnd {
public:
  // 1. Actions
  auto hide_info_panel() -> void;
};
