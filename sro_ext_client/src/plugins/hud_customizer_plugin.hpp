#pragma once

#include <Windows.h>
#include <imgui.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "core/core_event_manager.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cps_outer_interface.hpp"
#include "sdk/cnif_sro_ingame_start.hpp"
#include "sdk/calram_guide_mgr_wnd.hpp"

namespace ext_client::plugins::hud_customizer {

  // =========================================================================
  // 1. Globals (Extern)
  // =========================================================================
  extern bool g_saw_ingame;

  // =========================================================================
  // 2. Main Functions
  // =========================================================================
  auto apply_quick_hides() -> void;
  auto apply_saved_hides() -> void;

  // =========================================================================
  // 3. Helper Functions
  // =========================================================================
  auto resolve_widget(int res_key, bool ingame_map) -> cgwnd*;
  auto hide_widget(int res_key, bool ingame_map) -> void;
  auto show_widget(int res_key, bool ingame_map) -> void;
  auto add_hidden_widget(int res_key, bool ingame_map, const char* label) -> bool;
  auto remove_hidden_widget(int res_key, bool ingame_map) -> bool;
  auto tick_hides() -> void;

  // =========================================================================
  // 4. Named Event Handlers
  // =========================================================================
  auto handle_tick(void* raw_ctx) -> void;
  auto handle_menu(void* raw_ctx) -> void;

} // namespace ext_client::plugins::hud_customizer
