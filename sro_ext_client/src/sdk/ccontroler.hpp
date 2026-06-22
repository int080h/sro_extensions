#pragma once

#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>
#include <cstring>

struct cprocess;

// ccontroler — process management interface for the game.
// The global g_CProcessManager (0x013BAE00) is a CGFXMainFrame* that manages
// the active process child (screen): CPSVersionCheck, CPSTitle,
// CPSCharacterSelect, CPSilkroad (in-game), etc.
//
// The game's CControler class (RTTI: .?AVCControler@@, vtable at 0x10688DC)
// is a separate sub-object at 0x013BAE04, created by the CGFXMainFrame
// constructor.  The active screen pointer lives on CGFXMainFrame at +0x24.
//
// Layout confirmed from IDA disassembly of CGFXMainFrame (sub_D708A0, sub_D6EE60):
//   +0x000: vtable pointer (CGFXMainFrame::vftable @ 0x106886C)
//   +0x024: pProcess (cprocess* — current active screen, read in sub_D6EE60/sub_D6EE00)
//   +0x028: pNextProcess (cprocess* — pending next screen)
//   +0x09C: DWORD (frame timing, read in sub_D6EE60)
//   +0x0A0: float (frame delta time, read in sub_D6EE60)
//
// CProcess_SetChildProcess (sub_D729E0) is __thiscall with this = current
// process child; it writes the new process to this+0xA0 (m_pProcessChild).
// quit_process (sub_D72440) is __cdecl(int factory_entry_ptr); it calls
// CGFXMainFrame::vtable[10] (SetNextProcess) on g_CProcessManager.
struct ccontroler {
  void* vftable;                    // +0x000

  union {
    DEFINE_MEMBER_N(cprocess* m_process, 0x024 - sizeof(void*));
  };

  // ---------------------------------------------------------------------------

  // Get the global CControler instance, or nullptr if not initialized.
  static auto get() -> ccontroler*;

  // Get the active process child (current screen), or nullptr.
  static auto active_child() -> cprocess*;

  // Call get_factory_entry() (vtable slot 0) on the active child to
  // dynamically retrieve its factory entry pointer. Returns nullptr on failure.
  static auto active_child_factory_entry() -> void*;

  // Read the process name string from the active child's factory entry.
  // e.g. "CPSTitle", "CPSCharacterSelect", "CPSilkroad". Returns nullptr on failure.
  static auto active_child_process_name() -> const char*;

  // Call get_factory_entry() (vtable slot 0) on an arbitrary process pointer.
  // Returns nullptr on failure.
  static auto factory_entry(const void* ptr) -> void*;

  // Read the process name string from a process pointer's factory entry.
  // Returns nullptr on failure.
  static auto factory_entry_name(const void* ptr) -> const char*;

  // Check if a process pointer's factory entry name matches expected_name.
  static auto is_process(const void* ptr, const char* expected_name) -> bool;

  // Cast the active child to T* if its factory entry name matches expected_name.
  template<typename T> static auto active_child_as(const char* expected_name) -> T* {
    const char* name = active_child_process_name();
    if (!name || std::strcmp(name, expected_name) != 0) {
      return nullptr;
    }
    return reinterpret_cast<T*>(active_child());
  }

  // Cached active child pointer (updated on SetChildProcess and on successful reads).
  static auto cached_active_child() -> void*&;

  // Check if a child pointer is readable and has a valid-looking vtable.
  static auto is_child_readable(void* child) -> bool;

  // Resolve the active child, falling back to the cached value if the live
  // pointer is null but the cache is still readable.
  static auto resolved_active_child() -> void*;

  // Update the cached active child when SetChildProcess is called.
  static auto note_set_child_process(void* child, int activate) -> void;

  // Call the game's CProcess_SetChildProcess (factory_entry_ptr, activate).
  // this = the current process child (NOT the CGFXMainFrame).
  static auto set_child_process(void* current_process, int factory_entry_ptr, bool activate) -> int;

  // Call the game's quit process function (SetNextProcess on g_CProcessManager).
  // factory_entry_ptr is a pointer to a factory entry (e.g. &dword_117E81C).
  static auto quit_process(int factory_entry_ptr) -> void;
};
