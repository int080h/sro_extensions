#pragma once

#include "sdk/ui/cgwnd.hpp"
#include "sdk/types/cprocess_msg.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

using thread_map = ext_client::msvc9::n_map<void*, void*>;
using msg_queue_set = ext_client::msvc9::n_set<void*>;

// CProcess — extends CGWnd (+0x84..+0xAF), vt @ 0x1068A2C (40 slots).
class cprocess : public cgwnd {
public:
  static constexpr std::size_t vtable_slots = 40;

  auto get_net_state() -> int;
  auto get_thread_map() -> thread_map&;
  auto get_msg_queue() -> msg_queue_set&;
  auto get_load_thread() -> void*;

  auto set_net_state(int val) -> void;
  auto set_load_thread(void* val) -> void;
};
