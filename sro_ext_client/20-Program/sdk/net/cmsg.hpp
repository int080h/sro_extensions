#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Non-owning API for the game's pool-allocated wire CMsg. Never construct locally.
class cmsg {
public:
  // Reading mode - instance methods using raw offset arithmetic.
  // this = pointer to binary's internal wire CMsg (0x1068+ bytes).
  auto opcode() const -> std::uint16_t;
  auto header_opcode() const -> std::uint16_t;
  auto body_size() const -> std::size_t;
  auto header_size_raw() const -> std::uint16_t;
  auto is_massive() const -> bool;
  auto is_encrypted() const -> bool;
  auto security_count() const -> std::uint8_t;
  auto security_crc() const -> std::uint8_t;
  auto read_cursor_pos() const -> std::size_t;
  auto extract_payload(std::size_t max_bytes = static_cast<std::size_t>(-1)) const -> std::vector<std::uint8_t>;
  auto write_payload(const void *src, int size) -> int;

private:
  cmsg() = delete;
  ~cmsg() = delete;
};
