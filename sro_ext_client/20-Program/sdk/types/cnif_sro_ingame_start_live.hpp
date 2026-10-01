#pragma once

#include <cstdint>

class cnif_sro_ingame_start;
class cnif_sro_ingame_info;
class cgwnd;

struct cnif_sro_ingame_start_live {
  cnif_sro_ingame_start* start_panel = nullptr;
  cnif_sro_ingame_info* info_panel = nullptr;
  cgwnd* survey_button = nullptr;
};
