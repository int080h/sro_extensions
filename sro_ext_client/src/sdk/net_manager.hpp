#pragma once

#include "ext_client.hpp"
#include "sdk/cmsg_stream_buffer.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct cmsg;

namespace ext_client::net_manager {

  enum class handler_result : std::uint8_t {
    pass,    // continue to game / CMsg send
    consume, // swallow packet (block send or skip dispatch)
  };

  // Handler context uses CMsgStreamBuffer (payload buffer), not CMsg (wire
  // object).
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

  // Register handlers (thread-safe). Opcode-specific handlers run before wildcard
  // handlers.
  auto on_incoming(std::uint16_t opcode, handler_fn handler) -> handler_token;
  auto on_outgoing(std::uint16_t opcode, handler_fn handler) -> handler_token;
  auto on_incoming_any(handler_fn handler) -> handler_token;
  auto on_outgoing_any(handler_fn handler) -> handler_token;
  auto remove(handler_token token) -> void;
  auto clear_handlers() -> void;
  auto shutdown() -> void;

  // Called from packet hooks — returns consume to block further processing.
  auto dispatch_incoming(cmsg_stream_buffer* msg) -> handler_result;
  auto dispatch_outgoing(cmsg_stream_buffer* msg) -> handler_result;

  // Send a packet through the game's SendMsg (runs outgoing handlers first).
  auto send(std::uint16_t opcode) -> void;
  auto send(std::uint16_t opcode, const std::uint8_t* data, std::size_t size) -> void;
  auto send(std::uint16_t opcode, const std::vector<std::uint8_t>& payload) -> void;

  // Place custom handler registration in net_manager.cpp.
  auto register_handlers() -> void;

  enum class packet_layer : std::uint8_t {
    cmsg = 0,
    stream = 1,
  };

  enum class opcode_block_mode : std::uint8_t {
    off = 0,
    single = 1,
    list = 2,
  };

  struct entry {
    std::uint32_t id = 0;
    std::uint32_t tick = 0;
    std::uint64_t timestamp_ms = 0;
    packet_direction direction = packet_direction::server_to_client;
    packet_layer layer = packet_layer::stream;
    std::uint16_t opcode = 0;
    std::vector<std::uint8_t> payload;
    std::uint16_t payload_size = 0;
    const char* capture_point = nullptr;
    bool blocked = false;
    bool modified = false;
  };

  using control = ext_client::config::settings::net_manager_t;

  auto control_panel() -> control&;
  auto clear_log() -> void;
  auto snapshot() -> std::vector<entry>;
  auto count() -> std::size_t;

  auto should_block_opcode(std::uint16_t opcode) -> bool;

  auto record_outgoing(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;
  auto record_incoming(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;
  auto record_cmsg_outgoing(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;
  auto record_cmsg_incoming(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode = 0, const char* capture_point = nullptr) -> void;

  auto format_opcode(std::uint16_t opcode) -> std::string;
  auto format_layer(packet_layer layer) -> const char*;
  auto format_hex(const std::vector<std::uint8_t>& bytes, std::size_t bytes_per_row = 16) -> std::string;
  auto parse_hex(const char* text, std::vector<std::uint8_t>& out) -> bool;
  auto parse_opcode_list(const char* text, std::vector<std::uint16_t>& out) -> bool;
  auto opcode_in_list(std::uint16_t opcode, const char* list_text) -> bool;

  namespace detail {
    // Set while ext_client::net_manager::send / net_msg::send is on the stack (skips outgoing
    // handlers).
    extern thread_local bool programmatic_send;
  } // namespace detail

} // namespace ext_client::net_manager
