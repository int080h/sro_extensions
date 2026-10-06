#pragma once

#include "sdk/ui/cif_wnd.hpp"

#include <cstdint>

// Reversed from GInterface.cpp / IFTargetWindow (IDA 2015 ISRO).
// Class CIFTargetWindow : public CIFWnd (vt @ 0x10053AC). Size 0x394 (916 bytes).
//
// UI hierarchy:
//   CGWnd_GetManager()     -> dword_13BAEE0 (global widget registry)
//   CGInterface+0x374      -> CResIDManager (sub_9CF790 lookup)
//   CGInterface+0x3BC      -> cached target floater (sub_85D600)
//   CGInterface child 0x10 -> CIFTargetWindow root (sub_868030 -> sub_7E6B60)
//   root+0x388             -> CIFTargetWindowSpecialMob (sub_7E8EB0 draws here)
//
// CIFTargetWindow layout:
//   +0x000  CGWnd base (vtable @ 0x10053AC)
//   +0x084  CTextBoard base (vtable @ 0x1005364)
//   +0x1C4  CResIDManager
//   +0x374  target entity slot (std::uint32_t)
//   +0x378  name label  (cif_static*)
//   +0x37C  HP gauge    (cif_gauge*)
//   +0x380  rank label  (cif_static*)
//   +0x388  special mob window (cif_target_window*)

// ---------------------------------------------------------------------------
// CIFTargetWindow — Target mob / character HUD frame (extends CIFWnd)
// Native VTable: 0x010053AC | Class Size: 0x394
// ---------------------------------------------------------------------------
class cif_static;
class cif_gauge;

class cif_target_window : public cif_wnd {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x010053AC;
  static constexpr std::size_t   k_class_size  = 0x0394;

  // 1. Instance Member Accessors
  auto target_slot_id() const -> std::uint32_t;
  auto name_label() -> cif_static*;
  auto hp_gauge() -> cif_gauge*;
  auto rank_label() -> cif_static*;
  auto special_mob_window() -> cif_target_window*;

  // 2. Active Panel Resolution
  static auto is_live_target_panel(const void* panel) -> bool;
  static auto active() -> cif_target_window*;

  // 3. Static Child Accessors
  static auto hp_gauge(void* panel) -> cif_gauge*;
  static auto name_label(void* panel) -> cif_static*;
  static auto rank_label(void* panel) -> cif_static*;
  static auto hp_percent_label(void* wnd) -> cif_static*;
  static auto hp_label(void* special_mob_wnd) -> cif_static*;
  static auto target_slot_id(void* wnd) -> std::uint32_t;

  // 4. Panel Classification
  static auto is_common_enemy(const void* wnd) -> bool;
  static auto is_special_mob(const void* wnd) -> bool;
  static auto is_supported(const void* wnd) -> bool;
};
