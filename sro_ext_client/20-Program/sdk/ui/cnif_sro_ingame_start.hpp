#pragma once

#include "sdk/ui/cgwnd.hpp"
#include "sdk/types/cnif_sro_ingame_start_live.hpp"
#include "sdk/types/ingame_res_lookup.hpp"
#include "sdk/types/survey_resolve_diag.hpp"

#include <cstdint>

class cnif_sro_ingame_start;

class cnif_sro_ingame_info;

class cg_interface;



// CNIFSroInGameStart — bottom-bar survey promo (NIFSroInGame.cpp).

class cnif_sro_ingame_start : public cgwnd {

public:

  auto is_show_survey() const -> bool;

  auto get_show_survey() -> std::uint8_t;
  auto get_survey_button() -> cgwnd*;
  auto get_survey_button() const -> const cgwnd*;

  auto set_show_survey(std::uint8_t val) -> void;

  auto hide_start_panel() -> void;
  auto show_start_panel() -> void;
  auto hide_survey_panel() -> void;

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

