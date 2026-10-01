#pragma once

#include <cstddef>

#include <cstdint>

#include <string>

#include <type_traits>

#include <vector>

class cmsg_stream_buffer;

// CMsgStreamBuffer - small (28-byte) read/write buffer for message payloads.

class cmsg_stream_buffer {

public:
  static constexpr std::size_t storage_size = 0x1C;

  auto is_mode_flag() -> bool;

  auto get_read_cursor() -> std::uint32_t;
  auto get_total_bytes() -> std::uint32_t;
  auto get_node_primary() -> void *;
  auto get_node_active() -> void *;
  auto get_msg_id() -> std::uint16_t;
  auto get_opcode() const -> std::uint16_t;
  auto get_payload_size() const -> std::size_t;
  auto get_read_cursor_pos() const -> std::size_t;

  auto set_mode_flag(bool val) -> void;
  auto set_read_cursor(std::uint32_t val) -> void;
  auto set_total_bytes(std::uint32_t val) -> void;
  auto set_node_primary(void *val) -> void;
  auto set_node_active(void *val) -> void;
  auto set_msg_id(std::uint16_t val) -> void;

  auto destroy() -> void;
  auto extract_payload(std::size_t max_bytes = static_cast<std::size_t>(-1)) const -> std::vector<std::uint8_t>;
  auto replace_payload(const std::uint8_t *bytes, std::size_t size) -> bool;
  auto read_bytes(void *dst, std::size_t size) -> std::size_t;
  auto write_bytes(const void *src, std::size_t size) -> void;
  auto toggle_before() -> void;
  auto toggle_after() -> void;
  auto flush_remaining() -> void;

  template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>

  auto read(T &value) -> cmsg_stream_buffer & {

    read_bytes(&value, sizeof(value));

    return *this;
  }

  template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>

  auto write(const T &value) -> cmsg_stream_buffer & {

    write_bytes(&value, sizeof(value));

    return *this;
  }

  auto read_string(std::string &value) -> cmsg_stream_buffer &;
  auto write_string(const std::string &value) -> cmsg_stream_buffer &;
  auto skip_bytes(std::size_t count) -> cmsg_stream_buffer &;
  auto skip_string() -> cmsg_stream_buffer &;

  static auto construct(cmsg_stream_buffer *buf, std::uint16_t opcode) -> cmsg_stream_buffer *;
};
