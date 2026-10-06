#include "pch.hpp"
#include "sdk/game/cic_player.hpp"
#include "sdk/game/ccos_data_mgr.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/cif_main_popup.hpp"
#include "sdk/ui/cui_string_manager.hpp"

#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

// ===========================================================================
// 1. Instance Resolution
// ===========================================================================
auto cic_player::local() -> cic_player * {
  auto *ptr = *reinterpret_cast<cic_player **>(k_player_ptr_addr);
  if (!ext_client::utils::memory::is_game_ptr(ptr)) {
    return nullptr;
  }
  return ptr;
}

auto cic_player::is_valid(const cic_player *player) -> bool {
  return ext_client::utils::memory::is_game_ptr(player);
}

// ===========================================================================
// 2. Identity & Privileges
// ===========================================================================
auto cic_player::name() const -> const wchar_t * {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return L"";
  return user_name();
}

auto cic_player::guild_name() const -> const wchar_t * {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return L"";
  return user_guild_name();
}

auto cic_player::is_gamemaster() const -> bool {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return false;
  return gm_authority() == 0x10001 || cic_user::is_gamemaster();
}

auto cic_player::gm_authority() const -> std::uint32_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint32_t>(this, 0x3A58);
}

// ===========================================================================
// 3. Progression & Attributes
// ===========================================================================
auto cic_player::level() const -> std::uint8_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint8_t>(this, 0xA14);
}

auto cic_player::exp() const -> std::uint64_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint64_t>(this, 0xA18);
}

auto cic_player::sp() const -> std::uint32_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint32_t>(this, 0xA28);
}

auto cic_player::sp_exp() const -> std::uint32_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint32_t>(this, 0xA20);
}

auto cic_player::gold() const -> std::uint64_t {
  const auto *gold_ptr = reinterpret_cast<const std::uint64_t *>(k_gold_addr);
  if (!ext_client::utils::memory::is_readable_ptr(gold_ptr))
    return 0;
  return *gold_ptr;
}

auto cic_player::strength() const -> std::uint16_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint16_t>(this, 0xA24);
}

auto cic_player::intelligence() const -> std::uint16_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint16_t>(this, 0xA26);
}

auto cic_player::attribute_points() const -> std::uint32_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint32_t>(this, 0xA28);
}

auto cic_player::next_exp() const -> std::uint64_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  const auto lvl = level();
  if (lvl == 0)
    return 0;

  constexpr std::uintptr_t k_mgr = 0x0117EE20;
  constexpr std::uintptr_t k_func = 0x00A754C0;
  if (!ext_client::utils::memory::is_readable_ptr(reinterpret_cast<const void *>(k_mgr))) {
    return 0;
  }
  using fn_t = const void *(__thiscall *)(void *this_ptr, std::uint32_t lvl_arg);
  auto fn = reinterpret_cast<fn_t>(k_func);
  __try {
    const void *data_ptr = fn(reinterpret_cast<void *>(k_mgr), lvl);
    if (ext_client::utils::memory::is_readable_ptr(data_ptr)) {
      return *reinterpret_cast<const std::uint64_t *>(static_cast<const std::uint8_t *>(data_ptr) + 8);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
  return 0;
}

auto cic_player::job_alias() const -> const wchar_t * {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return L"";
  const auto *alias3a04 = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x3A04).data();
  if (alias3a04 && alias3a04[0] != L'\0') {
    return alias3a04;
  }
  const auto *alias8c8 = ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8C8).data();
  if (alias8c8 && alias8c8[0] != L'\0') {
    return alias8c8;
  }
  return L"";
}

auto cic_player::job_type_id() const -> std::uint8_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint8_t>(this, 0x3A24);
}

auto cic_player::job_level() const -> std::uint8_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  const auto jlvl = ext_client::off::field_at<std::uint8_t>(this, 0x3A25);
  if (jlvl > 0)
    return jlvl;
  return cic_user::job_level();
}

auto cic_player::job_exp() const -> std::uint64_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  return ext_client::off::field_at<std::uint64_t>(this, 0x3A28);
}

