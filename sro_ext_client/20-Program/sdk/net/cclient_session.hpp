#pragma once

#include "utils/offsets.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CClientSession — Client network session state (global @ 0x014203FC)
// Holds connection state, connection ID, handshake tokens.
// ---------------------------------------------------------------------------
class cclient_session {
public:
  // 1. Session State Accessors
  [[nodiscard]] auto is_connected() const -> bool {
    return ext_client::off::field_at<std::uint32_t>(this, 0x00) != 0;
  }

  [[nodiscard]] auto connection_id() const -> std::uint32_t {
    return ext_client::off::field_at<std::uint32_t>(this, 0x04);
  }

  [[nodiscard]] auto locale_byte() const -> std::uint32_t {
    return ext_client::off::field_at<std::uint32_t>(this, 0x08);
  }

  [[nodiscard]] auto security_token() const -> std::uint32_t {
    return ext_client::off::field_at<std::uint32_t>(this, 0x0C);
  }

  auto set_connected(bool val) -> void {
    ext_client::off::field_at<std::uint32_t>(this, 0x00) = val ? 1 : 0;
  }
};
