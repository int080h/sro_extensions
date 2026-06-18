#pragma once



#include <cstdint>

#include <vector>



namespace ext_client::combat_packets {



  inline constexpr std::uint16_t opcode_skill_cast_begin = 0xB070;

  inline constexpr std::uint16_t opcode_skill_cast_end = 0xB071;



  namespace skill_effect_flag {

    inline constexpr std::uint32_t KnockBack = 1u;

    inline constexpr std::uint32_t Block = 2u;

    inline constexpr std::uint32_t Position = 4u;

    inline constexpr std::uint32_t Cancel = 8u;

    inline constexpr std::uint32_t Dead = 128u;

  } // namespace skill_effect_flag



  namespace skill_damage_flag {

    inline constexpr std::uint8_t None = 0u;

    inline constexpr std::uint8_t Normal = 1u;

    inline constexpr std::uint8_t Critical = 2u;

    inline constexpr std::uint8_t Status = 4u;

  } // namespace skill_damage_flag



  inline auto should_skip_target(std::uint32_t effect) -> bool {

    return (effect & skill_effect_flag::Block) != 0 || (effect & skill_effect_flag::Cancel) != 0;

  }



  // Parsed after vanilla dispatch so the live read cursor is never touched.

  auto handle_payload(std::uint16_t opcode, const std::vector<std::uint8_t>& payload) -> void;



} // namespace ext_client::combat_packets