auto cic_player::job_max_exp() const -> std::uint64_t {
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return 0;
  const auto j_lvl = job_level();
  constexpr std::uintptr_t k_mgr = 0x0117EE20;
  constexpr std::uintptr_t k_func = 0x00A74550;
  if (!ext_client::utils::memory::is_readable_ptr(reinterpret_cast<const void *>(k_mgr))) {
    return 0;
  }
  using fn_t = const void *(__thiscall *)(void *this_ptr, std::uint32_t lvl_arg);
  auto fn = reinterpret_cast<fn_t>(k_func);
  __try {
    const void *data_ptr = fn(reinterpret_cast<void *>(k_mgr), j_lvl);
    if (ext_client::utils::memory::is_readable_ptr(data_ptr)) {
      return *reinterpret_cast<const std::uint64_t *>(data_ptr);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
  return 0;
}

auto cic_player::job_exp_ratio() const -> float {
  const auto cur = job_exp();
  const auto max_e = job_max_exp();
  if (max_e == 0)
    return 0.0f;
  const double ratio = static_cast<double>(cur) / static_cast<double>(max_e);
  return static_cast<float>(std::clamp(ratio, 0.0, 1.0));
}

static auto get_honor_points_internal(const cic_player *player) -> std::int32_t {
  constexpr std::uintptr_t k_check_func = 0x00AD68C0;
  constexpr std::uintptr_t k_block = 0x0117FD48;
  using check_fn_t = char(__thiscall *)(void *this_ptr);
  auto check_fn = reinterpret_cast<check_fn_t>(k_check_func);
  bool has_honor = false;
  __try {
    if (ext_client::utils::memory::is_readable_ptr(reinterpret_cast<const void *>(k_block))) {
      has_honor = (check_fn(reinterpret_cast<void *>(k_block)) != 0);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    has_honor = false;
  }

  if (!has_honor) {
    return -1;
  }

  constexpr std::uintptr_t k_fn1 = 0x00B22190;
  constexpr std::uintptr_t k_fn2 = 0x00AE1DE0;
  using fn1_t = void *(__thiscall *)(const void *this_ptr);
  using fn2_t = const void *(__thiscall *)(void *this_ptr);
  auto f1 = reinterpret_cast<fn1_t>(k_fn1);
  auto f2 = reinterpret_cast<fn2_t>(k_fn2);
  __try {
    void *p1 = f1(player);
    if (p1) {
      const void *p2 = f2(p1);
      if (p2) {
        return *reinterpret_cast<const std::int32_t *>(static_cast<const std::uint8_t *>(p2) + 92);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  return -1;
}

auto cic_player::honor_points_str() const -> std::string {
  const auto pts = get_honor_points_internal(this);
  if (pts < 0) {
    return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_TC_CAMP_NOT_JOIN", "N/A");
  }
  return std::to_string(pts);
}


auto cic_player::get_combat_stats() const -> s_combat_stats {
  s_combat_stats stats{};
  if (!ext_client::utils::memory::is_valid_ptr(this))
    return stats;

  stats.level = level();
  stats.exp = exp();
  stats.next_exp = next_exp();
  stats.sp = sp();
  stats.sp_exp = sp_exp();
  stats.gold = gold();
  stats.str = strength();
  stats.int_ = intelligence();
  stats.stat_points = ext_client::off::field_at<std::uint16_t>(this, 0xA2C);
  stats.cur_hp = get_hp();
  stats.max_hp = get_max_hp();
  stats.cur_mp = get_mp();
  stats.max_mp = get_max_mp();
  stats.hwan_level = hwan_level();
  stats.pk_state = pk_state();
  stats.pvp_cape = static_cast<std::uint8_t>(pvp_cape());

  stats.job_alias = job_alias();
  stats.job_name = job_name_loc();
  stats.job_level = job_level();
  stats.job_exp = job_exp();
  stats.job_max_exp = job_max_exp();
  stats.job_exp_ratio = job_exp_ratio();
  stats.honor_points = honor_points_str();

  // Physical & Magical Attack / Defense from global combat stats block (0x0117FD48 + 0x270)
  // NOTE: This global memory block is only populated for the local player's active character.
  // For other players, the server does not send combat attack/defense values or balance percentages.
  if (this == local()) {
    constexpr std::uintptr_t k_combat_stats_base = 0x0117FD48 + 0x270;
    if (ext_client::utils::memory::is_readable_ptr(reinterpret_cast<const void *>(k_combat_stats_base))) {
      stats.min_phy_atk = ext_client::off::field_at<std::int32_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x00);
      stats.max_phy_atk = ext_client::off::field_at<std::int32_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x04);
      stats.min_mag_atk = ext_client::off::field_at<std::int32_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x08);
      stats.max_mag_atk = ext_client::off::field_at<std::int32_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x0C);
      stats.phy_def     = ext_client::off::field_at<std::uint16_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x10);
      stats.mag_def     = ext_client::off::field_at<std::uint16_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x12);
      stats.hit_rate    = ext_client::off::field_at<std::uint16_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x14);
      stats.parry_rate  = ext_client::off::field_at<std::uint16_t>(reinterpret_cast<const void *>(k_combat_stats_base), 0x16);
    }

    // Exact engine balance formulas from sub_8A53A0
    const double lv_d = static_cast<double>(stats.level > 0 ? stats.level : 1);
    const double phy_denom = ((lv_d - 1.0) * 7.0 + 56.0) * 0.8571428656578064;
    if (phy_denom > 0.0) {
      const double phy_nom = (static_cast<double>(stats.str) + 2.0 * lv_d + 14.0) * 100.0;
      stats.raw_phy_balance_pct = static_cast<float>(phy_nom / phy_denom);
      stats.phy_balance_pct = (stats.raw_phy_balance_pct >= 120.0f) ? 120.0f : stats.raw_phy_balance_pct;
    }
    const double mag_denom = (lv_d + 7.0) * 5.0 * 0.8;
    if (mag_denom > 0.0) {
      const double mag_nom = static_cast<double>(stats.int_) * 100.0;
      stats.raw_mag_balance_pct = static_cast<float>(mag_nom / mag_denom);
      stats.mag_balance_pct = (stats.raw_mag_balance_pct >= 120.0f) ? 120.0f : stats.raw_mag_balance_pct;
    }
  }

  return stats;
}

