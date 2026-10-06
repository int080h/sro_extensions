#pragma once

#include "sdk/game/cso_item.hpp"
#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

// ---------------------------------------------------------------------------
// SCOSInfo / SCOS2Info — Companion / Pet runtime data structure
// Base class: __COSInfoBase (Vftable: 0x00FC297C)
// SCOSInfo (General/Growth/Ability/Ride): 0x00FC2934 (sub_4D03F0)
// SCOS2Info (Fellow Pet): 0x00FC27C4 (sub_4C2ED0)
// Deserializers: sub_4CF5F0 (SCOSInfo), sub_4C2650 (SCOS2Info)
// ---------------------------------------------------------------------------
class scos_info {
public:
  [[nodiscard]] auto cos_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x08);
  }

  [[nodiscard]] auto ref_obj_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x0C);
  }

  // Pet Given Name (msvc9::wstring at +0x10)
  [[nodiscard]] auto name() const -> const wchar_t * {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return L"";
    const auto ref = ext_client::msvc9::wstring_ref::from(
        reinterpret_cast<const std::uint8_t *>(this) + 0x10);
    const auto *d = ref.data();
    return (d && *d != L'\0') ? d : L"";
  }

  [[nodiscard]] auto current_hp() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x30);
  }

  [[nodiscard]] auto max_hp() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x34);
  }

  // Satiety / HGP (Hunger & Growth Points, 0 to 10000 = 0.00% to 100.00%)
  [[nodiscard]] auto satiety() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x38);
  }

  [[nodiscard]] auto satiety_percent() const -> float {
    return std::clamp(static_cast<float>(satiety()) / 100.0f, 0.0f, 100.0f);
  }

  [[nodiscard]] auto level() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x3C);
  }

  [[nodiscard]] auto type4() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x3E);
  }

  // Auto-pickup settings bitmask for Ability/Grab pets (+0x40)
  [[nodiscard]] auto pickup_flags() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x40);
  }

  // Current EXP (64-bit unsigned integer at +0x48)
  [[nodiscard]] auto current_exp() const -> std::uint64_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint64_t>(this, 0x48);
  }

  // Max EXP for current level (64-bit unsigned integer at +0x50)
  [[nodiscard]] auto max_exp() const -> std::uint64_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint64_t>(this, 0x50);
  }

  [[nodiscard]] auto exp_percent() const -> float {
    const auto cur = current_exp();
    const auto mx = max_exp();
    if (mx == 0)
      return 0.0f;
    return static_cast<float>(
        std::clamp((static_cast<double>(cur) / static_cast<double>(mx)) * 100.0,
                   0.0, 100.0));
  }

  // Fellow Pet Stored SP (Skill Points at +0xF8, populated by sub_4C2650)
  [[nodiscard]] auto stored_sp() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0xF8);
  }

  // Fellow Pet Available Stat / Skill Points (+0xFC)
  [[nodiscard]] auto stat_points() const -> std::uint16_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint16_t>(this, 0xFC);
  }

  // Pet Inventory & Equipment Item Accessor
  // SCOSInfo: 56 slots across 2 pages starting at +0xD8 (sub_4CEDD0 / sub_4CEC80)
  // SCOS2Info (Fellow Pet): 14 slots starting at +0x100 (sub_4C2D30)
  // Each slot is a complete cso_item (0x1F8 = 504 bytes)
  [[nodiscard]] auto get_item(std::uint8_t slot) const -> cso_item * {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return nullptr;
    const bool is_fellow = is_fellow_pet();
    const std::uint8_t max_slots = is_fellow ? 14 : 56;
    if (slot >= max_slots)
      return nullptr;

    const std::size_t base_offset = is_fellow ? 0x100 : 0xD8;
    const std::uintptr_t item_addr =
        reinterpret_cast<std::uintptr_t>(this) + base_offset +
        (static_cast<std::size_t>(slot) * cso_item::k_class_size);
    if (!ext_client::utils::memory::is_valid_ptr(
            reinterpret_cast<void *>(item_addr))) {
      return nullptr;
    }
    auto *item = reinterpret_cast<cso_item *>(item_addr);
    return (item && item->is_valid()) ? item : nullptr;
  }

  [[nodiscard]] auto slot_count() const -> std::uint8_t {
    return is_fellow_pet() ? 14 : 56;
  }

  // Fellow Pet Learned Skills
  // Located at SCOS2Info + 0x1C9C (std::map<uint32_t, void*> populated by sub_4C21E0)
  [[nodiscard]] auto get_fellow_skills() const -> std::vector<std::uint32_t> {
    std::vector<std::uint32_t> skills;
    if (!is_fellow_pet() || !ext_client::utils::memory::is_valid_ptr(this)) {
      return skills;
    }
    const auto skill_map =
        ext_client::msvc9::map_view<std::uint32_t, void *>::from_object(this, 0x1C9C);
    for (const auto &[skill_id, _] : skill_map) {
      if (skill_id > 0) {
        skills.push_back(skill_id);
      }
    }
    return skills;
  }

  [[nodiscard]] auto is_fellow_pet() const -> bool { return type4() == 9; }

  [[nodiscard]] auto is_growth_pet() const -> bool { return type4() == 3; }

  [[nodiscard]] auto is_ride_pet() const -> bool { return type4() == 2; }

  [[nodiscard]] auto is_cash_pet() const -> bool { return type4() == 4; }

  [[nodiscard]] auto is_mercenary() const -> bool { return type4() == 5; }
};

