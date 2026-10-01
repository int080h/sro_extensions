#pragma once

#include "sdk/ui/cif_wnd.hpp"
#include "sdk/ui/ctext_board.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

class cif_main_popup;
class calram_guide_mgr_wnd;

// CGInterface — CGWnd prefix + CTextBoard @ +0x84, ui res map @ +0x374.
class cg_interface {
public:
  auto get_textboard() -> ctext_board*;
  auto get_textboard() const -> const ctext_board*;
  auto get_ui_res_map() -> ui_res_map_t*;
  auto get_ui_res_map() const -> const ui_res_map_t*;
  auto get_alarm_guide_mgr_popup() -> void*;
  auto get_alarm_guide_mgr_popup() const -> void*;
  auto get_modal_wnd() -> void*;
  auto get_modal_wnd() const -> void*;
  auto get_alarm_store() -> cif_main_popup*;
  auto get_alarm_store() const -> const cif_main_popup*;
  auto get_alarm_guide_mgr() -> calram_guide_mgr_wnd*;
  auto get_alarm_guide_mgr() const -> const calram_guide_mgr_wnd*;

  static constexpr int k_child_main_hud = 1;
  static constexpr int k_child_alarm_strip = 0x9D;
  static constexpr int k_child_guide_host = 0x9E;
  static constexpr int k_child_alarm_guide_alt = 885;

  auto get_ui_child(int control_id, bool add_base_key = true) -> cgwnd*;
  auto get_guide_host(bool add_base_key = true) -> cgwnd*;
  auto walk_each(int max_depth, cgwnd::child_visitor_fn visit, void* ctx) -> void;
  auto show_magic_lamp_guide(bool show) -> unsigned int;
  auto show_daily_login_guide(bool show) -> unsigned int;
  auto show_facebook_guide(bool show) -> unsigned int;
  auto show_web_item_alarm_guide(bool show) -> unsigned int;
  auto show_macro_guide(bool show) -> unsigned int;

  static auto is_ready() -> bool;
  static auto is_instance(const void* ptr) -> bool;
  static auto is_ingame_hud_ready() -> bool;

  static auto get() -> cg_interface*;
  static auto known_child_id_count() -> std::size_t;
  static auto known_child_id(std::size_t index) -> int;
};
