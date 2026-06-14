#pragma once

#include "sdk/cmsg_stream_buffer.hpp"

#include <cstddef>
#include <cstdint>

// Two layers:
//   CMsg            — wire/socket message (IBSMsg pool, security header)
//   CMsgStreamBuffer — logical game payload read/write view
//
// Hooks (5):
//   stream S->C  from_wire           recv_pump ingress
//   stream S->C  stream_dispatch     block/consume only (logged at from_wire)
//   stream C->S  send_from_buffer    SendMsg entry (sync + deferred queue)
//   CMsg   S->C  cmsg_dispatch       recv_assemble -> session+0x22C (0xA101, handshake, …)
//   CMsg   C->S  send_pool_buffer    CNetEngine vtable+0x50 (all IBSMsg egress)
#include "config/client_config.hpp"

namespace ext_client::hooks::packet {

  using control = ext_client::config::settings::packet_t;

  auto control_panel() -> control&;
  auto set_outgoing_override(std::uint16_t opcode, const std::uint8_t* payload, std::size_t size, bool apply_to_all_matching) -> void;
  auto set_incoming_override(std::uint16_t opcode, const std::uint8_t* payload, std::size_t size, bool apply_to_all_matching) -> void;
  auto clear_overrides() -> void;

  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;

} // namespace ext_client::hooks::packet
