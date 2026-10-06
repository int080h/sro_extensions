#pragma once

#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

class cgwnd;

// CNInterfaceManager — global interface resource manager (singleton).
//
// Reversed from isro_client.exe:
//   global:  0x01420408  (g_CNInterfaceManager, initialized by sub_401010 via CRT init)
//   init:    0x00401010  (zeroes fields, allocates map sentinels, registers atexit cleanup)
//   find:    0x004016F0  (thiscall; std::map<int, void*> lookup by key → CNIFWnd*)
//   load:    0x00401810  (thiscall; loads .2dt file, creates widgets, inserts into map)
//   prep:    0x00401340  (thiscall; loads panel by key from second map)
//   cleanup: 0x00401210  (atexit; destroys managed widgets and frees maps)
//
// The VSRO counterpart is CNInterfaceManager in NInterfaceResource.h.
// ISRO layout has two std::map<int, void*> containers and two std::string fields.
//
// Layout (0x74 bytes):
//   +0x00: CNIRMManager* m_pResourceMgr        (pointer to vtable'd object, initially NULL)
//   +0x04: std::map<int, void*> m_mapInterface  (sentinel at +0x08, size at +0x0C)
//   +0x10: std::string m_string1                (28 bytes: allocator + buffer ptr + len + cap)
//   +0x2C: int m_unknown34
//   +0x30: bool m_bDiskFilePathLoad
//   +0x31: BYTE m_btDefaultLangId
//   +0x32: BYTE m_pad32
//   +0x33: bool m_flag3B
//   +0x34: int m_uiClientSize_w
//   +0x38: int m_uiClientSize_h
//   +0x3C: std::map<int, void*> m_mapInterface2 (sentinel at +0x40, size at +0x44)
//   +0x48: std::string m_string2                (28 bytes)
//   +0x64: int m_unknown64
//   +0x68: int m_unknown68
//   +0x6C: int m_unknown6C
//   +0x70: int m_unknown70

// ---------------------------------------------------------------------------
// CNInterfaceManager — Global interface resource manager (singleton)
// Singleton: 0x01420408 | Class Size: 0x74
// ---------------------------------------------------------------------------
class cninterface_manager {
public:
  static constexpr std::uint32_t k_singleton_addr = 0x01420408;
  static constexpr std::size_t   k_class_size     = 0x0074;

  using child_visitor_fn = void (*)(cgwnd* child, void* ctx);

  // 1. Singleton Access
  static auto get_instance() -> cninterface_manager*;

  // 2. Widget Lookup & Instantiation
  auto get_interface_obj_raw(int res_id) -> void*;
  auto find(int res_id) -> cgwnd*;
  auto instantiate_dimensional(const char* filename, void* parent, bool b) -> void;

  // 3. Root Widget Traversal
  auto walk_roots(child_visitor_fn visit, void* ctx, int child_depth) -> void;
};
