#pragma once

#include "sdk/game/ci_charactor.hpp"
#include "sdk/game/ccompound_obj.hpp"
#include "sdk/ui/cui_string_manager.hpp"
#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CICUser — User character game-object base class (base of CICPlayer)
// Size: 0x9F0 (2544 bytes) | Extends CICharactor
// Global Local Player Pointer: 0x01199114
// ---------------------------------------------------------------------------
class cic_user : public ci_charactor {
public:
  static constexpr std::size_t k_class_size = 0x9F0;
  static constexpr std::uint32_t k_local_player_addr = 0x01199114;

  // 1. Singleton / Local Player Access
  [[nodiscard]] static auto get_local_player() -> cic_user* {
    auto* player = ext_client::off::global_at<cic_user*>(k_local_player_addr);
    if (!ext_client::utils::memory::is_game_ptr(player)) {
      return nullptr;
    }
    return player;
  }

  // 2. Identity & Names
  [[nodiscard]] auto user_name() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    // Check job alias (0x8C8) if in job mode, else fallback to character name (0x110)
    const auto* alias = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8C8).data();
    if (alias && alias[0] != L'\0') {
      return alias;
    }
    const auto* cname = character_name();
    return cname ? cname : L"";
  }

  [[nodiscard]] auto character_name() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    const auto* name = get_display_name();
    return name ? name : L"";
  }

  [[nodiscard]] auto job_alias() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    const auto* alias = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8C8).data();
    return alias ? alias : L"";
  }

  [[nodiscard]] auto stall_title() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    const auto* title = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x9D0).data();
    return title ? title : L"";
  }

  // 3. Guild & Union Information
  [[nodiscard]] auto guild_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x8E0);
  }

  [[nodiscard]] auto user_guild_name() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    // Guild name wstring object is at 0x8F8 (buffer at 0x8FC, size at 0x90C, capacity at 0x910)
    const auto* gname = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8F8).data();
    return gname ? gname : L"";
  }

  [[nodiscard]] auto grant_name() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    // Grant name wstring object is at 0x914 (buffer at 0x918, size at 0x928, capacity at 0x92C)
    const auto* gname = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x914).data();
    return gname ? gname : L"";
  }

  [[nodiscard]] auto fortress_title() const -> const wchar_t* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    // Fortress war title wstring object is at 0x930 (buffer at 0x934, size at 0x944, capacity at 0x948)
    const auto* ftitle = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x930).data();
    return ftitle ? ftitle : L"";
  }

  [[nodiscard]] auto has_guild() const -> bool {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
    const auto* gname = user_guild_name();
    return (gname && gname[0] != L'\0');
  }

  // 4. Job System
  enum class job_type_e : std::uint8_t {
    none   = 0,
    trader = 1,
    thief  = 2,
    hunter = 3
  };

  [[nodiscard]] auto job_type() const -> job_type_e {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return job_type_e::none;
    const auto raw = ext_client::off::field_at<std::uint8_t>(this, 0x8F1);
    return (raw <= 3) ? static_cast<job_type_e>(raw) : job_type_e::none;
  }

  [[nodiscard]] auto job_level() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x575);
  }

  [[nodiscard]] auto is_in_job_mode() const -> bool {
    return job_type() != job_type_e::none;
  }

  [[nodiscard]] auto job_name_loc() const -> std::string {
    using ext_client::sdk::ui::get_string_utf8;
    switch (job_type()) {
      case job_type_e::trader: return get_string_utf8(L"UIIT_CTL_CONFLICT_MERCHANT");
      case job_type_e::thief:  return get_string_utf8(L"UIIT_CTL_CONFLICT_THIEF");
      case job_type_e::hunter: return get_string_utf8(L"UIIT_CTL_CONFLICT_HUNTER");
      default:                 return "";
    }
  }

  // 5. PVP & PK Status
  enum class pvp_cape_e : std::uint8_t {
    none   = 0,
    red    = 1,
    blue   = 2,
    black  = 3,
    white  = 4,
    yellow = 5
  };

  [[nodiscard]] auto pvp_cape() const -> pvp_cape_e {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return pvp_cape_e::none;
    const auto raw = ext_client::off::field_at<std::uint8_t>(this, 0x8ED);
    return (raw <= 5) ? static_cast<pvp_cape_e>(raw) : pvp_cape_e::none;
  }

  [[nodiscard]] auto is_in_pvp() const -> bool {
    return pvp_cape() != pvp_cape_e::none;
  }

  [[nodiscard]] auto pvp_cape_name_loc() const -> std::string {
    using ext_client::sdk::ui::get_string_utf8;
    switch (pvp_cape()) {
      case pvp_cape_e::red:    return get_string_utf8(L"UIIT_STT_FRPVP_RED");
      case pvp_cape_e::blue:   return get_string_utf8(L"UIIT_STT_FRPVP_BLUE");
      case pvp_cape_e::black:  return get_string_utf8(L"UIIT_STT_FRPVP_GRAY");
      case pvp_cape_e::white:  return get_string_utf8(L"UIIT_STT_FRPVP_WHITE");
      case pvp_cape_e::yellow: return get_string_utf8(L"UIIT_STT_FRPVP_YELLOW");
      default:                 return "";
    }
  }

  [[nodiscard]] auto pk_state() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x8EC);
  }

  [[nodiscard]] auto is_murderer() const -> bool {
    return pk_state() > 2;
  }

  [[nodiscard]] auto is_assailant() const -> bool {
    return pk_state() == 2;
  }

  [[nodiscard]] auto hwan_level() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x8EE);
  }

  [[nodiscard]] auto pvp_team() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x8EF);
  }

  [[nodiscard]] auto is_gamemaster() const -> bool {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
    const auto gm_flag = ext_client::off::field_at<std::uint8_t>(this, 0x8F0);
    const auto state_flag = ext_client::off::field_at<std::uint8_t>(this, 0x8E5);
    return gm_flag == 1 || (state_flag & 0x10) != 0;
  }

  // 6. Stall & Merchant Mode
  enum class stall_state_e : std::uint8_t {
    none   = 0,
    open   = 1,
    modify = 2
  };

  [[nodiscard]] auto stall_state() const -> stall_state_e {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return stall_state_e::none;
    const auto raw = ext_client::off::field_at<std::uint8_t>(this, 0x94D);
    return (raw <= 2) ? static_cast<stall_state_e>(raw) : stall_state_e::none;
  }

  [[nodiscard]] auto is_stall_open() const -> bool {
    return stall_state() != stall_state_e::none;
  }

  // 7. Visual Equipment Inspection (Slots 0..8 via CCompoundObj)
  // 0: Helm, 1: Chest, 2: Shoulder, 3: Hands, 4: Legs, 5: Feet, 6: Weapon, 7: Shield, 8: Avatar
  enum class visual_slot_e : std::uint8_t {
    helm     = 0,
    chest    = 1,
    shoulder = 2,
    hands    = 3,
    legs     = 4,
    feet     = 5,
    weapon   = 6,
    shield   = 7,
    avatar   = 8,
    count    = 9
  };

  struct s_visual_equip {
    std::uint32_t ref_item_id{0};
    std::uint8_t  plus_opt{0};
  };

  [[nodiscard]] auto get_visual_equip(visual_slot_e slot) const -> s_visual_equip {
    return get_visual_equip(static_cast<std::uint8_t>(slot));
  }

  [[nodiscard]] auto get_visual_equip(std::uint8_t slot_idx) const -> s_visual_equip {
    s_visual_equip out{};
    if (slot_idx >= 9 || !ext_client::utils::memory::is_valid_ptr(this)) {
      return out;
    }
    const auto* holder = ext_client::off::field_at<const void*>(this, 0x09C);
    if (!ext_client::utils::memory::is_valid_ptr(holder)) {
      return out;
    }
    const auto* compound_obj = *reinterpret_cast<const void* const*>(reinterpret_cast<std::uintptr_t>(holder) + 0x4);
    if (!ext_client::utils::memory::is_valid_ptr(compound_obj)) {
      return out;
    }
    out.ref_item_id = *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<std::uintptr_t>(compound_obj) + 0x38 + slot_idx * 12);
    out.plus_opt    = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(compound_obj) + 0x3C + slot_idx * 12);
    return out;
  }

  [[nodiscard]] static auto visual_slot_name_loc(std::uint8_t slot_idx) -> std::string {
    using ext_client::sdk::ui::get_string_utf8;
    switch (slot_idx) {
      case 0: return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_HEAD");
      case 1: return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_BREAST");
      case 2: return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_SHOULDER");
      case 3: return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_HAND");
      case 4: return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_LEG");
      case 5: return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_FOOT");
      case 6: return get_string_utf8(L"UIIT_STT_WEAPON_TYPE");
      case 7: return get_string_utf8(L"UIIT_STT_SHIELD");
      case 8: return get_string_utf8(L"UIIT_STT_SILKMALL_DRESS");
      default: return "Unknown";
    }
  }

  // 8. Status & Vitals Helpers
  [[nodiscard]] auto is_alive() const -> bool {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
    return (ext_client::off::field_at<std::uint8_t>(this, 0x768) & 1) != 0;
  }

  [[nodiscard]] auto is_in_combat() const -> bool {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
    return (ext_client::off::field_at<std::uint8_t>(this, 0x574) & 1) != 0;
  }

  [[nodiscard]] auto abnormal_state() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x3C8);
  }

  [[nodiscard]] auto has_bad_status() const -> bool {
    // 0x17FAFFF matches native sub_926A10 pill condition
    return (abnormal_state() & 0x017FAFFFu) != 0;
  }

  [[nodiscard]] auto hp_percent() const -> float {
    const auto max_val = get_max_hp();
    if (max_val == 0) {
      return 0.0f;
    }
    return (static_cast<float>(get_hp()) / static_cast<float>(max_val)) * 100.0f;
  }

  [[nodiscard]] auto mp_percent() const -> float {
    const auto max_val = get_max_mp();
    if (max_val == 0) {
      return 0.0f;
    }
    return (static_cast<float>(get_mp()) / static_cast<float>(max_val)) * 100.0f;
  }

  // 9. Speeds
  [[nodiscard]] auto walk_speed() const -> float {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0.0f;
    return ext_client::off::field_at<float>(this, 0x358);
  }

  [[nodiscard]] auto run_speed() const -> float {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0.0f;
    return ext_client::off::field_at<float>(this, 0x35C);
  }

  [[nodiscard]] auto berserk_speed() const -> float {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0.0f;
    return ext_client::off::field_at<float>(this, 0x5F4);
  }
};
