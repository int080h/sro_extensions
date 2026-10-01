#pragma once

#include "sdk/types/ingame_res_lookup.hpp"
#include "sdk/types/cnif_sro_ingame_start_live.hpp"

struct survey_resolve_diag {
  ingame_res_lookup start_map{};
  ingame_res_lookup info_map{};
  cnif_sro_ingame_start_live live{};
  bool ready = false;
};