// ---------------------------------------------------------------------------
// CCOSDataMgr — Game Client Companion & Pet Data Manager
// Vtable: 0x00FCFF38 | Instance: CICPlayer + 0x3A48 (sub_B36A90)
// Container: std::map<uint32_t, SCOSInfo*> at this + 0x08 (sub_580B90 / sub_57F470)
// ---------------------------------------------------------------------------
class ccos_data_mgr {
public:
  static constexpr std::uintptr_t k_vftable_addr = 0x00FCFF38;
  static constexpr std::uintptr_t k_find_cos_fn_addr = 0x004C0FD0;
  static constexpr std::uintptr_t k_count_type_fn_addr = 0x0057F5B0;

  // 1. Native Companion Lookup (FindCOS at 0x004C0FD0)
  [[nodiscard]] auto find_cos(std::uint32_t cos_id) -> scos_info * {
    if (!ext_client::utils::memory::is_valid_ptr(this) || cos_id == 0) {
      return nullptr;
    }
    using find_cos_fn =
        scos_info *(__thiscall *)(ccos_data_mgr *, std::uint32_t);
    const auto fn = ext_client::off::as_fn<find_cos_fn>(k_find_cos_fn_addr);
    if (!fn) {
      return nullptr;
    }
    auto *result = fn(this, cos_id);
    return ext_client::utils::memory::is_valid_ptr(result) ? result : nullptr;
  }

  // 2. Active Companion ID (stored at +0x14, updated by sub_907820)
  [[nodiscard]] auto get_active_cos_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x14);
  }

  [[nodiscard]] auto get_active_cos() -> scos_info * {
    const auto active_id = get_active_cos_id();
    return (active_id != 0) ? find_cos(active_id) : nullptr;
  }

  // 3. Count companions by category (Type4: 9=fellow, 3=growth, 2=ride,
  // 5=mercenary)
  [[nodiscard]] auto count_by_type(std::uint8_t type4) -> int {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    using count_type_fn = int(__thiscall *)(ccos_data_mgr *, std::uint8_t);
    const auto fn = ext_client::off::as_fn<count_type_fn>(k_count_type_fn_addr);
    return fn ? fn(this, type4) : 0;
  }

  // 4. Native std::map<uint32_t, scos_info*> active pets iterator (+0x08)
  [[nodiscard]] auto pet_count() const -> std::size_t {
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x10);
  }

  [[nodiscard]] auto get_all_pets() const
      -> std::vector<std::pair<std::uint32_t, scos_info *>> {
    std::vector<std::pair<std::uint32_t, scos_info *>> pets;
    if (!ext_client::utils::memory::is_valid_ptr(this))
      return pets;
    const auto pet_map =
        ext_client::msvc9::map_view<std::uint32_t, scos_info *>::from_object(this, 0x08);
    for (const auto &[cos_id, info] : pet_map) {
      if (info && ext_client::utils::memory::is_valid_ptr(info)) {
        pets.emplace_back(cos_id, info);
      }
    }
    return pets;
  }
};
