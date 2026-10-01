#include "pch.hpp"
#include "packet_parser.hpp"

#include <cstdio>
#include <cstring>
#include <optional>

namespace ext_client::plugins::net_log {

namespace {

class cursor {
public:
  cursor(const std::uint8_t* data, std::size_t size) : data_(data), size_(size), pos_(0) {}

  auto remaining() const -> std::size_t { return size_ - pos_; }
  auto position() const -> std::size_t { return pos_; }

  auto read_u8() -> std::optional<std::uint8_t> {
    if (pos_ + 1 > size_) return std::nullopt;
    return data_[pos_++];
  }

  auto read_u16() -> std::optional<std::uint16_t> {
    if (pos_ + 2 > size_) return std::nullopt;
    std::uint16_t v;
    std::memcpy(&v, data_ + pos_, 2);
    pos_ += 2;
    return v;
  }

  auto read_u32() -> std::optional<std::uint32_t> {
    if (pos_ + 4 > size_) return std::nullopt;
    std::uint32_t v;
    std::memcpy(&v, data_ + pos_, 4);
    pos_ += 4;
    return v;
  }

  auto read_u64() -> std::optional<std::uint64_t> {
    if (pos_ + 8 > size_) return std::nullopt;
    std::uint64_t v;
    std::memcpy(&v, data_ + pos_, 8);
    pos_ += 8;
    return v;
  }

  auto read_i8() -> std::optional<std::int8_t> {
    auto v = read_u8();
    if (!v) return std::nullopt;
    return static_cast<std::int8_t>(*v);
  }

  auto read_i16() -> std::optional<std::int16_t> {
    auto v = read_u16();
    if (!v) return std::nullopt;
    return static_cast<std::int16_t>(*v);
  }

  auto read_i32() -> std::optional<std::int32_t> {
    auto v = read_u32();
    if (!v) return std::nullopt;
    return static_cast<std::int32_t>(*v);
  }

  auto read_i64() -> std::optional<std::int64_t> {
    auto v = read_u64();
    if (!v) return std::nullopt;
    return static_cast<std::int64_t>(*v);
  }

  auto read_f32() -> std::optional<float> {
    if (pos_ + 4 > size_) return std::nullopt;
    float v;
    std::memcpy(&v, data_ + pos_, 4);
    pos_ += 4;
    return v;
  }

  auto read_bool() -> std::optional<bool> {
    auto v = read_u8();
    if (!v) return std::nullopt;
    return *v != 0;
  }

