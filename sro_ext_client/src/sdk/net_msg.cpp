#include "sdk/net_msg.hpp"

#include "sdk/net_manager.hpp"
#include "sdk/cmsg.hpp"

namespace ext_client::net_manager {

  net_msg::net_msg(std::uint16_t opcode)
    : opcode_(opcode) {
    msg_ = reinterpret_cast<cmsg_stream_buffer*>(storage_);
    cmsg_stream_buffer::construct(msg_, opcode_);
  }

  net_msg::~net_msg() {
    if (msg_) {
      msg_->destroy();
      msg_ = nullptr;
    }
  }

  auto net_msg::raw() -> cmsg_stream_buffer* {
    return msg_;
  }

  auto net_msg::raw() const -> const cmsg_stream_buffer* {
    return msg_;
  }

  auto net_msg::opcode() const -> std::uint16_t {
    return opcode_;
  }

  auto net_msg::write_bytes(const void* src, std::size_t size) -> net_msg& {
    if (!msg_ || !src || size == 0) {
      return *this;
    }
    ensure_writing();
    msg_->write_bytes(src, size);
    return *this;
  }

  auto net_msg::write_u8(std::uint8_t value) -> net_msg& {
    return write_bytes(&value, sizeof(value));
  }

  auto net_msg::write_u16(std::uint16_t value) -> net_msg& {
    return write_bytes(&value, sizeof(value));
  }

  auto net_msg::write_u32(std::uint32_t value) -> net_msg& {
    return write_bytes(&value, sizeof(value));
  }

  auto net_msg::write_string(const std::string& value) -> net_msg& {
    if (!msg_) {
      return *this;
    }
    ensure_writing();
    msg_->write_string(value);
    return *this;
  }

  auto net_msg::send() -> void {
    if (!msg_) {
      return;
    }
    if (writing_) {
      msg_->toggle_after();
      writing_ = false;
    }
    detail::programmatic_send = true;
    cmsg::send_from_buffer(msg_);
    detail::programmatic_send = false;
    msg_->destroy();
    msg_ = nullptr;
  }

  auto net_msg::ensure_writing() -> void {
    if (!msg_ || writing_) {
      return;
    }
    msg_->toggle_before();
    writing_ = true;
  }

} // namespace ext_client::net_manager
