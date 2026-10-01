#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ext_client::plugins::net_log {

struct session_packet {
  std::uint32_t id;
  std::uint32_t tick;
  std::uint64_t timestamp_ms;
  std::uint8_t direction;     // 0=s2c, 1=c2s
  std::uint8_t layer;         // 0=cmsg, 1=stream
  std::uint16_t opcode;
  std::uint16_t payload_size;
  bool has_wire_header;
  std::uint16_t header_size_raw;
  std::uint16_t header_payload_size;
  bool massive;
  std::uint8_t security_count;
  std::uint8_t security_crc;
  bool blocked;
  bool modified;
  std::string capture_point;
  std::vector<std::uint8_t> payload;
};

auto save_session(const std::string& path, const std::vector<session_packet>& packets) -> bool;
auto load_session(const std::string& path, std::vector<session_packet>& packets) -> bool;

} // namespace ext_client::plugins::net_log
