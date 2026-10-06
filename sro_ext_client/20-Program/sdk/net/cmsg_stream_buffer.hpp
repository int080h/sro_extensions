#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// CMsgStreamBuffer — Small (28-byte) read/write buffer for packet payloads
// Wraps native packet byte streams with type-safe streaming helpers.
// ---------------------------------------------------------------------------
class cmsg_stream_buffer {
public:
  static constexpr std::size_t storage_size = 0x1C;

  // 1. Stream Inspection
  auto get_opcode() const -> std::uint16_t;
  auto get_payload_size() const -> std::size_t;
  auto get_read_cursor_pos() const -> std::size_t;

  // 2. Stream Buffer Operations & Lifecycle
  static auto construct(cmsg_stream_buffer* buf, std::uint16_t opcode) -> cmsg_stream_buffer*;
  auto destroy() -> void;
  auto extract_payload(std::size_t max_bytes = static_cast<std::size_t>(-1)) const -> std::vector<std::uint8_t>;
  auto replace_payload(const std::uint8_t* bytes, std::size_t size) -> bool;
  auto read_bytes(void* dst, std::size_t size) -> std::size_t;
  auto write_bytes(const void* src, std::size_t size) -> void;
  auto toggle_before() -> void;
  auto toggle_after() -> void;
  auto flush_remaining() -> void;

  // 3. String & Byte Skipping Helpers
  auto read_string(std::string& value) -> cmsg_stream_buffer&;
  auto write_string(const std::string& value) -> cmsg_stream_buffer&;
  auto skip_bytes(std::size_t count) -> cmsg_stream_buffer&;
  auto skip_string() -> cmsg_stream_buffer&;

  // 4. Templated Primitive Streaming
  template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
  auto read(T& value) -> cmsg_stream_buffer& {
    read_bytes(&value, sizeof(value));
    return *this;
  }

  template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
  auto write(const T& value) -> cmsg_stream_buffer& {
    write_bytes(&value, sizeof(value));
    return *this;
  }
};
