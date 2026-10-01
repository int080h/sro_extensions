#pragma once

#include "core/core_event_manager.hpp"
#include "plugins/net_log/packet_parser.hpp"

#include <cstdint>
#include <vector>

namespace ext_client::plugins::net_log {

  enum class opcode_block_mode : std::uint8_t {
    off = 0,
    single = 1,
    list = 2,
  };

  struct read_chunk {
    std::size_t offset;
    std::vector<std::uint8_t> bytes;
  };

  struct log_entry {
    std::uint32_t id = 0;
    std::uint32_t tick = 0;
    std::uint64_t timestamp_ms = 0;
    ext_client::packet_direction direction = ext_client::packet_direction::server_to_client;
    ext_client::core::event::packet_layer layer = ext_client::core::event::packet_layer::stream;
    std::uint16_t opcode = 0;
    std::vector<std::uint8_t> payload;
    std::uint16_t payload_size = 0;
    bool has_wire_header = false;
    std::uint16_t header_size_raw = 0;
    std::uint16_t header_payload_size = 0;
    bool massive = false;
    std::uint8_t security_count = 0;
    std::uint8_t security_crc = 0;
    const char *capture_point = nullptr;
    bool blocked = false;
    bool modified = false;
    parse_result parsed;
    bool has_parsed = false;
  };

  struct override_rule {
    std::uint16_t opcode = 0;
    bool apply_all = false;
    std::vector<std::uint8_t> payload;
  };

  // Swapping operands keeps equality unordered in both directions.
  inline auto packet_less(const log_entry &a, const log_entry &b, int column, bool ascending) -> bool {
    const auto &lhs = ascending ? a : b;
    const auto &rhs = ascending ? b : a;
    switch (column) {
    case 0:
      return lhs.tick < rhs.tick;
    case 1:
      return lhs.direction < rhs.direction;
    case 2:
      return lhs.layer < rhs.layer;
    case 3:
      return lhs.opcode < rhs.opcode;
    case 4:
      return lhs.payload_size < rhs.payload_size;
    case 5:
      return (static_cast<int>(lhs.massive) | (lhs.blocked << 1) | (lhs.modified << 2)) <
             (static_cast<int>(rhs.massive) | (rhs.blocked << 1) | (rhs.modified << 2));
    default:
      return lhs.id < rhs.id;
    }
  }

  inline constexpr std::size_t k_log_ring_capacity = 2048;
  inline constexpr std::size_t k_max_payload_store = 4096;
  inline constexpr std::size_t k_file_flush_every = 32;

} // namespace ext_client::plugins::net_log
