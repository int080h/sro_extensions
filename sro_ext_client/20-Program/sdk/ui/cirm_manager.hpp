#pragma once

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

// ---------------------------------------------------------------------------
// CIRMManager — Interface Resource Manager singleton
// Native VTable: 0x0102CB9C | Singleton: 0x0117ED1C | Class Size: 0x2C
// ---------------------------------------------------------------------------
using section_map_t = std::n_hash_map<std::n_string, void*>;

class cirm_manager {
public:
  static constexpr std::uint32_t k_vtable_addr    = 0x0102CB9C;
  static constexpr std::uint32_t k_singleton_addr = 0x0117ED1C;
  static constexpr std::size_t   k_class_size     = 0x002C;

  // 1. Singleton & Instance Queries
  static auto get() -> cirm_manager*;
  static auto is_instance(const void* ptr) -> bool;

  // 2. Resource Document Parsing & Sections
  auto get_section_map_ref() const -> ext_client::msvc9::stdext_hash_map_ref;
  auto load_and_parse_file(const char* filename) -> void*;

  // 3. Raw Instance Access
  auto get_raw() const -> const void*;
};
