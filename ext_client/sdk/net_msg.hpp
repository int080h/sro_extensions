#pragma once

#include "sdk/cmsg_stream_buffer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ext_client::net_manager {

  // RAII builder: write payload into a CMsgStreamBuffer, then send via CMsg (SendMsg).
  class net_msg {
  public:
    explicit net_msg(std::uint16_t opcode);
    ~net_msg();

    net_msg(const net_msg&) = delete;
    net_msg& operator=(const net_msg&) = delete;

    auto raw() -> cmsg_stream_buffer*;
    auto raw() const -> const cmsg_stream_buffer*;
    auto opcode() const -> std::uint16_t;

    auto write_bytes(const void* src, std::size_t size) -> net_msg&;
    auto write_u8(std::uint8_t value) -> net_msg&;
    auto write_u16(std::uint16_t value) -> net_msg&;
    auto write_u32(std::uint32_t value) -> net_msg&;
    auto write_string(const std::string& value) -> net_msg&;

    // Queues packet via SendMsg and releases ownership.
    auto send() -> void;

  private:
    auto ensure_writing() -> void;

    alignas(cmsg_stream_buffer) std::uint8_t storage_[sizeof(cmsg_stream_buffer)]{};
    cmsg_stream_buffer* msg_ = nullptr;
    std::uint16_t opcode_ = 0;
    bool writing_ = false;
  };

} // namespace ext_client::net_manager
