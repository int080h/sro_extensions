#pragma once

#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class packet_direction : std::uint8_t {
  client_to_server = 0,
  server_to_client = 1,
};

class cmsg_stream_buffer;

// CMsgStreamBuffer — small (28-byte) read/write buffer for message payloads.
// Not CMsg: no wire header, no socket ownership. Game code fills or drains this
// while building or parsing a logical packet body; CMsg carries it on the wire.
struct cmsg_stream_buffer_vtable {
  VFN_THISCALL(scalar_deleting_dtor, void*, cmsg_stream_buffer* self, char should_free);
};

class cmsg_stream_buffer {
public:
  static auto construct(cmsg_stream_buffer* buf, std::uint16_t opcode) -> cmsg_stream_buffer*;

  auto destroy() -> void;
  auto opcode() const -> std::uint16_t;
  auto payload_size() const -> std::size_t;
  auto read_cursor_pos() const -> std::size_t;
  auto extract_payload() const -> std::vector<std::uint8_t>;
  auto replace_payload(const std::uint8_t* bytes, std::size_t size) -> bool;

  auto read_bytes(void* dst, std::size_t size) -> std::size_t;
  auto write_bytes(const void* src, std::size_t size) -> void;

  auto toggle_before() -> void;
  auto toggle_after() -> void;
  auto flush_remaining() -> void;

  auto read_u8(std::uint8_t& value) -> cmsg_stream_buffer&;
  auto read_u16(std::uint16_t& value) -> cmsg_stream_buffer&;
  auto read_u32(std::uint32_t& value) -> cmsg_stream_buffer&;
  auto read_string(std::string& value) -> cmsg_stream_buffer&;

  auto write_u8(std::uint8_t value) -> cmsg_stream_buffer&;
  auto write_u16(std::uint16_t value) -> cmsg_stream_buffer&;
  auto write_u32(std::uint32_t value) -> cmsg_stream_buffer&;
  auto write_string(const std::string& value) -> cmsg_stream_buffer&;

private:
  cmsg_stream_buffer_vtable* vftable;
  std::uint32_t m_read_cursor;
  std::uint32_t m_total_bytes;
  std::uint8_t m_mode_flag;
  PAD(0x3);
  void* m_node_primary;
  void* m_node_active;
  std::uint16_t m_msg_id;
  PAD(0x2);

  static inline auto check_layout() -> void {
    static_assert(sizeof(cmsg_stream_buffer) == ext_client::offsets::cmsg_stream_buffer::object_size, "cmsg_stream_buffer size mismatch");
    static_assert(offsetof(cmsg_stream_buffer, m_read_cursor) == ext_client::offsets::cmsg_stream_buffer::fields::read_cursor,
                  "cmsg_stream_buffer::m_read_cursor offset mismatch");
    static_assert(offsetof(cmsg_stream_buffer, m_msg_id) == ext_client::offsets::cmsg_stream_buffer::fields::msg_id,
                  "cmsg_stream_buffer::m_msg_id offset mismatch");
  }
};
