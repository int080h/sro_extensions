#pragma once

#include "sdk/ui/cif_wnd.hpp"
#include "sdk/ui/ctext_board.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

class cif_main_popup;
class calram_guide_mgr_wnd;
class cif_target_window;
class ci_charactor;

// ---------------------------------------------------------------------------
// CGInterface — Main UI interface manager and root container
// Native VTable: 0x0101011C | Singleton: 0x013BAE3C
// ---------------------------------------------------------------------------
class cg_interface {
public:
  static constexpr std::uint32_t k_vtable_addr    = 0x0101011C;
  static constexpr std::uint32_t k_singleton_addr = 0x013BAE3C;

  // Known Control IDs
  static constexpr int k_child_main_hud       = 1;
  static constexpr int k_child_alarm_strip    = 0x9D;
  static constexpr int k_child_guide_host     = 0x9E;
  static constexpr int k_child_alarm_guide_alt = 885;

  // 1. Singleton & Instance Queries
  static auto get() -> cg_interface*;
  static auto is_ready() -> bool;
  static auto is_instance(const void* ptr) -> bool;
  static auto is_ingame_hud_ready() -> bool;

  // 2. Child Lookup & Hierarchy Navigation
  auto get_textboard() -> ctext_board*;
  auto get_textboard() const -> const ctext_board*;
  auto get_ui_res_map() -> ui_res_map_t*;
  auto get_ui_res_map() const -> const ui_res_map_t*;
  auto get_ui_child(int control_id, bool add_base_key = true) -> cgwnd*;
  auto get_guide_host(bool add_base_key = true) -> cgwnd*;
  auto walk_each(int max_depth, cgwnd::child_visitor_fn visit, void* ctx) -> void;
  static auto known_child_id_count() -> std::size_t;
  static auto known_child_id(std::size_t index) -> int;

  // 3. Modals & Alarm Sub-Panels
  auto get_alarm_guide_mgr_popup() -> void*;
  auto get_alarm_guide_mgr_popup() const -> void*;
  auto get_modal_wnd() -> void*;
  auto get_modal_wnd() const -> void*;
  auto get_alarm_store() -> cif_main_popup*;
  auto get_alarm_store() const -> const cif_main_popup*;
  auto get_alarm_guide_mgr() -> calram_guide_mgr_wnd*;
  auto get_alarm_guide_mgr() const -> const calram_guide_mgr_wnd*;

  // 4. Promo / Guide Management
  auto show_magic_lamp_guide(bool show) -> unsigned int;
  auto show_daily_login_guide(bool show) -> unsigned int;
  auto show_facebook_guide(bool show) -> unsigned int;
  auto show_web_item_alarm_guide(bool show) -> unsigned int;
  auto show_macro_guide(bool show) -> unsigned int;

  // 5. Target Window & Selection (Reversed from native engine)
  [[nodiscard]] auto target_window() -> cif_target_window*;
  [[nodiscard]] auto target_window() const -> const cif_target_window*;
  [[nodiscard]] auto has_target() const -> bool;
  [[nodiscard]] auto target_entity() const -> ci_charactor*;
};