#include "sdk/game/cso_item.hpp"

// ===========================================================================
// 4. Local Equipment & Inventory Slots
// ===========================================================================
auto cic_player::get_equipped_item(std::uint8_t slot_idx) const -> cso_item * {
  if (slot_idx >= 13 || !ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }

  // 1. If this is local player, check native live equipment on CIFInventory (sub_78BAD0 at +0x2450)
  // CIFInventory is updated immediately upon equipping/unequipping by sub_99DA60 / sub_78F5D0.
  if (this == local()) {
    if (auto *iface = cg_interface::get()) {
      if (auto *popup = cif_main_popup::from_interface(iface)) {
        const auto *inv = ext_client::off::field_at<const void *>(popup, 0x7C8);
        if (inv && ext_client::utils::memory::is_valid_ptr(inv)) {
          const auto item_addr =
              reinterpret_cast<std::uintptr_t>(inv) + 0x2450 +
              (static_cast<std::size_t>(slot_idx) * cso_item::k_class_size);
          if (ext_client::utils::memory::is_valid_ptr(reinterpret_cast<const void *>(item_addr))) {
            auto *item = reinterpret_cast<cso_item *>(item_addr);
            if (item && item->is_valid()) {
              return item;
            }
            // For the local player, CIFInventory is authoritative for all 13 equipment slots.
            // If the slot is empty/inactive in CIFInventory, return nullptr immediately so
            // stale 3D visual caches at this + 0xA48 are not displayed.
            return nullptr;
          }
        }
      }
    }
  }

  constexpr std::size_t k_slot_size =
      cso_item::k_class_size; // 0x1F8 (504 bytes)

  // 2. Remote player / fallback: Check via slot indirection table at this + 0xA38 (used natively by
  // sub_86FFE0)
  const auto internal_idx =
      ext_client::off::field_at<std::uint8_t>(this, 0xA38 + slot_idx);
  if (internal_idx < 13) {
    const auto slot_addr = reinterpret_cast<std::uintptr_t>(this) + 0xA48 +
                           (internal_idx * k_slot_size);
    auto *item = reinterpret_cast<cso_item *>(slot_addr);
    if (item && item->is_valid()) {
      return item;
    }
  }

  // 3. Direct slot index fallback
  const auto direct_addr =
      reinterpret_cast<std::uintptr_t>(this) + 0xA48 + (slot_idx * k_slot_size);
  auto *direct_item = reinterpret_cast<cso_item *>(direct_addr);
  if (direct_item && direct_item->is_valid()) {
    return direct_item;
  }

  return nullptr;
}

