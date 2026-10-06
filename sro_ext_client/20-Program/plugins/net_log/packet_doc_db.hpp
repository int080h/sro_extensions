#pragma once

#include <cstdint>

namespace ext_client::plugins::net_log {

  enum class opcode_category : std::uint8_t {
    all = 0,
    handshake_system,
    auth_login,
    character_data,
    entity_spawn,
    movement,
    chat,
    combat_skills,
    inventory_storage,
    social_party_guild,
    stall_exchange,
    environment,
    other,
    count
  };

  auto classify_opcode(std::uint16_t opcode) -> opcode_category;
  auto opcode_category_name(opcode_category cat) -> const char*;
  auto opcode_category_badge(opcode_category cat) -> const char*;
  auto opcode_summary_doc(std::uint16_t opcode) -> const char*;

  // Semantic enum string formatters based on Silkroad documentation & binary RE
  auto chat_type_name(std::uint8_t type) -> const char*;
  auto life_state_name(std::uint8_t state) -> const char*;
  auto motion_state_name(std::uint8_t state) -> const char*;
  auto body_mode_name(std::uint8_t mode) -> const char*;
  auto pvp_state_name(std::uint8_t state) -> const char*;
  auto entity_type_name(std::uint8_t type) -> const char*;
  auto despawn_reason_name(std::uint8_t reason) -> const char*;
  auto move_speed_type_name(std::uint8_t type) -> const char*;
  auto notice_type_name(std::uint8_t type) -> const char*;
  auto status_update_type_name(std::uint8_t type) -> const char*;
  auto damage_flag_name(std::uint8_t flag) -> const char*;

} // namespace ext_client::plugins::net_log
