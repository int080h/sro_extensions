#pragma once

#include "sdk/game/cic_user.hpp"
#include "sdk/game/c_state_deliver.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// CICPlayer — Active local player character entity instance (g_pPlayer)
// Pointer: 0x01199114 | Gold: 0x0119B610 | Class Size: 0x3A94 (15000 bytes)
// Hierarchy: CObj -> CIEntity -> CIObject -> CIGidObject -> CICharactor -> CICUser -> CICPlayer
// ---------------------------------------------------------------------------
class cic_player : public cic_user, public c_state_deliver {
public:
  static constexpr std::uint32_t k_player_ptr_addr = 0x01199114;
  static constexpr std::uint32_t k_gold_addr       = 0x0119B610;
  static constexpr std::size_t   k_class_size      = 0x3A94;

  // 1. Instance Resolution
  static auto local() -> cic_player*;
  static auto is_valid(const cic_player* player) -> bool;

  // 2. Identity & Privileges
  auto name() const -> const wchar_t*;
  auto guild_name() const -> const wchar_t*;
  auto is_gamemaster() const -> bool;
  auto gm_authority() const -> std::uint32_t;

  // 3. Progression & Attributes
  auto level() const -> std::uint8_t;
  auto exp() const -> std::uint64_t;
  auto next_exp() const -> std::uint64_t;
  auto sp() const -> std::uint32_t;
  auto sp_exp() const -> std::uint32_t;
  auto gold() const -> std::uint64_t;
  auto strength() const -> std::uint16_t;
  auto intelligence() const -> std::uint16_t;
  auto attribute_points() const -> std::uint32_t;

  // 3A. Job & Honor Status
  auto job_alias() const -> const wchar_t*;
  auto job_type_id() const -> std::uint8_t;
  auto job_level() const -> std::uint8_t;
  auto job_exp() const -> std::uint64_t;
  auto job_max_exp() const -> std::uint64_t;
  auto job_exp_ratio() const -> float;
  auto honor_points_str() const -> std::string;

  // 3B. Combat Stats & Calculations (Physical & Magical Attack/Defense/Balance)
  struct s_combat_stats {
    std::int32_t  min_phy_atk{0};
    std::int32_t  max_phy_atk{0};
    std::int32_t  min_mag_atk{0};
    std::int32_t  max_mag_atk{0};
    std::uint16_t phy_def{0};
    std::uint16_t mag_def{0};
    std::uint16_t hit_rate{0};
    std::uint16_t parry_rate{0};
    float         phy_balance_pct{0.0f};
    float         mag_balance_pct{0.0f};
    float         raw_phy_balance_pct{0.0f};
    float         raw_mag_balance_pct{0.0f};
    std::uint32_t cur_hp{0};
    std::uint32_t max_hp{0};
    std::uint32_t cur_mp{0};
    std::uint32_t max_mp{0};
    std::uint16_t stat_points{0};
    std::uint16_t str{0};
    std::uint16_t int_{0};
    std::uint8_t  level{0};
    std::uint64_t exp{0};
    std::uint64_t next_exp{0};
    std::uint32_t sp{0};
    std::uint32_t sp_exp{0};
    std::uint64_t gold{0};
    std::uint8_t  hwan_level{0};
    std::uint8_t  pk_state{0};
    std::uint8_t  pvp_cape{0};
    std::wstring  job_alias{};
    std::string   job_name{};
    std::uint8_t  job_level{0};
    std::uint64_t job_exp{0};
    std::uint64_t job_max_exp{0};
    float         job_exp_ratio{0.0f};
    std::string   honor_points{};
  };
  auto get_combat_stats() const -> s_combat_stats;

  // 4. Equipment Inventory
  // 4A. Normal Equipment (13 slots at +0xA48, each slot size 0x1F8 = 504 bytes)
  // Slots: 0=Helm, 1=Chest, 2=Shoulder, 3=Hands, 4=Legs, 5=Feet, 6=Weapon, 7=Shield,
  //        8=Job Suit (Special Dress), 9=Earring, 10=Necklace, 11=Ring 1, 12=Ring 2
  struct s_equipped_item {
    std::uint32_t item_id{0};
    std::uint32_t ref_item_id{0};
    std::uint8_t  opt_level{0};
    std::uint32_t durability{0};
    std::uint32_t count{0};
  };
  auto get_equipped_item(std::uint8_t slot_idx) const -> class cso_item*;
  auto get_all_equipped_items() const -> std::vector<std::pair<std::uint8_t, class cso_item*>>;
  auto get_inventory_equipped_item(std::uint8_t slot_idx) const -> s_equipped_item;
  static auto equipment_slot_name_loc(std::uint8_t slot_idx) -> std::string;

  // 4B. Avatar Equipment (5 slots via m_mapAvatarItem at +0x23E0)
  // Slots: 0=Avatar Dress, 1=Avatar Attachment, 2=Avatar Hat, 3=Avatar Flag, 4=Devil's Spirit
  auto get_avatar_item(std::uint8_t slot_idx) const -> class cso_item*;
  auto get_all_avatar_items() const -> std::vector<std::pair<std::uint8_t, class cso_item*>>;
  static auto avatar_slot_name_loc(std::uint8_t slot_idx) -> std::string;

  // 4C. Job Equipment (11 slots at +0x2410, each slot size 0x1F8 = 504 bytes)
  // Slots: 0=Job Helm, 1=Job Chest, 2=Job Shoulder, 3=Job Hands, 4=Job Legs, 5=Job Feet,
  //        6=Job Weapon, 7=Job Earring, 8=Job Necklace, 9=Job Ring 1, 10=Job Ring 2
  auto get_job_equipped_item(std::uint8_t slot_idx) const -> class cso_item*;
  auto get_all_job_equipped_items() const -> std::vector<std::pair<std::uint8_t, class cso_item*>>;
  static auto job_slot_name_loc(std::uint8_t slot_idx) -> std::string;

  // 4D. Inventory Bag Items (Page 1..3 via CIFInventory at popup + 0x7C4)
  auto get_bag_item(std::size_t slot_idx) const -> class cso_item*;
  auto get_bag_item_count() const -> std::size_t;
  auto get_all_bag_items() const -> std::vector<std::pair<std::size_t, class cso_item*>>;

  // 5. Companion / Pet Manager (CCOSDataMgr at +0x3A48, sub_B36A90)
  auto get_cos_data_mgr() const -> class ccos_data_mgr*;
};
