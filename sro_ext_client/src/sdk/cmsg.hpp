#pragma once

#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

class cmsg_stream_buffer;

// CMsg — the Silkroad network message object (BSNet / socket layer).
// Large heap allocation; carries MsgID, wire header, security metadata, and body bytes.
// Outgoing app data is assembled into a CMsgStreamBuffer first, then copied into CMsg
// via WritePayload / SendMsg before hitting the socket.
class cmsg {
public:
  static auto send_from_buffer(cmsg_stream_buffer* buffer) -> void;

  auto opcode() const -> std::uint16_t;
  auto header_opcode() const -> std::uint16_t;
  auto body_size() const -> std::size_t;
  auto is_encrypted() const -> bool;
  auto read_cursor_pos() const -> std::size_t;
  auto extract_payload() const -> std::vector<std::uint8_t>;
  auto write_payload(const void* src, int size) -> int;

private:
  PAD(ext_client::offsets::cmsg::fields::data_ptr);
  void* m_data;
  PAD_TO(ext_client::offsets::cmsg::fields::data_ptr + sizeof(void*), ext_client::offsets::cmsg::fields::read_cursor);
  std::uint16_t m_read_cursor;
  std::uint16_t m_write_cursor;
  PAD_TO(ext_client::offsets::cmsg::fields::write_cursor + sizeof(std::uint16_t), ext_client::offsets::cmsg::fields::ref_count);
  volatile long m_ref_count;
  PAD_TO(ext_client::offsets::cmsg::fields::ref_count + sizeof(long), ext_client::offsets::cmsg::fields::capacity);
  std::uint32_t m_capacity;
  std::uint16_t* m_opcode;
  std::uint16_t* m_size;
  std::uint16_t* m_sec_count;
  std::uint32_t* m_crc;
  std::uint32_t m_recv_handler;

  static inline auto check_layout() -> void {
    static_assert(offsetof(cmsg, m_data) == ext_client::offsets::cmsg::fields::data_ptr, "cmsg::m_data offset mismatch");
    static_assert(offsetof(cmsg, m_read_cursor) == ext_client::offsets::cmsg::fields::read_cursor, "cmsg::m_read_cursor offset mismatch");
    static_assert(offsetof(cmsg, m_write_cursor) == ext_client::offsets::cmsg::fields::write_cursor, "cmsg::m_write_cursor offset mismatch");
    static_assert(offsetof(cmsg, m_ref_count) == ext_client::offsets::cmsg::fields::ref_count, "cmsg::m_ref_count offset mismatch");
    static_assert(offsetof(cmsg, m_capacity) == ext_client::offsets::cmsg::fields::capacity, "cmsg::m_capacity offset mismatch");
    static_assert(offsetof(cmsg, m_opcode) == ext_client::offsets::cmsg::fields::opcode_ptr, "cmsg::m_opcode offset mismatch");
    static_assert(offsetof(cmsg, m_size) == ext_client::offsets::cmsg::fields::size_ptr, "cmsg::m_size offset mismatch");
    static_assert(offsetof(cmsg, m_sec_count) == ext_client::offsets::cmsg::fields::sec_count_ptr, "cmsg::m_sec_count offset mismatch");
    static_assert(offsetof(cmsg, m_crc) == ext_client::offsets::cmsg::fields::crc_ptr, "cmsg::m_crc offset mismatch");
    static_assert(offsetof(cmsg, m_recv_handler) == ext_client::offsets::cmsg::fields::recv_handler, "cmsg::m_recv_handler offset mismatch");
  }
};
