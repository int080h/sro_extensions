#pragma once

#include "sdk/ui/cgwnd.hpp"

// CNIFSroInGameInfo — "Take Survey" browser window (map key 0x34), not the HUD button.
class cnif_sro_ingame_info : public cgwnd {
public:
  auto hide_info_panel() -> void;
};
