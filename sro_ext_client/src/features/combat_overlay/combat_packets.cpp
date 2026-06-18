#include "features/combat_overlay/combat_packets.hpp"

#include "config/client_config.hpp"
#include "features/combat_overlay/combat_overlay.hpp"

#include <cstddef>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace ext_client::combat_packets {
  namespace {

    std::unordered_map<std::uint32_t, std::uint32_t> g_skill_uid_cache;

    class payload_reader {
    public:
      explicit payload_reader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

      auto read_u8(std::uint8_t& value) -> bool {
        if (pos_ + sizeof(std::uint8_t) > bytes_.size()) {
          return false;
        }
        value = bytes_[pos_++];
        return true;
      }

      auto read_u32(std::uint32_t& value) -> bool {
        if (pos_ + sizeof(std::uint32_t) > bytes_.size()) {
          return false;
        }
        std::memcpy(&value, bytes_.data() + pos_, sizeof(value));
        pos_ += sizeof(value);
        return true;
      }

      auto read_uint24(std::uint32_t& value) -> bool {
        if (pos_ + 3 > bytes_.size()) {
          return false;
        }
        value = static_cast<std::uint32_t>(bytes_[pos_]) | (static_cast<std::uint32_t>(bytes_[pos_ + 1]) << 8) |
                (static_cast<std::uint32_t>(bytes_[pos_ + 2]) << 16);
        pos_ += 3;
        return true;
      }

    private:
      const std::vector<std::uint8_t>& bytes_;
      std::size_t pos_ = 0;
    };

    auto resolve_attacker_uid(std::uint32_t attacker_uid, std::uint32_t skill_uid) -> std::uint32_t {
      if (attacker_uid != 0) {
        return attacker_uid;
      }

      if (skill_uid == 0) {
        return 0;
      }

      const auto it = g_skill_uid_cache.find(skill_uid);
      return it != g_skill_uid_cache.end() ? it->second : 0;
    }

    auto parse_damage_targets(payload_reader& reader, std::uint32_t attacker_uid) -> void {
      std::uint8_t has_damage = 0;
      if (!reader.read_u8(has_damage) || has_damage != 1) {
        return;
      }

      std::uint8_t hit_count = 0;
      std::uint8_t target_count = 0;
      if (!reader.read_u8(hit_count) || !reader.read_u8(target_count)) {
        return;
      }

      (void)hit_count;

      for (std::uint8_t i = 0; i < target_count; ++i) {
        std::uint32_t target_uid = 0;
        std::uint32_t effect = 0;
        if (!reader.read_u32(target_uid) || !reader.read_u32(effect)) {
          return;
        }

        std::uint8_t damage_flag = 0;
        std::uint32_t damage = 0;
        std::uint32_t unk = 0;
        if (!reader.read_u8(damage_flag) || !reader.read_uint24(damage) || !reader.read_u32(unk)) {
          return;
        }

        if (should_skip_target(effect) || damage == 0) {
          continue;
        }

        combat_overlay::on_damage_event(attacker_uid, target_uid, damage, damage_flag, effect);
      }
    }

    auto should_parse_b070_damage(std::uint8_t cast_type) -> bool {
      const auto& cfg = ext_client::config::data().combat_overlay;
      if (cfg.skill_cast_attack_type < 0) {
        return false;
      }
      return cast_type == static_cast<std::uint8_t>(cfg.skill_cast_attack_type);
    }

    auto parse_skill_cast_begin(const std::vector<std::uint8_t>& payload) -> void {
      const auto& cfg = ext_client::config::data().combat_overlay;
      if (!cfg.enabled || payload.empty()) {
        return;
      }

      payload_reader reader(payload);

      std::uint8_t result = 0;
      if (!reader.read_u8(result) || result != 1) {
        return;
      }

      std::uint8_t cast_type = 0;
      std::uint8_t unk_byte = 0;
      std::uint32_t skill_id = 0;
      std::uint32_t source_uid = 0;
      std::uint32_t skill_uid = 0;
      std::uint32_t dest_uid = 0;
      if (!reader.read_u8(cast_type) || !reader.read_u8(unk_byte) || !reader.read_u32(skill_id) || !reader.read_u32(source_uid) ||
          !reader.read_u32(skill_uid) || !reader.read_u32(dest_uid)) {
        return;
      }

      if (skill_uid != 0 && source_uid != 0) {
        g_skill_uid_cache[skill_uid] = source_uid;
      }

      if (should_parse_b070_damage(cast_type)) {
        parse_damage_targets(reader, source_uid);
      }
    }

    auto parse_skill_cast_end(const std::vector<std::uint8_t>& payload) -> void {
      const auto& cfg = ext_client::config::data().combat_overlay;
      if (!cfg.enabled || payload.empty()) {
        return;
      }

      payload_reader reader(payload);

      std::uint8_t result = 0;
      if (!reader.read_u8(result) || result != 1) {
        return;
      }

      std::uint32_t skill_uid = 0;
      std::uint32_t dest_uid = 0;
      if (!reader.read_u32(skill_uid) || !reader.read_u32(dest_uid)) {
        return;
      }

      const std::uint32_t attacker_uid = resolve_attacker_uid(0, skill_uid);
      parse_damage_targets(reader, attacker_uid);
    }

  } // namespace

  auto handle_payload(std::uint16_t opcode, const std::vector<std::uint8_t>& payload) -> void {
    if (opcode == opcode_skill_cast_begin) {
      parse_skill_cast_begin(payload);
    } else if (opcode == opcode_skill_cast_end) {
      parse_skill_cast_end(payload);
    }
  }

} // namespace ext_client::combat_packets
