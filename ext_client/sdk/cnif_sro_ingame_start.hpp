#pragma once

#include "cgwnd.hpp"
#include "cif_manager.hpp"

#include <cstdint>

#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

class cnif_sro_ingame_start;
class cnif_sro_ingame_info;
class cg_interface;

struct cnif_sro_ingame_start_live {
  cnif_sro_ingame_start* start_panel = nullptr;
  cnif_sro_ingame_info* info_panel = nullptr;
  cgwnd* survey_button = nullptr;
};

struct survey_resolve_diag {
  ext_client::cif_manager::ingame_res_lookup start_map{};
  ext_client::cif_manager::ingame_res_lookup info_map{};
  cnif_sro_ingame_start_live live{};
  bool ready = false; // start_panel + survey_button resolved
};

// CNIFSroInGameStart_vtable
struct cnif_sro_ingame_start_vtable {
  VFN_CDECL(get_gwnd_manager, void*);
  VFN_CDECL(get_gwnd_manager_alt, void*);
  VFN_THISCALL(scalar_deleting_dtor, void*, cnif_sro_ingame_start* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cnif_sro_ingame_start* self);
  VFN_THISCALL(equals, int, cnif_sro_ingame_start* self, cnif_sro_ingame_start* other);
  VFN_THISCALL(null_05, void, cnif_sro_ingame_start* self);
  VFN_CDECL(create_instance, cnif_sro_ingame_start*);
  VFN_THISCALL(on_create, int, cnif_sro_ingame_start* self, int mode);
  VFN_THISCALL(on_destroy, int, cnif_sro_ingame_start* self, int mode);
  VFN_THISCALL(on_init, int, cnif_sro_ingame_start* self, int mode);
};

// CNIFSroInGameStart — bottom-bar survey promo (NIFSroInGame.cpp).
class cnif_sro_ingame_start : public cgwnd {
public:
  DECLARE_SDK_VTABLE(cnif_sro_ingame_start_vtable, start_vftable)
  DECLARE_SDK_WND_CAST(cnif_sro_ingame_start)

  static auto is_instance(const void* wnd) -> bool;

  auto survey_button() -> cgwnd*;
  auto survey_button() const -> const cgwnd*;
  auto show_survey() const -> bool { return m_show_survey != 0; }

  static auto diagnose(cg_interface* iface) -> survey_resolve_diag;
  static auto resolve_start(cg_interface* iface) -> cnif_sro_ingame_start*;
  static auto resolve_info(cg_interface* iface) -> cnif_sro_ingame_info*;
  static auto resolve(cg_interface* iface) -> cnif_sro_ingame_start*;
  static auto is_child_of_panel(const cgwnd* wnd, int unique_id) -> bool;
  static auto find_live(cg_interface* iface) -> cnif_sro_ingame_start_live;
  static auto hide_survey_button(cg_interface* iface) -> void;
  static auto show_survey_button(cg_interface* iface) -> void;
  static auto hide_panel(cg_interface* iface) -> void;
  static auto set_panel_visible(cg_interface* iface, bool visible) -> void;

  auto hide_start_panel() -> void;
  auto show_start_panel() -> void;
  auto hide_survey_panel() -> void;

private:
  PAD_TO(ext_client::offsets::cgwnd::size, ext_client::offsets::cnif_sro_ingame_start::fields::show_survey);
  std::uint8_t m_show_survey;
};

// CNIFSroInGameInfo — "Take Survey" browser window (map key 0x34), not the HUD button.
class cnif_sro_ingame_info : public cgwnd {
public:
  DECLARE_SDK_WND_CAST(cnif_sro_ingame_info)

  static auto is_instance(const void* wnd) -> bool;

  auto hide_info_panel() -> void;
};
