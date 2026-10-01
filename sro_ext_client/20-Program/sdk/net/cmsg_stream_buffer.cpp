#include "pch.hpp"
#include "sdk/net/cmsg_stream_buffer.hpp"
#include "utils/offsets.hpp"

#include <cstring>

namespace {

  using read_fn = int(__thiscall *)(cmsg_stream_buffer *self, char *dst, int size);
  using write_fn = void *(__thiscall *)(cmsg_stream_buffer * self, const void *src, int size);
  using ctor_fn = cmsg_stream_buffer *(__thiscall *)(cmsg_stream_buffer * self, std::uint16_t opcode);
  using dtor_fn = void(__thiscall *)(cmsg_stream_buffer *self);

  using ext_client::off::as_fn;

  auto read_fn_ptr() -> read_fn {
    return as_fn<read_fn>(0x00499710);
  }

  auto write_fn_ptr() -> write_fn {
    return as_fn<write_fn>(0x0049B010);
  }

  auto ctor_fn_ptr() -> ctor_fn {
    return as_fn<ctor_fn>(0x0049AFB0);
  }

  auto dtor_fn_ptr() -> dtor_fn {
    return as_fn<dtor_fn>(0x0049B3B0);
  }

} // namespace

auto cmsg_stream_buffer::get_opcode() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x018);
}

auto cmsg_stream_buffer::get_payload_size() const -> std::size_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x008);
}

auto cmsg_stream_buffer::get_read_cursor_pos() const -> std::size_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x004);
}

auto cmsg_stream_buffer::read_bytes(void *dst, std::size_t size) -> std::size_t {
  if (size == 0) {
    return 0;
  }
  const int requested = static_cast<int>(size);
  const int consumed = read_fn_ptr()(const_cast<cmsg_stream_buffer *>(this), static_cast<char *>(dst), requested);
  return consumed > 0 ? static_cast<std::size_t>(consumed) : 0;
}

auto cmsg_stream_buffer::write_bytes(const void *src, std::size_t size) -> void {
  if (size == 0) {
    return;
  }
  write_fn_ptr()(this, src, static_cast<int>(size));
}

auto cmsg_stream_buffer::extract_payload(std::size_t max_bytes) const -> std::vector<std::uint8_t> {
  if (ext_client::off::field_at<std::uint32_t>(this, 0x008) == 0) {
    return {};
  }

  std::vector<std::uint8_t> payload;
  payload.reserve(
      (std::min)(static_cast<std::size_t>(ext_client::off::field_at<std::uint32_t>(this, 0x008)), max_bytes));

  struct stream_node {
    stream_node *next;
    std::uint8_t data[4096];
  };

  stream_node *current = static_cast<stream_node *>(ext_client::off::field_at<void *>(this, 0x010));
  std::size_t remaining =
      (std::min)(static_cast<std::size_t>(ext_client::off::field_at<std::uint32_t>(this, 0x008)), max_bytes);

  while (current && remaining > 0) {
    std::size_t chunk_size = (remaining > 4096) ? 4096 : remaining;
    payload.insert(payload.end(), current->data, current->data + chunk_size);
    remaining -= chunk_size;
    current = current->next;
  }

  return payload;
}

auto cmsg_stream_buffer::replace_payload(const std::uint8_t *bytes, std::size_t size) -> bool {
  toggle_before();
  if (bytes && size > 0) {
    write_bytes(bytes, size);
  }
  toggle_after();
  return true;
}

auto cmsg_stream_buffer::construct(cmsg_stream_buffer *buf, std::uint16_t opcode) -> cmsg_stream_buffer * {
  if (!buf) {
    return nullptr;
  }
  return ctor_fn_ptr()(buf, opcode);
}

auto cmsg_stream_buffer::destroy() -> void {
  dtor_fn_ptr()(this);
}

auto cmsg_stream_buffer::toggle_before() -> void {
  if (ext_client::off::field_at<bool>(this, 0x00C) == 1) {
    return;
  }
  ext_client::off::field_at<bool>(this, 0x00C) = 1;
  ext_client::off::field_at<void *>(this, 0x014) = ext_client::off::field_at<void *>(this, 0x010);
  ext_client::off::field_at<std::uint32_t>(this, 0x008) = 0;
}

auto cmsg_stream_buffer::toggle_after() -> void {
  if (ext_client::off::field_at<bool>(this, 0x00C) == 0) {
    return;
  }
  ext_client::off::field_at<bool>(this, 0x00C) = 0;
  ext_client::off::field_at<void *>(this, 0x014) = ext_client::off::field_at<void *>(this, 0x010);
  ext_client::off::field_at<std::uint32_t>(this, 0x004) = 0;
}

auto cmsg_stream_buffer::flush_remaining() -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x004) = ext_client::off::field_at<std::uint32_t>(this, 0x008);
}

auto cmsg_stream_buffer::read_string(std::string &value) -> cmsg_stream_buffer & {
  std::uint16_t length = 0;
  read(length);
  if (length == 0) {
    value.clear();
    return *this;
  }
  value.resize(length);
  read_bytes(value.data(), length);
  return *this;
}

auto cmsg_stream_buffer::write_string(const std::string &value) -> cmsg_stream_buffer & {
  const auto length = static_cast<std::uint16_t>(value.size());
  write(length);
  if (!value.empty()) {
    write_bytes(value.data(), value.size());
  }
  return *this;
}

auto cmsg_stream_buffer::skip_bytes(const std::size_t count) -> cmsg_stream_buffer & {
  if (count == 0) {
    return *this;
  }
  std::uint8_t scratch[64]{};
  std::size_t remaining = count;
  while (remaining > 0) {
    const auto chunk = remaining < sizeof(scratch) ? remaining : sizeof(scratch);
    read_bytes(scratch, chunk);
    remaining -= chunk;
  }
  return *this;
}

auto cmsg_stream_buffer::skip_string() -> cmsg_stream_buffer & {
  std::uint16_t length = 0;
  read(length);
  skip_bytes(length);
  return *this;
}
