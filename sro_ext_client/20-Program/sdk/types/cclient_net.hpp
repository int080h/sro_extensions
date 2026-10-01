#pragma once

#include "sdk/net/cnet_engine.hpp"
#include "sdk/types/cclient_session.hpp"

#include <cstdint>

class cmsg;

struct cclient_net {
  auto get_event_sink() -> int;
  auto get_engine() -> cnet_engine;
  auto get_process_registry() -> void*;

  auto set_event_sink(int val) -> void;
  auto set_engine(cnet_engine val) -> void;
  auto set_process_registry(void* val) -> void;

  static auto is_connected() -> bool;

  static auto get_instance() -> cclient_net*;
  static auto get_active_socket() -> void*;
  static auto get_session() -> cclient_session*;
  static auto alloc_msg(std::uint16_t opcode, int pool_id = 0) -> cmsg*;
  static auto free_msg(cmsg* msg) -> int;
  static auto send_msg(cmsg* msg) -> int;
  static auto pump_messages() -> int;
};
