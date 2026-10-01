#include "pch.hpp"
#include "sdk/net/packet_builder.hpp"
#include "utils/offsets.hpp"
#include <stdexcept>

namespace ext_client::net {
  packet_builder::packet_builder(std::uint16_t opcode) {
    if (!cmsg_stream_buffer::construct(stream(), opcode))
      throw std::runtime_error("CMsgStreamBuffer construction failed");
    stream()->toggle_before();
  }
  packet_builder::~packet_builder() {
    stream()->toggle_after();
    stream()->destroy();
  }
  auto packet_builder::operator<<(const std::string &value) -> packet_builder & {
    stream()->write_string(value);
    return *this;
  }
  auto packet_builder::operator<<(const char *value) -> packet_builder & {
    return *this << std::string(value);
  }
  auto packet_builder::operator>>(std::string &value) -> packet_builder & {
    std::uint16_t length = 0;
    *this >> length;
    if (!m_read_ok)
      return *this;
    value.resize(length);
    return read_raw(value.data(), length);
  }
  auto packet_builder::read_raw(void *dst, std::size_t size) -> packet_builder & {
    if (m_read_ok && size && (!dst || stream()->read_bytes(dst, size) != size))
      m_read_ok = false;
    return *this;
  }
  auto packet_builder::skip_bytes(std::size_t count) -> packet_builder & {
    std::uint8_t scratch[64];
    while (m_read_ok && count) {
      const auto chunk = (std::min)(count, sizeof(scratch));
      read_raw(scratch, chunk);
      count -= chunk;
    }
    return *this;
  }
  auto packet_builder::skip_string() -> packet_builder & {
    std::uint16_t length = 0;
    *this >> length;
    return m_read_ok ? skip_bytes(length) : *this;
  }
  auto packet_builder::send() -> void {
    stream()->toggle_after();
    reinterpret_cast<void(__cdecl *)(cmsg_stream_buffer *)>(0x00941600)(stream());
  }
  auto packet_builder::copy_from(const cmsg_stream_buffer &source) -> std::unique_ptr<packet_builder> {
    auto copy = std::make_unique<packet_builder>(source.get_opcode());
    const auto bytes = source.extract_payload();
    copy->stream()->replace_payload(bytes.data(), bytes.size());
    return copy;
  }
} // namespace ext_client::net
