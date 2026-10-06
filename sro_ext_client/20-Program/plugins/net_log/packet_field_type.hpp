#pragma once

#include <cstdint>

namespace ext_client::plugins::net_log::pkt {

  enum class field_type : std::uint8_t {
    u8,
    u16,
    u32,
    u64,
    i8,
    i16,
    i32,
    i64,
    f32,
    bool_,
    ascii,
    raw,
    loop,
    branch,
  };

} // namespace ext_client::plugins::net_log::pkt
