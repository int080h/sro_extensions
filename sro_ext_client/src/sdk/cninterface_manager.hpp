#pragma once

#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

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

class cninterface_manager {
public:
  static auto instance() -> cninterface_manager*;

  // GetInterfaceObj — lookup a widget by resource ID in the primary map.
  // Returns raw pointer (may be invalid/dead). Use find() for validated access.
  auto get_interface_obj_raw(int res_id) -> void*;

  // GetInterfaceObj — lookup + liveness check. Returns nullptr if not found or dead.
  auto find(int res_id) -> cgwnd*;

  // InstantiateDimensional — load a .2dt file and create widgets under parent.
  // This is the game's sub_401810: loads file, parses, creates CNIFWnd tree,
  // and inserts root widgets into the primary map by their unique IDs.
  auto instantiate_dimensional(const char* filename, void* parent, bool b) -> void;

  // Walk all root widgets in the primary map, visiting each with children.
  using child_visitor_fn = void (*)(cgwnd* child, void* ctx);
  auto walk_roots(child_visitor_fn visit, void* ctx, int child_depth) -> void;

  void* m_pResourceMgr;                                       // +0x00
  std::n_map<std::uint32_t, void*> m_mapInterface;            // +0x04  std::n_map<DWORD, CNIFWnd*>
  std::n_string m_string1;                                    // +0x10
  int m_unknown2C;                                            // +0x2C
  std::uint8_t m_bDiskFilePathLoad;                           // +0x30
  std::uint8_t m_btDefaultLangId;                             // +0x31
  std::uint8_t m_pad32;                                       // +0x32
  std::uint8_t m_flag3B;                                      // +0x33
  int m_uiClientSize_w;                                       // +0x34
  int m_uiClientSize_h;                                       // +0x38
  std::n_map<int, void*> m_mapInterface2;                     // +0x3C  std::map<int, void*>
  std::n_string m_string2;                                    // +0x48
  int m_unknown64;                                            // +0x64
  int m_unknown68;                                            // +0x68
  int m_unknown6C;                                            // +0x6C
  int m_unknown70;                                            // +0x70
};

