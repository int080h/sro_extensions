#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "packet_field_defs.hpp"

namespace ext_client::plugins::net_log {

struct parsed_field {
  const char* name;
  pkt::field_type type;
  std::size_t offset;
  std::size_t size;
  std::string value;
  int indent;
};

struct parse_result {
  std::vector<parsed_field> fields;
  bool success = false;
  std::string error;
  std::size_t bytes_consumed = 0;
};

auto parse_packet(std::uint16_t opcode, const std::uint8_t* data, std::size_t size) -> parse_result;

} // namespace ext_client::plugins::net_log
