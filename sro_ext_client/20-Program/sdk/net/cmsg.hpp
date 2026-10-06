#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// ---------------------------------------------------------------------------
// CMsg — Silkroad Online network packet container
// Pool-allocated by CClientNet::alloc_msg (0x0045D200). Never construct locally.
// Holds opcode header, body length, security flags/CRC, and payload stream.
// ---------------------------------------------------------------------------
class cmsg {
public:
  // 1. Opcode & Size Inspection
  auto opcode() const -> std::uint16_t;
  auto header_opcode() const -> std::uint16_t;
  auto body_size() const -> std::size_t;
  auto header_size_raw() const -> std::uint16_t;

  // 2. Flags & Security Properties
  auto is_massive() const -> bool;
  auto is_encrypted() const -> bool;
  auto security_count() const -> std::uint8_t;
  auto security_crc() const -> std::uint8_t;

  // 3. Payload & Stream Operations
  auto read_cursor_pos() const -> std::size_t;
  auto extract_payload(std::size_t max_bytes = static_cast<std::size_t>(-1)) const -> std::vector<std::uint8_t>;
  auto write_payload(const void* src, int size) -> int;

private:
  cmsg() = delete;
  ~cmsg() = delete;
};
