#include "sdk/cmsg.hpp"

#include "sdk/cmsg_stream_buffer.hpp"

#include "utils/offsets.hpp"

namespace {

  using write_payload_fn = int(__thiscall*)(cmsg* self, void* src, int size);
  using send_from_buffer_fn = void(__cdecl*)(cmsg_stream_buffer* buffer);

  using ext_client::offsets::as_fn;

} // namespace

auto cmsg::opcode() const -> std::uint16_t {
  return m_opcode ? *m_opcode : 0;
}

auto cmsg::header_opcode() const -> std::uint16_t {
  if (!m_data) {
    return 0;
  }
  return *reinterpret_cast<const std::uint16_t*>(static_cast<const std::uint8_t*>(m_data) + 2);
}

auto cmsg::body_size() const -> std::size_t {
  return m_size ? static_cast<std::size_t>(*m_size & 0x7FFFu) : 0;
}

auto cmsg::is_encrypted() const -> bool {
  return m_size && (*m_size & 0x8000u) != 0;
}

auto cmsg::read_cursor_pos() const -> std::size_t {
  return m_read_cursor;
}

auto cmsg::extract_payload() const -> std::vector<std::uint8_t> {
  if (!m_data) {
    return {};
  }

  const auto offset = read_cursor_pos();
  const auto size = body_size();
  if (size == 0) {
    return {};
  }

  const auto* bytes = static_cast<const std::uint8_t*>(m_data);
  return std::vector<std::uint8_t>(bytes + offset, bytes + offset + size);
}

auto cmsg::write_payload(const void* src, int size) -> int {
  if (!src || size <= 0) {
    return 0;
  }
  return as_fn<write_payload_fn>(ext_client::offsets::cmsg::functions::write_payload)(this, const_cast<void*>(src), size);
}

auto cmsg::send_from_buffer(cmsg_stream_buffer* buffer) -> void {
  if (!buffer) {
    return;
  }
  as_fn<send_from_buffer_fn>(ext_client::offsets::cmsg::functions::send_from_buffer)(buffer);
}
