#pragma once

#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

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
  union {
    DEFINE_MEMBER_0(std::uint32_t m_read_cursor, "read_cursor");
    DEFINE_MEMBER_N(std::uint32_t m_total_bytes, 0x04);
    DEFINE_MEMBER_N(std::uint8_t m_mode_flag, 0x08);
    DEFINE_MEMBER_N(void* m_node_primary, 0x0C);
    DEFINE_MEMBER_N(void* m_node_active, 0x10);
    DEFINE_MEMBER_N(std::uint16_t m_msg_id, 0x14);
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[ext_client::offsets::cmsg_stream_buffer::object_size - sizeof(void*)], "pad_end");
  };

  static inline auto check_layout() -> void {
    static_assert(sizeof(cmsg_stream_buffer) == 28, "cmsg_stream_buffer size must be 28");
    static_assert(offsetof(cmsg_stream_buffer, m_read_cursor) == 4, "m_read_cursor offset must be 4");
    static_assert(offsetof(cmsg_stream_buffer, m_total_bytes) == 8, "m_total_bytes offset must be 8");
    static_assert(offsetof(cmsg_stream_buffer, m_node_primary) == 16, "m_node_primary offset must be 16");
    static_assert(offsetof(cmsg_stream_buffer, m_node_active) == 20, "m_node_active offset must be 20");
    static_assert(offsetof(cmsg_stream_buffer, m_msg_id) == 24, "m_msg_id offset must be 24");
  }
};
