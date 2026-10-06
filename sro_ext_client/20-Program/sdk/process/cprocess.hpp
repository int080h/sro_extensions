#pragma once

#include "sdk/ui/cgwnd.hpp"
#include "sdk/process/cprocess_msg.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CProcess — Base game process / state machine class
// Native VTable: 0x01068A2C (40 slots) | Extends CGWnd (+0x84..+0xAF)
// Base for all game states: Title, VersionCheck, CharacterSelect, Silkroad.
// ---------------------------------------------------------------------------
using thread_map_t = ext_client::msvc9::n_map<void*, void*>;
using msg_queue_set_t = ext_client::msvc9::n_set<void*>;

class cprocess : public cgwnd {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01068A2C;
  static constexpr std::size_t   vtable_slots  = 40;

  // 1. Thread & Queue Accessors
  auto get_thread_map() -> thread_map_t&;
  auto get_msg_queue() -> msg_queue_set_t&;
  auto get_load_thread() -> void*;
  auto set_load_thread(void* val) -> void;

  // 2. Network State
  auto get_net_state() const -> int;
  auto set_net_state(int val) -> void;
};
