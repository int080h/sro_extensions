#pragma once

#include "cgwnd.hpp"

#include <cstdint>

// Reversed from GInterface.cpp / IFTargetWindow (IDA 2015 ISRO).
//
// UI hierarchy:
//   CGWnd_GetManager()     -> dword_13BAEE0 (global widget registry)
//   CGInterface+0x374      -> CResIDManager (sub_9CF790 lookup)
//   CGInterface+0x3BC        -> cached target floater (sub_85D600)
//   CGInterface child 0x10   -> CIFTargetWindow root (sub_868030 -> sub_7E6B60)
//   root+0x388               -> CIFTargetWindowSpecialMob (sub_7E8EB0 draws here)
//
// CIFTargetWindowSpecialMob layout (sub_7E8EB0 this):
//   +0x374  target entity slot
//   +0x378  name label  (CIFStatic)
//   +0x37C  HP gauge     (fill ratio @ gauge+920)
//   +0x380  rank label   (CIFStatic — shows "General", "Champion", etc.)

class cif_static;

class cif_target_window {
public:
  static auto note_active_panel(void* panel) -> void;
  static auto clear_active_panel() -> void;

  static auto is_live_target_panel(void* panel) -> bool;
  static auto active() -> void*;
  static auto hp_gauge(void* panel) -> cgwnd*;
  static auto name_label(void* panel) -> cif_static*;
  static auto rank_label(void* panel) -> cif_static*;

  static auto is_common_enemy(const void* wnd) -> bool;
  static auto is_special_mob(const void* wnd) -> bool;
  static auto is_supported(const void* wnd) -> bool;
  static auto target_slot_id(void* wnd) -> std::uint32_t;

  static auto hp_percent_label(void* wnd) -> cif_static*;
  static auto as_special_mob(void* wnd) -> void*;
  static auto hp_label(void* special_mob_wnd) -> cif_static*;
};
