#pragma once

#include "sdk/cmsg_stream_buffer.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// CMsg — dual-role network message class.
//
// Building mode: construct locally with an opcode, write payload via << operators,
// call send(). Internally wraps a cmsg_stream_buffer and calls the game's
// send_from_buffer (0x00941600), exactly as the game does.
//
// Reading mode: when this points to the binary's internal wire CMsg (0x1068+ bytes,
// pool-allocated), the reading methods use raw offset arithmetic to extract opcode,
// payload, etc. This is used by net_log_plugin to inspect incoming packets.
class cmsg {
public:
  // Building mode
  explicit cmsg(std::uint16_t opcode);
  ~cmsg();
  cmsg(const cmsg&) = delete;
  cmsg& operator=(const cmsg&) = delete;

  auto operator<<(std::uint8_t v) -> cmsg&;
  auto operator<<(std::uint16_t v) -> cmsg&;
  auto operator<<(std::uint32_t v) -> cmsg&;
  auto operator<<(const std::string& v) -> cmsg&;
  auto operator<<(const char* v) -> cmsg&;

  auto send() -> void;

  // Static API (game's send_from_buffer at 0x00941600)
  static auto send_from_buffer(cmsg_stream_buffer* buf) -> void;

  // Reading mode — instance methods using raw offset arithmetic.
  // this = pointer to binary's internal wire CMsg (0x1068+ bytes).
  auto opcode() const -> std::uint16_t;
  auto header_opcode() const -> std::uint16_t;
  auto body_size() const -> std::size_t;
  auto is_encrypted() const -> bool;
  auto read_cursor_pos() const -> std::size_t;
  auto extract_payload() const -> std::vector<std::uint8_t>;
  auto write_payload(const void* src, int size) -> int;

private:
  cmsg_stream_buffer m_stream;
};
