#pragma once

#include "ext_client.hpp"
#include "sdk/cmsg.hpp"
#include "sdk/cmsg_stream_buffer.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Extension networking — single module for hooks, handlers, logging, codec, send, overrides.
//
// Two game layers:
//   CMsg            — wire/socket message (IBSMsg pool, security header)
//   CMsgStreamBuffer — logical game payload read/write view
//
// Hooks (5):
//   stream S->C  from_wire           recv_pump ingress
//   stream S->C  stream_dispatch     block/consume only (logged at from_wire)
//   stream C->S  send_from_buffer    SendMsg entry (sync + deferred queue)
//   CMsg   S->C  cmsg_dispatch       recv_assemble -> session+0x22C (0xA101, handshake, ...)
//   CMsg   C->S  send_pool_buffer    CNetEngine vtable+0x50 (all IBSMsg egress)

namespace ext_client::hooks::net {

  // -------------------------------------------------------------------------
  // Types (moved out of sdk/ — sdk is game-class-only)
  // -------------------------------------------------------------------------

  enum class packet_direction : std::uint8_t {
    client_to_server = 0,
    server_to_client = 1,
  };

  enum class packet_layer : std::uint8_t {
    cmsg = 0,
    stream = 1,
  };

  enum class handler_result : std::uint8_t {
    pass,
    consume,
  };

  enum class opcode_block_mode : std::uint8_t {
    off = 0,
    single = 1,
    list = 2,
  };

  struct packet_view {
    cmsg_stream_buffer* msg = nullptr;
    packet_direction direction = packet_direction::client_to_server;
    std::uint16_t opcode = 0;
  };

  using handler_fn = std::function<handler_result(packet_view& ctx)>;

  struct handler_token {
    std::uint32_t id = 0;
    auto valid() const -> bool { return id != 0; }
  };

  struct entry {
    std::uint32_t id = 0;
    std::uint32_t tick = 0;
    std::uint64_t timestamp_ms = 0;
    packet_direction direction = packet_direction::server_to_client;
    packet_layer layer = packet_layer::stream;
    std::uint16_t opcode = 0;
    std::vector<std::uint8_t> payload;
    const char* capture_point = nullptr;
    bool blocked = false;
    bool modified = false;
  };

  // -------------------------------------------------------------------------
  // Config
  // -------------------------------------------------------------------------

  using control = ext_client::config::settings::net_config_t;

  auto control_panel() -> control&;

  // -------------------------------------------------------------------------
  // Hook install / uninstall
  // -------------------------------------------------------------------------

  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;

  // -------------------------------------------------------------------------
  // Handler registration (thread-safe)
  // -------------------------------------------------------------------------

  auto on_incoming(std::uint16_t opcode, handler_fn handler) -> handler_token;
  auto on_outgoing(std::uint16_t opcode, handler_fn handler) -> handler_token;
  auto on_incoming_any(handler_fn handler) -> handler_token;
  auto on_outgoing_any(handler_fn handler) -> handler_token;
  auto remove(handler_token token) -> void;
  auto clear_handlers() -> void;
  auto dispatch_incoming(cmsg_stream_buffer* msg) -> handler_result;
  auto dispatch_outgoing(cmsg_stream_buffer* msg) -> handler_result;

  // -------------------------------------------------------------------------
  // Send (net_msg is internal)
  // -------------------------------------------------------------------------

  auto send(std::uint16_t opcode) -> void;
  auto send(std::uint16_t opcode, const std::uint8_t* data, std::size_t size) -> void;
  auto send(std::uint16_t opcode, const std::vector<std::uint8_t>& payload) -> void;

  // -------------------------------------------------------------------------
  // Recording (called from hook detours)
  // -------------------------------------------------------------------------

  auto record_outgoing(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;
  auto record_incoming(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;
  auto record_cmsg_outgoing(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;
  auto record_cmsg_incoming(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;

  // -------------------------------------------------------------------------
  // Log management
  // -------------------------------------------------------------------------

  auto clear_log() -> void;
  auto count() -> std::size_t;
  auto visit_entries(const std::function<void(const std::vector<entry>&)>& visitor) -> void;
  auto snapshot() -> std::vector<entry>;
  auto flush_log() -> void;

  // -------------------------------------------------------------------------
  // Codec utilities
  // -------------------------------------------------------------------------

  auto format_opcode(std::uint16_t opcode) -> const char*;
  auto format_layer(packet_layer layer) -> const char*;
  auto format_hex(const std::vector<std::uint8_t>& bytes, std::size_t bytes_per_row = 16) -> std::string;
  auto parse_hex(const char* text, std::vector<std::uint8_t>& out) -> bool;
  auto parse_opcode_list(const char* text, std::vector<std::uint16_t>& out) -> bool;
  auto opcode_in_list(std::uint16_t opcode, const char* list_text) -> bool;

  // -------------------------------------------------------------------------
  // Block logic
  // -------------------------------------------------------------------------

  auto should_block_opcode(std::uint16_t opcode) -> bool;

  // -------------------------------------------------------------------------
  // Overrides
  // -------------------------------------------------------------------------

  auto set_outgoing_override(std::uint16_t opcode, const std::uint8_t* payload, std::size_t size, bool apply_to_all_matching) -> void;
  auto set_incoming_override(std::uint16_t opcode, const std::uint8_t* payload, std::size_t size, bool apply_to_all_matching) -> void;
  auto clear_overrides() -> void;

  // -------------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------------

  auto register_handlers() -> void;
  auto shutdown() -> void;

} // namespace ext_client::hooks::net
