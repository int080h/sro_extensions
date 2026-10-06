#pragma once

#include "sdk/net/cmsg_stream_buffer.hpp"
#include <array>
#include <memory>
#include <string>

namespace ext_client::net {
  // Owns storage for a real CMsgStreamBuffer and delegates construction/IO to the client.
  class packet_builder {
  public:
    explicit packet_builder(std::uint16_t opcode);
    ~packet_builder();
    packet_builder(const packet_builder &) = delete;
    packet_builder &operator=(const packet_builder &) = delete;
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
    auto operator<<(const T &value) -> packet_builder & {
      stream()->write_bytes(&value, sizeof(value));
      return *this;
    }
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
    auto operator>>(T &value) -> packet_builder & {
      return read_raw(&value, sizeof(value));
    }
    auto operator<<(const std::string &value) -> packet_builder &;
    auto operator<<(const char *value) -> packet_builder &;
    auto operator>>(std::string &value) -> packet_builder &;
    auto skip_bytes(std::size_t count) -> packet_builder &;
    auto skip_string() -> packet_builder &;
    auto good() const -> bool { return m_read_ok; }
    explicit operator bool() const { return good(); }
    auto send() -> void;
    static auto copy_from(const cmsg_stream_buffer &source) -> std::unique_ptr<packet_builder>;

  private:
    auto stream() -> cmsg_stream_buffer * { return reinterpret_cast<cmsg_stream_buffer *>(m_storage.data()); }
    auto read_raw(void *dst, std::size_t size) -> packet_builder &;
    alignas(4) std::array<std::uint8_t, cmsg_stream_buffer::storage_size> m_storage{};
    bool m_read_ok = true;
  };
  static_assert(sizeof(void *) == 4, "Client SDK requires x86");
  static_assert(alignof(packet_builder) == 4);
} // namespace ext_client::net
