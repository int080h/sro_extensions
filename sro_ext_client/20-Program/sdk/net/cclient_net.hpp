#pragma once

#include "sdk/net/cclient_session.hpp"

#include <cstdint>

class cmsg;

// ---------------------------------------------------------------------------
// CClientNet — Top-level client networking engine facade
// Native Global: 0x01420400 | Socket: 0x01420404 | Session: 0x014203FC
// Handles message allocation, packet queuing, transmission, and frame pumping.
// ---------------------------------------------------------------------------
class cclient_net {
public:
  static constexpr std::uint32_t k_singleton_addr = 0x01420400;

  // 1. Singleton & Connection State
  static auto get_instance() -> cclient_net*;
  static auto get_active_socket() -> void*;
  static auto get_session() -> cclient_session*;
  static auto is_connected() -> bool;

  // 2. Packet Memory & Message Lifecycle
  static auto alloc_msg(std::uint16_t opcode, int pool_id = 0) -> cmsg*;
  static auto free_msg(cmsg* msg) -> int;

  // 3. Transmission & Engine Pump
  static auto send_msg(cmsg* msg) -> int;
  static auto pump_messages() -> int;
};
