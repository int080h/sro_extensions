#include "sdk/cmsg_stream_buffer.hpp"

#include <cstring>

namespace {

  using read_fn = int(__thiscall*)(cmsg_stream_buffer* self, char* dst, int size);
  using write_fn = void*(__thiscall*)(cmsg_stream_buffer* self, const void* src, int size);
  using ctor_fn = cmsg_stream_buffer*(__thiscall*)(cmsg_stream_buffer* self, std::uint16_t opcode);
  using dtor_fn = void(__thiscall*)(cmsg_stream_buffer* self);

  using ext_client::offsets::as_fn;

  auto read_fn_ptr() -> read_fn {
    return as_fn<read_fn>(ext_client::offsets::cmsg_stream_buffer::functions::read_bytes);
  }

  auto write_fn_ptr() -> write_fn {
    return as_fn<write_fn>(ext_client::offsets::cmsg_stream_buffer::functions::write_bytes);
  }

  auto ctor_fn_ptr() -> ctor_fn {
    return as_fn<ctor_fn>(ext_client::offsets::cmsg_stream_buffer::functions::ctor);
  }

  auto dtor_fn_ptr() -> dtor_fn {
    return as_fn<dtor_fn>(ext_client::offsets::cmsg_stream_buffer::functions::dtor);
  }

} // namespace

auto cmsg_stream_buffer::opcode() const -> std::uint16_t {
  return m_msg_id;
}

auto cmsg_stream_buffer::payload_size() const -> std::size_t {
  return m_total_bytes;
}

auto cmsg_stream_buffer::read_cursor_pos() const -> std::size_t {
  return m_read_cursor;
}

auto cmsg_stream_buffer::read_bytes(void* dst, std::size_t size) -> std::size_t {
  if (size == 0) {
    return 0;
  }
  const int requested = static_cast<int>(size);
  const int consumed = read_fn_ptr()(const_cast<cmsg_stream_buffer*>(this), static_cast<char*>(dst), requested);
  return consumed > 0 ? static_cast<std::size_t>(consumed) : 0;
}

auto cmsg_stream_buffer::write_bytes(const void* src, std::size_t size) -> void {
  if (size == 0) {
    return;
  }
  write_fn_ptr()(this, src, static_cast<int>(size));
}

auto cmsg_stream_buffer::extract_payload() const -> std::vector<std::uint8_t> {
  if (m_total_bytes == 0) {
    return {};
  }

  std::vector<std::uint8_t> payload;
  payload.reserve(m_total_bytes);

  struct stream_node {
    stream_node* next;
    std::uint8_t data[4096];
  };

  stream_node* current = static_cast<stream_node*>(m_node_primary);
  std::size_t remaining = m_total_bytes;

  while (current && remaining > 0) {
    std::size_t chunk_size = (remaining > 4096) ? 4096 : remaining;
    payload.insert(payload.end(), current->data, current->data + chunk_size);
    remaining -= chunk_size;
    current = current->next;
  }

  return payload;
}

auto cmsg_stream_buffer::replace_payload(const std::uint8_t* bytes, std::size_t size) -> bool {
  toggle_before();
  if (bytes && size > 0) {
    write_bytes(bytes, size);
  }
  toggle_after();
  return true;
}

auto cmsg_stream_buffer::construct(cmsg_stream_buffer* buf, std::uint16_t opcode) -> cmsg_stream_buffer* {
  if (!buf) {
    return nullptr;
  }
  return ctor_fn_ptr()(buf, opcode);
}

auto cmsg_stream_buffer::destroy() -> void {
  dtor_fn_ptr()(this);
}

auto cmsg_stream_buffer::toggle_before() -> void {
  if (m_mode_flag == 1) {
    return;
  }
  m_mode_flag = 1;
  m_node_active = m_node_primary;
  m_total_bytes = 0;
}

auto cmsg_stream_buffer::toggle_after() -> void {
  if (m_mode_flag == 0) {
    return;
  }
  m_mode_flag = 0;
  m_node_active = m_node_primary;
  m_read_cursor = 0;
}

auto cmsg_stream_buffer::flush_remaining() -> void {
  m_read_cursor = m_total_bytes;
}

auto cmsg_stream_buffer::read_u8(std::uint8_t& value) -> cmsg_stream_buffer& {
  read_bytes(&value, sizeof(value));
  return *this;
}

auto cmsg_stream_buffer::read_u16(std::uint16_t& value) -> cmsg_stream_buffer& {
  read_bytes(&value, sizeof(value));
  return *this;
}

auto cmsg_stream_buffer::read_u32(std::uint32_t& value) -> cmsg_stream_buffer& {
  read_bytes(&value, sizeof(value));
  return *this;
}

auto cmsg_stream_buffer::read_string(std::string& value) -> cmsg_stream_buffer& {
  std::uint16_t length = 0;
  read_u16(length);
  if (length == 0) {
    value.clear();
    return *this;
  }
  value.resize(length);
  read_bytes(value.data(), length);
  return *this;
}

auto cmsg_stream_buffer::write_u8(std::uint8_t value) -> cmsg_stream_buffer& {
  write_bytes(&value, sizeof(value));
  return *this;
}

auto cmsg_stream_buffer::write_u16(std::uint16_t value) -> cmsg_stream_buffer& {
  write_bytes(&value, sizeof(value));
  return *this;
}

auto cmsg_stream_buffer::write_u32(std::uint32_t value) -> cmsg_stream_buffer& {
  write_bytes(&value, sizeof(value));
  return *this;
}

auto cmsg_stream_buffer::write_string(const std::string& value) -> cmsg_stream_buffer& {
  const auto length = static_cast<std::uint16_t>(value.size());
  write_u16(length);
  if (!value.empty()) {
    write_bytes(value.data(), value.size());
  }
  return *this;
}