auto cic_player::get_all_equipped_items() const
    -> std::vector<std::pair<std::uint8_t, cso_item *>> {
  std::vector<std::pair<std::uint8_t, cso_item *>> result;
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return result;
  }
  for (std::uint8_t i = 0; i < 13; ++i) {
    if (auto *item = get_equipped_item(i)) {
      result.emplace_back(i, item);
    }
  }
  return result;
}

auto cic_player::get_inventory_equipped_item(std::uint8_t slot_idx) const
    -> s_equipped_item {
  s_equipped_item res{};
  auto *item = get_equipped_item(slot_idx);
  if (!item) {
    return res;
  }
  res.item_id = item->item_id();
  res.ref_item_id = item->ref_id();
  res.opt_level = item->opt_level();
  res.durability = item->durability();
  res.count = item->count();
  return res;
}

// ---------------------------------------------------------------------------
// 4B. Avatar Equipment (5 slots: Dress, Attachment, Hat, Flag, Nasrun)
// Native container:
// 1. Temporary packet staging map: std::map<uint8_t, cso_item*> at this + 0x23E0 (sub_78F9D0)
// 2. Live equipment container: CIFInventory::m_avatarItems[5] at CIFInventory + 0x470 (sub_78BC90)
// ---------------------------------------------------------------------------
auto cic_player::get_avatar_item(std::uint8_t slot_idx) const -> cso_item * {
  if (slot_idx >= 5 || !ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }

  // 1. If this is local player, check native live avatar equipment on CIFInventory (sub_78F9D0 / sub_78BC90 at +0x470)
  if (this == local()) {
    if (auto *iface = cg_interface::get()) {
      if (auto *popup = cif_main_popup::from_interface(iface)) {
        const auto *inv = ext_client::off::field_at<const void *>(popup, 0x7C8);
        if (inv && ext_client::utils::memory::is_valid_ptr(inv)) {
          const auto item_addr =
              reinterpret_cast<std::uintptr_t>(inv) + 0x470 +
              (static_cast<std::size_t>(slot_idx) * cso_item::k_class_size);
          if (ext_client::utils::memory::is_valid_ptr(reinterpret_cast<const void *>(item_addr))) {
            auto *item = reinterpret_cast<cso_item *>(item_addr);
            if (item && item->is_valid()) {
              return item;
            }
            return nullptr;
          }
        }
      }
    }
  }

  // 2. Check local player's staging / packet map at this + 0x23E0
  const auto avatar_map =
      ext_client::msvc9::map_view<std::uint8_t, cso_item *>::from_object(this, 0x23E0);
  if (auto *item_ptr = avatar_map.find_value(slot_idx)) {
    if (*item_ptr && (*item_ptr)->is_valid()) {
      return *item_ptr;
    }
  }

  return nullptr;
}

