#pragma once

#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// CIRMManager — global Interface Resource Manager singleton.
//
// Reversed from isro_client.exe:
//   vtable:  0x0102CB9C
//   global:  0x0117ED1C  (g_CIRMManager, constructed by sub_FB3770 via atexit)
//   ctor:    0x00925760  (sets vtable, calls sub_9241F0 on this+4)
//   dtor:    0x00925A70  (clears internal map + vector)
//   parse:   0x00925B00  (loads + parses .txt file, returns parsed document*)
//
// CIRMManager is NOT CObj-derived (vtable[0] is the scalar deleting destructor,
// not GetRuntimeClass). It is a standalone class with a global singleton.
//
// Layout (0x2C bytes):
//   +0x00: vftable pointer (4 bytes)
//   +0x04: stdext::hash_map<std::string, Section*> m_section_map (0x28 bytes)
//
// The hash_map at +0x04 is the MSVC9 stdext::hash_map which contains:
//   +0x04: internal field (4 bytes)
//   +0x08: std::list<pair<string, Section*>> _List (12 bytes: alloc + sentinel + size)
//   +0x14: std::vector<list_iterator> _Vec   (16 bytes: alloc + first + last + end)
//   +0x24: _Mask   (4 bytes)
//   +0x28: _Maxidx (4 bytes)
//
// CResIDManager::load_from_file (sub_9CF640) calls CIRMManager::load_and_parse_file
// internally to parse .txt files, then stores the result at CResIDManager+0x0C.

using section_map_t = std::n_hash_map<std::n_string, void*>;

class cirm_manager {
public:
  static auto get() -> cirm_manager*;
  static auto is_instance(const void* ptr) -> bool;

  auto raw() const -> const void* { return this; }

  // Load and parse a resinfo .txt file. Returns a pointer to the parsed
  // document (stdext::hash_map<string, Section>), or nullptr on failure.
  // The parsed document is cached internally by CIRMManager.
  auto load_and_parse_file(const char* filename) -> void*;

  // Read-only view of the internal stdext::hash_map<string, Section*>.
  auto section_map() const -> ext_client::msvc9::stdext_hash_map_ref;

  void* vftable;                                          // +0x00
  union {
    DEFINE_MEMBER_0(section_map_t m_section_map, "section map"); // +0x04
  };
};

