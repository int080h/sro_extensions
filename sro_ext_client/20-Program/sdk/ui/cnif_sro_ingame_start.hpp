#pragma once

#include "sdk/ui/cgwnd.hpp"

#include <cstdint>

class cnif_sro_ingame_start;
class cnif_sro_ingame_info;
class cg_interface;

// ---------------------------------------------------------------------------
// Ingame Start Diagnostics & Lookup Result Types
// ---------------------------------------------------------------------------
struct cnif_sro_ingame_start_live {
  cnif_sro_ingame_start* start_panel = nullptr;
  cnif_sro_ingame_info* info_panel = nullptr;
  cgwnd* survey_button = nullptr;
};

struct ingame_res_lookup {
  int res_key = 0;
  bool map_readable = false;
  void* raw = nullptr;
  bool found = false;
  std::uint32_t vftable = 0;
  bool live = false;
  cgwnd* wnd = nullptr;
};

struct survey_resolve_diag {
  ingame_res_lookup start_map{};
  ingame_res_lookup info_map{};
  cnif_sro_ingame_start_live live{};
  bool ready = false;
};

// ---------------------------------------------------------------------------
// CNIFSroInGameStart — Bottom-bar survey promo widget (NIFSroInGame.cpp)
// ---------------------------------------------------------------------------
class cnif_sro_ingame_start : public cgwnd {
public:
  // 1. Survey Button & State Accessors
  auto is_show_survey() const -> bool;
  auto get_show_survey() -> std::uint8_t;
  auto get_survey_button() -> cgwnd*;
  auto get_survey_button() const -> const cgwnd*;
  auto set_show_survey(std::uint8_t val) -> void;

  // 2. Panel Controls
  auto hide_start_panel() -> void;
  auto show_start_panel() -> void;
  auto hide_survey_panel() -> void;

  // 3. Static Resolution & Diagnostics
  static auto is_child_of_panel(const cgwnd* wnd, int unique_id) -> bool;
  static auto diagnose(cg_interface* iface) -> survey_resolve_diag;
  static auto resolve_start(cg_interface* iface) -> cnif_sro_ingame_start*;
  static auto resolve_info(cg_interface* iface) -> cnif_sro_ingame_info*;
  static auto resolve(cg_interface* iface) -> cnif_sro_ingame_start*;
  static auto find_live(cg_interface* iface) -> cnif_sro_ingame_start_live;
  static auto hide_survey_button(cg_interface* iface) -> void;
  static auto show_survey_button(cg_interface* iface) -> void;
  static auto hide_panel(cg_interface* iface) -> void;
  static auto set_panel_visible(cg_interface* iface, bool visible) -> void;
};
