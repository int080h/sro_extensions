#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "plugins/net_log/packet_field_type.hpp"

namespace ext_client::plugins::net_log {

struct parsed_field {
  std::string name;
  pkt::field_type type{pkt::field_type::raw};
  std::size_t offset = 0;
  std::size_t size = 0;
  std::string value;
  std::string description;
  int indent = 0;
};

struct parse_result {
  std::vector<parsed_field> fields;
  bool success = false;
  std::string error;
  std::size_t bytes_consumed = 0;
  const char* doc_summary = nullptr;
  const char* parser_method = nullptr;
};

auto parse_packet(std::uint16_t opcode, const std::uint8_t* data, std::size_t size) -> parse_result;

} // namespace ext_client::plugins::net_log