auto cic_player::get_all_avatar_items() const
    -> std::vector<std::pair<std::uint8_t, cso_item *>> {
  std::vector<std::pair<std::uint8_t, cso_item *>> result;
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return result;
  }
  for (std::uint8_t i = 0; i < 5; ++i) {
    if (auto *item = get_avatar_item(i)) {
      result.emplace_back(i, item);
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
// 4C. Job Equipment (11 slots at +0x2410, each slot size 0x1F8 = 504 bytes)
// Native container:
// 1. Live equipment container: CIFInventory::m_jobItems[11] at CIFInventory + 0xEA0 (sub_78F4D0)
// 2. Fallback / remote player: cso_item m_aJobItem[11] embedded array at +0x2410 (sub_B3CE80)
// ---------------------------------------------------------------------------
auto cic_player::get_job_equipped_item(std::uint8_t slot_idx) const
    -> cso_item * {
  if (slot_idx >= 11 || !ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }

  // 1. If this is local player, check native live job equipment on CIFInventory (sub_78F4D0 at +0xEA0)
  if (this == local()) {
    if (auto *iface = cg_interface::get()) {
      if (auto *popup = cif_main_popup::from_interface(iface)) {
        const auto *inv = ext_client::off::field_at<const void *>(popup, 0x7C8);
        if (inv && ext_client::utils::memory::is_valid_ptr(inv)) {
          const auto item_addr =
              reinterpret_cast<std::uintptr_t>(inv) + 0xEA0 +
              (static_cast<std::size_t>(slot_idx) * cso_item::k_class_size);
          if (ext_client::utils::memory::is_valid_ptr(reinterpret_cast<const void *>(item_addr))) {
            auto *item = reinterpret_cast<cso_item *>(item_addr);
            if (item && item->is_valid()) {
              return item;
            }
            return nullptr;
          }
        }
      }
    }
  }

  constexpr std::size_t k_slot_size =
      cso_item::k_class_size; // 0x1F8 (504 bytes)
  const auto slot_addr = reinterpret_cast<std::uintptr_t>(this) + 0x2410 +
                         (slot_idx * k_slot_size);
  auto *item = reinterpret_cast<cso_item *>(slot_addr);
  if (item && item->is_valid()) {
    return item;
  }

  return nullptr;
}

auto cic_player::get_all_job_equipped_items() const
    -> std::vector<std::pair<std::uint8_t, cso_item *>> {
  std::vector<std::pair<std::uint8_t, cso_item *>> result;
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return result;
  }
  for (std::uint8_t i = 0; i < 11; ++i) {
    if (auto *item = get_job_equipped_item(i)) {
      result.emplace_back(i, item);
    }
  }
  return result;
}

// ===========================================================================
// 5. Companion / Pet Manager (CCOSDataMgr)
// ===========================================================================
auto cic_player::get_cos_data_mgr() const -> ccos_data_mgr * {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }
  // Native CICPlayer::GetCOSDataMgr at 0x00B36A90: return *(this + 0x3A48);
  auto *mgr = ext_client::off::field_at<ccos_data_mgr *>(this, 0x3A48);
  return ext_client::utils::memory::is_valid_ptr(mgr) ? mgr : nullptr;
}

// ===========================================================================
// 6. Dynamic Native Localization for Slots (CUIStringManager / textdata)
// ===========================================================================
auto cic_player::equipment_slot_name_loc(std::uint8_t slot_idx) -> std::string {
  using ext_client::sdk::ui::get_string_utf8;
  switch (slot_idx) {
  case 0:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_HEAD");
  case 1:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_BREAST");
  case 2:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_SHOULDER");
  case 3:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_HAND");
  case 4:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_LEG");
  case 5:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_FOOT");
  case 6:
    return get_string_utf8(L"UIIT_STT_WEAPON_TYPE");
  case 7:
    return get_string_utf8(L"UIIT_STT_SHIELD");
  case 8:
    return get_string_utf8(L"UIIT_MSG_TC_ERROR_JOB_DRESS");
  case 9:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_EARRING");
  case 10:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_NECKLACE");
  case 11:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_RING") + " 1";
  case 12:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_RING") + " 2";
  default:
    return "Unknown";
  }
}