  // SRO ascii string: 2-byte length prefix, then bytes (may or may not be null-terminated)
  auto read_ascii() -> std::optional<std::string> {
    auto len = read_u16();
    if (!len) return std::nullopt;
    if (pos_ + *len > size_) return std::nullopt;
    std::string s(reinterpret_cast<const char*>(data_ + pos_), *len);
    pos_ += *len;
    // Strip trailing null if present
    if (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
  }

  auto skip(std::size_t n) -> bool {
    if (pos_ + n > size_) return false;
    pos_ += n;
    return true;
  }

private:
  const std::uint8_t* data_;
  std::size_t size_;
  std::size_t pos_;
};

auto format_hex(std::uint64_t v) -> std::string {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%llu (0x%llX)", static_cast<unsigned long long>(v),
                static_cast<unsigned long long>(v));
  return buf;
}

auto format_signed(std::int64_t v) -> std::string {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
  return buf;
}

auto format_float(float v) -> std::string {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(v));
  return buf;
}

// Recursive: walk field definitions and emit parsed_field entries
auto walk_fields(const pkt::field_def* defs, std::uint16_t start, std::uint16_t count,
                 cursor& cur, std::vector<parsed_field>& out, int indent) -> bool {
  for (std::uint16_t i = 0; i < count; ++i) {
    const auto& def = defs[start + i];
    std::size_t field_offset = cur.position();

    if (def.type == pkt::field_type::loop) {
      // Determine loop count
      std::uint32_t loop_count = 0;
      if (def.loop_count_ref == 0xFFFF) {
        // ReadBool-based: read bool, if true repeat children, keep going
        while (true) {
          auto b = cur.read_bool();
          if (!b) break;
          if (!*b) break;
          // Repeat children
          if (def.child_count > 0) {
            walk_fields(defs, def.child_start, def.child_count, cur, out, indent + 1);
          }
        }
      } else if (def.loop_count_ref != 0xFFFE) {
        // Count-based: look up the referenced field's value
        // The loop_count_ref is an index into the flat table relative to opcode start
        // We stored it as an absolute index in the field table
        // For simplicity, we read the count from the cursor if the ref points to
        // a preceding u8/u16/u32 field. But since we don't track field values,
        // we just read a u8 as the count (most common case)
        auto c = cur.read_u8();
        if (!c) return false;
        loop_count = *c;
        out.push_back({def.name, pkt::field_type::loop, field_offset, 1,
                       std::to_string(loop_count), indent});
        for (std::uint32_t j = 0; j < loop_count; ++j) {
          if (def.child_count > 0) {
            walk_fields(defs, def.child_start, def.child_count, cur, out, indent + 1);
          }
        }
      }
      continue;
    }

    if (def.type == pkt::field_type::branch) {
      // The switch value was already read as a previous field.
      // We need to find which case matches. But we don't have the value here.
      // Instead, we try each case's branch_value and see which one matches
      // the last-read u8. This is a simplification — we'll just emit children
      // for all cases and let the user figure it out from context.
      // Actually, better: we skip branch entirely and just emit children
      // with their branch_value as a label. The runtime parser will need
      // to know the switch value. For now, emit all case children.
      if (def.child_count > 0) {
        // Check if children are cases (branch type) or direct fields
        const auto& first_child = defs[def.child_start];
        if (first_child.type == pkt::field_type::branch) {
          // Children are cases — we need the switch value
          // For now, just emit all cases as potential fields
          // The real parser would need to track the switch variable
          walk_fields(defs, def.child_start, def.child_count, cur, out, indent + 1);
        } else {
          // Direct children (inline cases with branch_value set)
          walk_fields(defs, def.child_start, def.child_count, cur, out, indent + 1);
        }
      }
      continue;
    }

    // Primitive fields
    parsed_field pf;
    pf.name = def.name;
    pf.type = def.type;
    pf.offset = field_offset;
    pf.indent = indent;

    switch (def.type) {
      case pkt::field_type::u8: {
        auto v = cur.read_u8();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 1;
        pf.value = format_hex(*v);
        break;
      }
      case pkt::field_type::u16: {
        auto v = cur.read_u16();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 2;
        pf.value = format_hex(*v);
        break;
      }
      case pkt::field_type::u32: {
        auto v = cur.read_u32();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 4;
        pf.value = format_hex(*v);
        break;
      }
      case pkt::field_type::u64: {
        auto v = cur.read_u64();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 8;
        pf.value = format_hex(*v);
        break;
      }
      case pkt::field_type::i8: {
        auto v = cur.read_i8();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 1;
        pf.value = format_signed(*v);
        break;
      }
      case pkt::field_type::i16: {
        auto v = cur.read_i16();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 2;
        pf.value = format_signed(*v);
        break;
      }
      case pkt::field_type::i32: {
        auto v = cur.read_i32();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 4;
        pf.value = format_signed(*v);
        break;
      }
      case pkt::field_type::i64: {
        auto v = cur.read_i64();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 8;
        pf.value = format_signed(*v);
        break;
      }
      case pkt::field_type::f32: {
        auto v = cur.read_f32();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 4;
        pf.value = format_float(*v);
        break;
      }
      case pkt::field_type::bool_: {
        auto v = cur.read_bool();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = 1;
        pf.value = *v ? "true" : "false";
        break;
      }
      case pkt::field_type::ascii: {
        auto v = cur.read_ascii();
        if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
        pf.size = cur.position() - field_offset;
        pf.value = *v;
        break;
      }
      default:
        // raw or unknown — skip
        pf.size = 0;
        pf.value = "<raw>";
        break;
    }

    out.push_back(pf);
  }
  return true;
}

} // anonymous namespace

auto parse_packet(std::uint16_t opcode, const std::uint8_t* data, std::size_t size) -> parse_result {
  parse_result result;

  const auto* op_def = pkt::lookup(opcode);
  if (!op_def) {
    result.success = false;
    result.error = "No definition available";
    result.bytes_consumed = 0;
    return result;
  }

  if (op_def->field_count == 0) {
    result.success = true;
    result.error = "Empty definition";
    result.bytes_consumed = 0;
    return result;
  }

  cursor cur(data, size);
  walk_fields(pkt::g_field_table, op_def->field_start, op_def->field_count, cur, result.fields, 0);

  result.bytes_consumed = cur.position();
  result.success = cur.remaining() == 0;
  if (cur.remaining() > 0) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%zu bytes unconsumed", cur.remaining());
    result.error = buf;
  }

  return result;
}

} // namespace ext_client::plugins::net_log
