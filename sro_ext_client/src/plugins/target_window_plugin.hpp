#pragma once

#include <Windows.h>
#include <imgui.h>
#include <cstdio>

#include "core/core_event_manager.hpp"
#include "sdk/cif_target_window.hpp"
#include "sdk/cgwnd.hpp"

namespace ext_client::plugins::target_window {

  // =========================================================================
  // 1. Helper Functions
  // =========================================================================
  auto target_id_active(void* target_id) -> bool;
  auto hp_percent_from_gauge(const cgwnd* gauge) -> int;
  auto draw_outlined_text(ImDrawList* draw_list, ImVec2 pos, ImU32 color, const char* text) -> void;
  auto render_hp_overlay() -> void;

  // =========================================================================
  // 2. Named Event Handlers
  // =========================================================================
  auto handle_populate_target(void* raw_ctx) -> void;
  auto handle_populate_special_mob(void* raw_ctx) -> void;
  auto handle_update_special_mob(void* raw_ctx) -> void;
  auto handle_menu(void* raw_ctx) -> void;

} // namespace ext_client::plugins::target_window
