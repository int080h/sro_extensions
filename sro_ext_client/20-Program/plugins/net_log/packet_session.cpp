#include "pch.hpp"
#include "packet_session.hpp"

#include <cstdio>
#include <cstring>

namespace ext_client::plugins::net_log {

namespace {

constexpr std::uint32_t SESSION_MAGIC = 0x53524F50; // "SROP"
constexpr std::uint32_t SESSION_VERSION = 1;

// Simple binary read/write helpers

auto write_u8(FILE* f, std::uint8_t v) -> bool {
  return std::fwrite(&v, 1, 1, f) == 1;
}

auto write_u16(FILE* f, std::uint16_t v) -> bool {
  return std::fwrite(&v, 2, 1, f) == 1;
}

auto write_u32(FILE* f, std::uint32_t v) -> bool {
  return std::fwrite(&v, 4, 1, f) == 1;
}

auto write_u64(FILE* f, std::uint64_t v) -> bool {
  return std::fwrite(&v, 8, 1, f) == 1;
}

auto write_bool(FILE* f, bool v) -> bool {
  return write_u8(f, v ? 1 : 0);
}

auto write_string(FILE* f, const std::string& s) -> bool {
  auto len = static_cast<std::uint16_t>(s.size());
  if (!write_u16(f, len)) return false;
  if (len > 0 && std::fwrite(s.data(), 1, len, f) != len) return false;
  return true;
}

auto write_bytes(FILE* f, const std::vector<std::uint8_t>& data) -> bool {
  auto len = static_cast<std::uint32_t>(data.size());
  if (!write_u32(f, len)) return false;
  if (len > 0 && std::fwrite(data.data(), 1, len, f) != len) return false;
  return true;
}

auto read_u8(FILE* f, std::uint8_t& v) -> bool {
  return std::fread(&v, 1, 1, f) == 1;
}

auto read_u16(FILE* f, std::uint16_t& v) -> bool {
  return std::fread(&v, 2, 1, f) == 1;
}

auto read_u32(FILE* f, std::uint32_t& v) -> bool {
  return std::fread(&v, 4, 1, f) == 1;
}

auto read_u64(FILE* f, std::uint64_t& v) -> bool {
  return std::fread(&v, 8, 1, f) == 1;
}

auto read_bool(FILE* f, bool& v) -> bool {
  std::uint8_t b;
  if (!read_u8(f, b)) return false;
  v = b != 0;
  return true;
}

auto read_string(FILE* f, std::string& s) -> bool {
  std::uint16_t len;
  if (!read_u16(f, len)) return false;
  s.resize(len);
  if (len > 0 && std::fread(s.data(), 1, len, f) != len) return false;
  return true;
}

auto read_bytes(FILE* f, std::vector<std::uint8_t>& data) -> bool {
  std::uint32_t len;
  if (!read_u32(f, len)) return false;
  data.resize(len);
  if (len > 0 && std::fread(data.data(), 1, len, f) != len) return false;
  return true;
}

} // anonymous namespace

auto save_session(const std::string& path, const std::vector<session_packet>& packets) -> bool {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;

  bool ok = true;
  ok = ok && write_u32(f, SESSION_MAGIC);
  ok = ok && write_u32(f, SESSION_VERSION);
  ok = ok && write_u32(f, static_cast<std::uint32_t>(packets.size()));

  for (const auto& p : packets) {
    ok = ok && write_u32(f, p.id);
    ok = ok && write_u32(f, p.tick);
    ok = ok && write_u64(f, p.timestamp_ms);
    ok = ok && write_u8(f, p.direction);
    ok = ok && write_u8(f, p.layer);
    ok = ok && write_u16(f, p.opcode);
    ok = ok && write_u16(f, p.payload_size);
    ok = ok && write_bool(f, p.has_wire_header);
    ok = ok && write_u16(f, p.header_size_raw);
    ok = ok && write_u16(f, p.header_payload_size);
    ok = ok && write_bool(f, p.massive);
    ok = ok && write_u8(f, p.security_count);
    ok = ok && write_u8(f, p.security_crc);
    ok = ok && write_bool(f, p.blocked);
    ok = ok && write_bool(f, p.modified);
    ok = ok && write_string(f, p.capture_point);
    ok = ok && write_bytes(f, p.payload);
    if (!ok) break;
  }

  std::fclose(f);
  return ok;
}

auto load_session(const std::string& path, std::vector<session_packet>& packets) -> bool {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;

  bool ok = true;
  std::uint32_t magic, version, count;
  ok = ok && read_u32(f, magic);
  ok = ok && read_u32(f, version);
  ok = ok && read_u32(f, count);

  if (!ok || magic != SESSION_MAGIC || version != SESSION_VERSION) {
    std::fclose(f);
    return false;
  }

  packets.clear();
  packets.reserve(count);

  for (std::uint32_t i = 0; i < count && ok; ++i) {
    session_packet p{};
    ok = ok && read_u32(f, p.id);
    ok = ok && read_u32(f, p.tick);
    ok = ok && read_u64(f, p.timestamp_ms);
    ok = ok && read_u8(f, p.direction);
    ok = ok && read_u8(f, p.layer);
    ok = ok && read_u16(f, p.opcode);
    ok = ok && read_u16(f, p.payload_size);
    ok = ok && read_bool(f, p.has_wire_header);
    ok = ok && read_u16(f, p.header_size_raw);
    ok = ok && read_u16(f, p.header_payload_size);
    ok = ok && read_bool(f, p.massive);
    ok = ok && read_u8(f, p.security_count);
    ok = ok && read_u8(f, p.security_crc);
    ok = ok && read_bool(f, p.blocked);
    ok = ok && read_bool(f, p.modified);
    ok = ok && read_string(f, p.capture_point);
    ok = ok && read_bytes(f, p.payload);
    if (ok) packets.push_back(std::move(p));
  }

  std::fclose(f);
  return ok;
}

} // namespace ext_client::plugins::net_log