// Avatar slot order follows the item sub-type (sub_768770: tid4 == 2 is the
// attachment): 0=Dress, 1=Attachment, 2=Hat, 3=Flag, 4=Devil's Spirit (NASRUN)
auto cic_player::avatar_slot_name_loc(std::uint8_t slot_idx) -> std::string {
  using ext_client::sdk::ui::get_string_utf8;
  switch (slot_idx) {
  case 0:
    return get_string_utf8(L"UIIT_STT_SILKMALL_DRESS");
  case 1:
    return get_string_utf8(L"UIIT_STT_SILKMALL_ATTACH");
  case 2:
    return get_string_utf8(L"UIIT_STT_SILKMALL_HAT");
  case 3:
    return get_string_utf8(L"UIIT_STT_ARENA_FLAG");
  case 4:
    return get_string_utf8(L"UIIT_CTL_WK_COSTUME_NASRUN");
  default:
    return "Unknown";
  }
}

auto cic_player::job_slot_name_loc(std::uint8_t slot_idx) -> std::string {
  using ext_client::sdk::ui::get_string_utf8;
  switch (slot_idx) {
  case 0:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_HEAD");
  case 1:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_BREAST");
  case 2:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_SHOULDER");
  case 3:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_HAND");
  case 4:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_LEG");
  case 5:
    return get_string_utf8(L"UIIT_STT_ARMOR_POSITION_FOOT");
  case 6:
    return get_string_utf8(L"UIIT_STT_WEAPON_TYPE");
  case 7:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_EARRING");
  case 8:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_NECKLACE");
  case 9:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_RING") + " 1";
  case 10:
    return get_string_utf8(L"UIIT_STT_ACCESSARY_RING") + " 2";
  default:
    return "Unknown";
  }
}

// ===========================================================================
// 4D. Inventory Bag Items (Page 1..3 via CIFInventory at popup + 0x7C4)
// ===========================================================================
auto cic_player::get_bag_item(std::size_t slot_idx) const -> cso_item * {
  if (!ext_client::utils::memory::is_valid_ptr(this) || this != local())
    return nullptr;
  if (auto *iface = cg_interface::get()) {
    if (auto *popup = cif_main_popup::from_interface(iface)) {
      const auto *inv_bag = ext_client::off::field_at<const void *>(popup, 0x7C4);
      if (inv_bag && ext_client::utils::memory::is_valid_ptr(inv_bag)) {
        auto *const *first = ext_client::off::field_at<cso_item *const *>(inv_bag, 0x3A8);
        auto *const *last  = ext_client::off::field_at<cso_item *const *>(inv_bag, 0x3AC);
        if (first && last && last >= first) {
          const auto count = static_cast<std::size_t>(last - first);
          if (slot_idx < count) {
            auto *item = first[slot_idx];
            if (item && item->is_valid())
              return item;
          }
        }
      }
    }
  }
  return nullptr;
}

auto cic_player::get_bag_item_count() const -> std::size_t {
  if (!ext_client::utils::memory::is_valid_ptr(this) || this != local())
    return 0;
  if (auto *iface = cg_interface::get()) {
    if (auto *popup = cif_main_popup::from_interface(iface)) {
      const auto *inv_bag = ext_client::off::field_at<const void *>(popup, 0x7C4);
      if (inv_bag && ext_client::utils::memory::is_valid_ptr(inv_bag)) {
        auto *const *first = ext_client::off::field_at<cso_item *const *>(inv_bag, 0x3A8);
        auto *const *last  = ext_client::off::field_at<cso_item *const *>(inv_bag, 0x3AC);
        if (first && last && last >= first) {
          return static_cast<std::size_t>(last - first);
        }
      }
    }
  }
  return 0;
}

auto cic_player::get_all_bag_items() const -> std::vector<std::pair<std::size_t, cso_item *>> {
  std::vector<std::pair<std::size_t, cso_item *>> result;
  if (!ext_client::utils::memory::is_valid_ptr(this) || this != local())
    return result;
  const auto total = get_bag_item_count();
  result.reserve(total);
  for (std::size_t i = 0; i < total; ++i) {
    if (auto *item = get_bag_item(i)) {
      result.emplace_back(i, item);
    }
  }
  return result;
}
