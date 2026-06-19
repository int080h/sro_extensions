#pragma once

#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

struct cprocess;
struct cprocess_manager;

// CProcessManager (game class: CControler) — global instance at 0x013BAE00.
// Manages the active game process child (screen): CPSVersionCheck, CPSTitle,
// CPSCharacterSelect, CPSSilkroad (in-game), etc.
//
// The game's class is named CControler (RTTI: .?AVCControler@@, vtable at 0x10688DC).
// We call it cprocess_manager for clarity. The vtable has 7 entries but we don't
// need them — we only access m_process_child directly.
//
// Layout confirmed from IDA disassembly:
//   +0x000: vtable pointer (CControler::vftable @ 0x10688DC)
//   +0x024: m_pScriptOuterParent (CGWnd* for /script outer window)
//   +0x09C: m_blRun (bool — inside RunProcess, DWORD index 0x27)
//   +0x0A0: m_pProcessChild (cprocess* — active screen, [esi+0A0h] in CProcess_SetChildProcess)
//   Object size: 0x25C (from new(0x25C) in constructor)
struct cprocess_manager {
  void* vftable;                    // +0x000

  PAD(0x0A0 - sizeof(void*));       // +0x004..+0x09F
  cprocess* m_process_child;        // +0x0A0 — active screen (cps_title, cps_character_select, cps_silkroad, etc.)

  // ---------------------------------------------------------------------------

  // Get the global CProcessManager instance, or nullptr if not initialized.
  static auto get() -> cprocess_manager*;

  // Get the active process child (current screen), or nullptr.
  static auto active_child() -> cprocess*;

  // Get the vtable address of the active process child, or 0.
  static auto active_child_vftable() -> std::uint32_t;

  // True when the active screen is the in-game world (CPSSilkroad),
  // i.e. past the title/character-select/loading screens.
  static auto is_ingame() -> bool;

  // True when the active screen is the title/login screen (CPSTitle).
  static auto is_title() -> bool;

  // True when the active screen is the character select screen (CPSCharacterSelect).
  static auto is_character_select() -> bool;

  // True when the active screen is the version check / loading screen (CPSVersionCheck).
  static auto is_version_check() -> bool;
};
