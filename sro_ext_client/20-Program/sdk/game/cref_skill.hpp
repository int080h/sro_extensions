#pragma once

#include "utils/memory.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/string.hpp"
#include "sdk/ui/cui_string_manager.hpp"

#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// CRefSkill — Static Skill Definition loaded from refskill.txt / PK2
// Lookup function: sub_A75BD0(0x0117EE20, skill_id)
// Secondary group lookup: sub_A76200(group_id)
// ---------------------------------------------------------------------------
class cref_skill {
public:
  [[nodiscard]] auto id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x00);
  }

  [[nodiscard]] auto group_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x04);
  }

  // Internal code name (msvc9::wstring at +0x20, e.g. "SKILL_CH_SWORD_SLASH")
  [[nodiscard]] auto code_name() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    const auto* str = reinterpret_cast<const ext_client::msvc9::wstring*>(
      reinterpret_cast<const std::uint8_t*>(this) + 0x20);
    return (str && str->c_str()) ? str->c_str() : L"";
  }

  // Localized Display Name dynamically resolved from game textdata (SN_ prefix)
  [[nodiscard]] auto name() const -> std::string {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return "";
    const auto* cn = code_name();
    if (cn && *cn != L'\0') {
      const std::wstring sn = L"SN_" + std::wstring(cn);
      const auto loc = ext_client::sdk::ui::get_string(sn.c_str());
      if (!loc.empty()) {
        return ext_client::utils::string::to_utf8(loc.c_str());
      }
      const auto loc_direct = ext_client::sdk::ui::get_string(cn);
      if (!loc_direct.empty()) {
        return ext_client::utils::string::to_utf8(loc_direct.c_str());
      }
      return ext_client::utils::string::to_utf8(cn);
    }
    return "";
  }

  // Skill type category:
  // 0 = Passive
  // 1 = Direct Attack
  // 2 = Self-Buff / Active Buff
  // 3 = Debuff / Heal / Target Spell
  // 5 = Ground Target / Area of Effect (AoE)
  [[nodiscard]] auto skill_type() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x3C);
  }

  [[nodiscard]] auto is_passive() const -> bool { return skill_type() == 0; }
  [[nodiscard]] auto is_attack() const -> bool { return skill_type() == 1; }
  [[nodiscard]] auto is_buff() const -> bool { return skill_type() == 2; }
  [[nodiscard]] auto is_debuff_or_heal() const -> bool { return skill_type() == 3; }
  [[nodiscard]] auto is_aoe_ground() const -> bool { return skill_type() == 5; }

  // Action type ID
  [[nodiscard]] auto action_type() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x44);
  }

  // Target requirement type (Self, Enemy, Party Member, Dead Ally, Corpse)
  [[nodiscard]] auto target_type() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x48);
  }

  // Casting / preparation duration in milliseconds
  [[nodiscard]] auto cast_time_ms() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x50);
  }

  // Base cooldown in milliseconds
  [[nodiscard]] auto cooldown_ms() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x54);
  }

  // MP consumption
  [[nodiscard]] auto mp_cost() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x58);
  }

  // HP consumption (for sacrificial skills)
  [[nodiscard]] auto hp_cost() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x5C);
  }

  // Cast range in 10ths of meters (e.g. 150 = 15.0 meters)
  [[nodiscard]] auto range_raw() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x60);
  }

  [[nodiscard]] auto range_meters() const -> float {
    return static_cast<float>(range_raw()) * 0.1f;
  }

  // Required primary weapon (e.g. Sword, Blade, Spear, Glavie, Bow, European weapons)
  [[nodiscard]] auto req_weapon_1() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x64);
  }

  // Required secondary weapon (e.g. Shield for 1H weapons)
  [[nodiscard]] auto req_weapon_2() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x65);
  }

  // Shared cooldown group index (skills sharing the same cooldown pool)
  [[nodiscard]] auto shared_cooldown_group() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x8B);
  }

  // Flag: 1 if target must be explicitly selected, 0 for self/untargeted
  [[nodiscard]] auto needs_target() const -> bool {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
    return ext_client::off::field_at<std::uint8_t>(this, 0x98) != 0;
  }
};
