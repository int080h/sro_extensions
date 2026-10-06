#include "pch.hpp"
#include "sdk/net/cmsg.hpp"

#include "utils/offsets.hpp"

namespace {
  using write_payload_fn = int(__thiscall*)(void* self, void* src, int size);
  using ext_client::off::as_fn;

  namespace wire {
    inline constexpr std::size_t data_ptr = 0x1034;
    inline constexpr std::size_t read_cursor = 0x103C;
    inline constexpr std::size_t write_cursor = 0x103E;
    inline constexpr std::size_t opcode_ptr = 0x1050;
    inline constexpr std::size_t size_ptr = 0x1054;
    inline constexpr std::size_t sec_count_ptr = 0x1058;
    inline constexpr std::size_t crc_ptr = 0x105C;
  } // namespace wire
} // namespace

// ===========================================================================
// 1. Opcode & Size Inspection
// ===========================================================================
auto cmsg::opcode() const -> std::uint16_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* ptr = *reinterpret_cast<const std::uint16_t* const*>(raw + wire::opcode_ptr);
  return ptr ? *ptr : 0;
}

auto cmsg::header_opcode() const -> std::uint16_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* data = *reinterpret_cast<void* const*>(raw + wire::data_ptr);
  if (!data) {
    return 0;
  }
  return *reinterpret_cast<const std::uint16_t*>(static_cast<const std::uint8_t*>(data) + 2);
}

auto cmsg::body_size() const -> std::size_t {
  return static_cast<std::size_t>(header_size_raw() & 0x7FFFu);
}

auto cmsg::header_size_raw() const -> std::uint16_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* ptr = *reinterpret_cast<const std::uint16_t* const*>(raw + wire::size_ptr);
  return ptr ? *ptr : 0;
}

// ===========================================================================
// 2. Flags & Security Properties
// ===========================================================================
auto cmsg::is_massive() const -> bool {
  return (header_size_raw() & 0x8000u) != 0;
}

auto cmsg::is_encrypted() const -> bool {
  return security_count() != 0 || security_crc() != 0;
}

auto cmsg::security_count() const -> std::uint8_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* ptr = *reinterpret_cast<const std::uint8_t* const*>(raw + wire::sec_count_ptr);
  return ptr ? *ptr : 0;
}

auto cmsg::security_crc() const -> std::uint8_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* ptr = *reinterpret_cast<const std::uint8_t* const*>(raw + wire::crc_ptr);
  return ptr ? *ptr : 0;
}

// ===========================================================================
// 3. Payload & Stream Operations
// ===========================================================================
auto cmsg::read_cursor_pos() const -> std::size_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  return *reinterpret_cast<const std::uint16_t*>(raw + wire::read_cursor);
}

auto cmsg::extract_payload(std::size_t max_bytes) const -> std::vector<std::uint8_t> {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* data = *reinterpret_cast<const std::uint8_t* const*>(raw + wire::data_ptr);
  const auto total = body_size();
  if (!data || total == 0) {
    return {};
  }
  constexpr std::size_t k_header_size = 6;
  const auto count = std::min(total, max_bytes);
  return std::vector<std::uint8_t>(data + k_header_size, data + k_header_size + count);
}

auto cmsg::write_payload(const void* src, int size) -> int {
  auto* self = this;
  const auto fn = as_fn<write_payload_fn>(0x00462470);
  if (!fn) {
    return 0;
  }
  return fn(self, const_cast<void*>(src), size);
}
