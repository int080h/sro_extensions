#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ext_client::net_manager {
  struct entry;
}

namespace ext_client::menu::packet_decoder {

  enum class field_type : std::uint8_t {
    u8,
    u16,
    u32,
    u24,
    f32,
    bytes,
    unknown,
  };

  struct decoded_field {
    std::size_t offset = 0;
    field_type type = field_type::unknown;
    const char* name = "";
    std::string formatted_value;
  };

  struct decode_result {
    const char* opcode_name = nullptr;
    std::vector<decoded_field> fields;
  };

  auto decode(const ext_client::net_manager::entry& entry) -> decode_result;
  auto opcode_name(std::uint16_t opcode) -> const char*;

} // namespace ext_client::menu::packet_decoder
