#include "menu/packet_decoder.hpp"

#include <cstring>
#include <cstdio>

namespace ext_client::menu::packet_decoder {
  namespace {

    class reader {
    public:
      explicit reader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

      auto remaining() const -> std::size_t { return bytes_.size() - pos_; }
      auto offset() const -> std::size_t { return pos_; }

      auto read_u8(std::uint8_t& value) -> bool {
        if (pos_ + 1 > bytes_.size()) {
          return false;
        }
        value = bytes_[pos_++];
        return true;
      }

      auto read_u16(std::uint16_t& value) -> bool {
        if (pos_ + 2 > bytes_.size()) {
          return false;
        }
        std::memcpy(&value, bytes_.data() + pos_, sizeof(value));
        pos_ += 2;
        return true;
      }

      auto read_u32(std::uint32_t& value) -> bool {
        if (pos_ + 4 > bytes_.size()) {
          return false;
        }
        std::memcpy(&value, bytes_.data() + pos_, sizeof(value));
        pos_ += 4;
        return true;
      }

      auto read_u24(std::uint32_t& value) -> bool {
        if (pos_ + 3 > bytes_.size()) {
          return false;
        }
        value = static_cast<std::uint32_t>(bytes_[pos_]) | (static_cast<std::uint32_t>(bytes_[pos_ + 1]) << 8) |
                (static_cast<std::uint32_t>(bytes_[pos_ + 2]) << 16);
        pos_ += 3;
        return true;
      }

      auto read_f32(float& value) -> bool {
        if (pos_ + 4 > bytes_.size()) {
          return false;
        }
        std::memcpy(&value, bytes_.data() + pos_, sizeof(value));
        pos_ += 4;
        return true;
      }

    private:
      const std::vector<std::uint8_t>& bytes_;
      std::size_t pos_ = 0;
    };

    auto push_u8(decode_result& out, reader& r, const char* name) -> void {
      std::uint8_t value = 0;
      const auto offset = r.offset();
      if (!r.read_u8(value)) {
        return;
      }
      char buf[64]{};
      std::snprintf(buf, sizeof(buf), "%u", value);
      out.fields.push_back({offset, field_type::u8, name, buf});
    }

    auto push_u16(decode_result& out, reader& r, const char* name) -> void {
      std::uint16_t value = 0;
      const auto offset = r.offset();
      if (!r.read_u16(value)) {
        return;
      }
      char buf[64]{};
      std::snprintf(buf, sizeof(buf), "%u (0x%04X)", value, value);
      out.fields.push_back({offset, field_type::u16, name, buf});
    }

    auto push_u32(decode_result& out, reader& r, const char* name) -> void {
      std::uint32_t value = 0;
      const auto offset = r.offset();
      if (!r.read_u32(value)) {
        return;
      }
      char buf[64]{};
      std::snprintf(buf, sizeof(buf), "%u (0x%08X)", value, value);
      out.fields.push_back({offset, field_type::u32, name, buf});
    }

    auto push_u24(decode_result& out, reader& r, const char* name) -> void {
      std::uint32_t value = 0;
      const auto offset = r.offset();
      if (!r.read_u24(value)) {
        return;
      }
      char buf[64]{};
      std::snprintf(buf, sizeof(buf), "%u (0x%06X)", value, value);
      out.fields.push_back({offset, field_type::u24, name, buf});
    }

    auto push_f32(decode_result& out, reader& r, const char* name) -> void {
      float value = 0.f;
      const auto offset = r.offset();
      if (!r.read_f32(value)) {
        return;
      }
      char buf[64]{};
      std::snprintf(buf, sizeof(buf), "%.3f", value);
      out.fields.push_back({offset, field_type::f32, name, buf});
    }

    auto decode_skill_cast_begin(const std::vector<std::uint8_t>& payload) -> decode_result {
      decode_result out{};
      out.opcode_name = "SkillCastBegin";
      reader r(payload);
      push_u8(out, r, "result");
      push_u8(out, r, "castType");
      push_u8(out, r, "unk");
      push_u32(out, r, "skillId");
      push_u32(out, r, "sourceUid");
      push_u32(out, r, "skillUid");
      push_u32(out, r, "destUid");
      return out;
    }

    auto decode_skill_cast_end(const std::vector<std::uint8_t>& payload) -> decode_result {
      decode_result out{};
      out.opcode_name = "SkillCastEnd";
      reader r(payload);
      push_u8(out, r, "result");
      push_u32(out, r, "skillUid");
      push_u32(out, r, "destUid");
      return out;
    }

    auto decode_handshake(const std::vector<std::uint8_t>& payload) -> decode_result {
      decode_result out{};
      out.opcode_name = "Handshake";
      reader r(payload);
      push_u8(out, r, "flag0");
      push_u8(out, r, "flag1");
      push_u8(out, r, "flag2");
      push_u8(out, r, "flag3");
      return out;
    }

    auto decode_movement(const std::vector<std::uint8_t>& payload) -> decode_result {
      decode_result out{};
      out.opcode_name = "Movement";
      reader r(payload);
      push_u24(out, r, "entityId");
      push_u16(out, r, "region");
      push_f32(out, r, "x");
      push_f32(out, r, "y");
      push_f32(out, r, "z");
      return out;
    }

    auto decode_fallback(const std::vector<std::uint8_t>& payload) -> decode_result {
      decode_result out{};
      out.opcode_name = nullptr;
      reader r(payload);

      while (r.remaining() >= 4) {
        push_u32(out, r, "u32");
      }
      while (r.remaining() >= 2) {
        push_u16(out, r, "u16");
      }
      while (r.remaining() >= 1) {
        push_u8(out, r, "u8");
      }
      return out;
    }

  } // namespace

  auto opcode_name(std::uint16_t opcode) -> const char* {
    switch (opcode) {
      case 0xB070:
        return "SkillCastBegin";
      case 0xB071:
        return "SkillCastEnd";
      case 0xA101:
        return "Handshake";
      case 0x7001:
        return "Movement";
      default:
        return nullptr;
    }
  }

  auto decode(const ext_client::hooks::net::entry& entry) -> decode_result {
    switch (entry.opcode) {
      case 0xB070:
        return decode_skill_cast_begin(entry.payload);
      case 0xB071:
        return decode_skill_cast_end(entry.payload);
      case 0xA101:
        return decode_handshake(entry.payload);
      case 0x7001:
        return decode_movement(entry.payload);
      default: {
        auto out = decode_fallback(entry.payload);
        out.opcode_name = opcode_name(entry.opcode);
        return out;
      }
    }
  }

} // namespace ext_client::menu::packet_decoder
