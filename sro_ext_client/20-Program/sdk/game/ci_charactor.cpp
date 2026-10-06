#include "pch.hpp"
#include "sdk/game/ci_charactor.hpp"
#include "sdk/game/cic_player.hpp"
#include "sdk/game/ccos_data_mgr.hpp"
#include "sdk/ui/cui_string_manager.hpp"

#include "sdk/game/ccompound_obj.hpp"
#include "sdk/runtime/gfx_runtime.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

// ===========================================================================
// 1. Entity Identity & Position
// ===========================================================================
auto ci_charactor::get_compound_obj() -> ccompound_obj* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }
  // Visual holder lives at +0x9C. The CCompoundObj is the pointer at holder+0x4
  // (CIObject render path, sub_B25A10). Offset +0x4 on the character itself is not the model.
  auto* holder = ext_client::off::field_at<void*>(this, 0x09C);
  if (!ext_client::utils::memory::is_valid_ptr(holder)) {
    return nullptr;
  }
  auto* compound = ext_client::off::field_at<ccompound_obj*>(holder, 0x004);
  if (!ext_client::utils::memory::is_valid_ptr(compound)) {
    return nullptr;
  }
  return compound;
}

auto ci_charactor::get_position() const -> const s_position* {
  return &ext_client::off::field_at<s_position>(this, 0x07C);
}

auto ci_charactor::get_unique_id() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x010);
}

auto ci_charactor::get_display_name() const -> const wchar_t* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }

  // 1. For Player characters (CICUser / CICPlayer), the character's unique in-game name
  // is stored as an msvc9::wstring at this + 0x8CC. Check this FIRST so it is never
  // overridden by character species template descriptors (e.g. "CHAR_CH_MAN").
  if (is_player()) {
    const auto* user_name = &ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8C8);
    if (!user_name->empty() && user_name->c_str() && user_name->c_str()[0] != L'\0') {
      return user_name->c_str();
    }
  }

  // 2. Check CICharactor::m_name at this + 0x110 (stored as msvc9::wstring).
  // Inherited by CICharactor and populated for named pets and custom entities.
  const auto* name_wstr = &ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x110);
  if (!name_wstr->empty() && name_wstr->c_str() && name_wstr->c_str()[0] != L'\0') {
    return name_wstr->c_str();
  }

  // 2. Check CRefObjChar* cached on this entity at 0x76C or 0x2D8.
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }

  // 3. Fallback: lookup CRefObjChar from global CRefObjItemManager (0x0117EE20) by ref_id.
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto ref_id = get_refobj_id();
    if (ref_id > 0) {
      using lookup_thiscall_fn = void*(__thiscall*)(void* mgr, std::uint32_t id);
      const auto fn = ext_client::off::as_fn<lookup_thiscall_fn>(0x00A93E20);
      if (fn) {
        ref = fn(reinterpret_cast<void*>(0x0117EE20), ref_id);
      }
    }
  }

  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    // 4. Try CTextStringManager::GetString(0x0117EDA8, &ref->m_szNameStrID).
    // ref + 0x60 is m_szNameStrID (ext_client::msvc9::wstring).
    const auto* name_id = &ext_client::off::field_at<ext_client::msvc9::wstring>(ref, 0x060);
    if (!name_id->empty() && name_id->c_str()) {
      using get_string_fn = const ext_client::msvc9::wstring*(__thiscall*)(void* mgr, const ext_client::msvc9::wstring* id);
      const auto get_str = ext_client::off::as_fn<get_string_fn>(0x009E4E80);
      if (get_str) {
        void* tsm = reinterpret_cast<void*>(0x0117EDA8);
        const auto* localized = get_str(tsm, name_id);
        if (ext_client::utils::memory::is_valid_ptr(localized) &&
            !localized->empty() && localized->c_str() && localized->c_str()[0] != L'\0') {
          return localized->c_str();
        }
      }
    }

    // 5. Fallback: CodeName at ref + 0x08 (ext_client::msvc9::wstring).
    const auto* codename = &ext_client::off::field_at<ext_client::msvc9::wstring>(ref, 0x008);
    if (!codename->empty() && codename->c_str()) {
      return codename->c_str();
    }
  }

  // 6. Legacy fallback: check 0x8CC if valid.
  const auto* legacy_name = &ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8CC);
  if (!legacy_name->empty() && legacy_name->c_str() && legacy_name->c_str()[0] != L'\0') {
    return legacy_name->c_str();
  }

  return nullptr;
}

// ===========================================================================
// 2. Vitals & Combat Stats
// ===========================================================================
auto ci_charactor::get_hp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x554);
}

auto ci_charactor::get_display_hp() const -> std::uint32_t {
  if (!this) {
    return 0;
  }
  using display_hp_fn = std::uint32_t(__thiscall*)(const ci_charactor*);
  const auto fn = ext_client::off::as_fn<display_hp_fn>(0x00B287B0);
  return fn ? fn(this) : 0;
}

auto ci_charactor::get_mp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x558);
}

auto ci_charactor::get_max_hp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x55C);
}

auto ci_charactor::get_max_mp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x560);
}

auto ci_charactor::set_hp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_hp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_hp_func = reinterpret_cast<set_hp_fn>(vtable[42/*set_hp*/]);
  if (set_hp_func) {
    set_hp_func(this, val);
  }
}

auto ci_charactor::set_mp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_mp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_mp_func = reinterpret_cast<set_mp_fn>(vtable[43/*set_mp*/]);
  if (set_mp_func) {
    set_mp_func(this, val);
  }
}

auto ci_charactor::set_max_hp(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x55C) = val;
}

auto ci_charactor::set_max_mp(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x560) = val;
}

// ===========================================================================
// 3. State & Timers
// ===========================================================================
auto ci_charactor::get_state_619() const -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x619);
}

auto ci_charactor::get_idle_timer() const -> int {
  return ext_client::off::field_at<int>(this, 0x5E8);
}

auto ci_charactor::get_state_mask() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x618);
}

auto ci_charactor::set_state_619(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x619) = val;
}

auto ci_charactor::set_idle_timer(int val) -> void {
  ext_client::off::field_at<int>(this, 0x5E8) = val;
}

auto ci_charactor::set_state_mask(std::uint16_t val) -> void {
  ext_client::off::field_at<std::uint16_t>(this, 0x618) = val;
}

// ===========================================================================
// 4. Actions & Emotes
// ===========================================================================
auto ci_charactor::play_emote(unsigned char action_type, int emote_id) -> bool {
  using play_emote_fn = char(__thiscall*)(ci_charactor* this_ptr, unsigned char action_type, int emote_id);
  const auto play_emote_func = reinterpret_cast<play_emote_fn>(0x00B2A9B0);
  return play_emote_func && play_emote_func(this, action_type, emote_id) != 0;
}

// ===========================================================================
// 5. Classification & Rarity
// ===========================================================================
auto ci_charactor::get_refobj_id() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x2D4);
}

auto ci_charactor::get_cos_id() const -> std::uint32_t {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return 0;
  }
  return ext_client::off::field_at<std::uint32_t>(this, 0xF8);
}

auto ci_charactor::get_level() const -> std::uint32_t {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return 0;
  }

  // 1. Pet / COS companion: Query CCOSDataMgr using the game's native COS Manager
  if (is_pet()) {
    auto* player = cic_player::local();
    if (player) {
      auto* cos_mgr = player->get_cos_data_mgr();
      if (cos_mgr) {
        // Look up by COS ID (this + 0xF8), exactly as the game engine does in sub_B331F0 / sub_907820
        const auto cos_id = get_cos_id();
        const auto* pet_info = (cos_id != 0) ? cos_mgr->find_cos(cos_id) : nullptr;
        if (!pet_info) {
          // If this is our active summoned companion, query active COS record
          pet_info = cos_mgr->get_active_cos();
        }
        if (!pet_info) {
          pet_info = cos_mgr->find_cos(get_unique_id());
        }
        if (pet_info) {
          const auto lvl = pet_info->level();
          if (lvl > 0 && lvl <= 150) {
            return lvl;
          }
        }
      }
    }
  }

  // 2. Player: check local player (+0xA14) or remote player (+0x8F0)
  if (is_player()) {
    const auto ply_lvl = ext_client::off::field_at<std::uint8_t>(this, 0xA14);
    if (ply_lvl > 0 && ply_lvl <= 200) {
      return ply_lvl;
    }
    const auto remote_lvl = ext_client::off::field_at<std::uint8_t>(this, 0x8F0);
    if (remote_lvl > 0 && remote_lvl <= 200) {
      return remote_lvl;
    }
  }

  // 3. Fallback or Monster/NPC: Query CRefObjChar (+0x1C0)
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto ref_id = get_refobj_id();
    if (ref_id > 0) {
      using lookup_thiscall_fn = void*(__thiscall*)(void* mgr, std::uint32_t id);
      const auto fn = ext_client::off::as_fn<lookup_thiscall_fn>(0x00A93E20);
      if (fn) {
        ref = fn(reinterpret_cast<void*>(0x0117EE20), ref_id);
      }
    }
  }

  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto lvl = ext_client::off::field_at<std::uint32_t>(ref, 0x1C0);
    if (lvl > 0 && lvl <= 200) {
      return lvl;
    }
  }

  return 0;
}

auto ci_charactor::is_player() const -> bool {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return false;
  }

  // Safe read vtable pointer
  std::uintptr_t vt = 0;
  if (!ext_client::utils::memory::safe_read(this, vt)) {
    return false;
  }

  // 1. Ultra-fast direct vtable matching: CICUser (0x0104214C) or CICPlayer (0x01041EBC)
  if (vt == 0x0104214Cu || vt == 0x01041EBCu) {
    return true;
  }

  // 2. Native GFX engine runtime class check: sub_D62670(this, 0x0119B6F0) [GFX_RUNTIME_CLASS(CICUser)]
  using is_kind_of_engine_fn = char(__thiscall*)(const void* obj, const void* runtime_class);
  const auto engine_is_kind_of = ext_client::off::as_fn<is_kind_of_engine_fn>(0x00D62670);
  if (engine_is_kind_of) {
    __try {
      if (engine_is_kind_of(this, reinterpret_cast<const void*>(0x0119B6F0))) {
        return true;
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      return false;
    }
  }

  // 3. Fallback to RTTI check
  return ext_client::rtti::is_kind_of(this, "CICUser") ||
         ext_client::rtti::is_kind_of(this, "CICPlayer");
}

auto ci_charactor::is_npc() const -> bool {
  if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
  std::uintptr_t vt = 0;
  if (!ext_client::utils::memory::safe_read(this, vt)) return false;
  return ext_client::rtti::is_kind_of(this, "CICNPC") ||
         ext_client::rtti::is_kind_of(this, "CICMonsterNPC");
}

auto ci_charactor::is_guard() const -> bool {
  if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
  std::uintptr_t vt = 0;
  if (!ext_client::utils::memory::safe_read(this, vt)) return false;
  return ext_client::rtti::is_kind_of(this, "CICGuard");
}

auto ci_charactor::is_pet() const -> bool {
  if (ext_client::rtti::is_kind_of(this, "CICPet2") ||
      ext_client::rtti::is_kind_of(this, "CICCos") ||
      ext_client::rtti::is_kind_of(this, "CICRide")) {
    return true;
  }
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }
  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto type3 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 2);
    return type3 == 3;
  }
  return false;
}

auto ci_charactor::is_fellow_pet() const -> bool {
  if (!is_pet()) {
    return false;
  }
  if (ext_client::rtti::is_kind_of(this, "CICPet2")) {
    return true;
  }
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }
  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto type3 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 2);
    const auto type4 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 3);
    return type3 == 3 && type4 == 9;
  }
  return false;
}

auto ci_charactor::is_growth_pet() const -> bool {
  if (!is_pet()) {
    return false;
  }
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }
  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto type3 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 2);
    const auto type4 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 3);
    return type3 == 3 && type4 == 3;
  }
  return false;
}

auto ci_charactor::is_ability_pet() const -> bool {
  if (!is_pet()) {
    return false;
  }
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }
  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto type3 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 2);
    const auto type4 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 3);
    return type3 == 3 && type4 == 4;
  }
  return false;
}

auto ci_charactor::is_ride_pet() const -> bool {
  if (!is_pet()) {
    return false;
  }
  if (ext_client::rtti::is_kind_of(this, "CICRide")) {
    return true;
  }
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }
  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto type3 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 2);
    const auto type4 = *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<std::uintptr_t>(ref) + 3);
    return type3 == 3 && (type4 == 1 || type4 == 2);
  }
  return false;
}

auto ci_charactor::get_owner_unique_id() const -> std::uint32_t {
  if (!ext_client::utils::memory::is_valid_ptr(this) || !is_pet()) {
    return 0;
  }
  return ext_client::off::field_at<std::uint32_t>(this, 0x8D0);
}

auto ci_charactor::get_pet_archetype() const -> std::uint8_t {
  if (!is_fellow_pet()) {
    return 0;
  }
  const void* ref = ext_client::off::field_at<const void*>(this, 0x76C);
  if (!ext_client::utils::memory::is_valid_ptr(ref)) {
    ref = ext_client::off::field_at<const void*>(this, 0x2D8);
  }
  if (ext_client::utils::memory::is_valid_ptr(ref)) {
    const auto* codename = &ext_client::off::field_at<ext_client::msvc9::wstring>(ref, 0x008);
    if (codename && codename->c_str()) {
      const std::wstring_view sv(codename->c_str(), codename->length());
      if (sv.find(L"_DEF") != std::wstring_view::npos || sv.find(L"DEF") != std::wstring_view::npos) return 4; // Defensive
      if (sv.find(L"_ATT") != std::wstring_view::npos || sv.find(L"_ENC") != std::wstring_view::npos) return 5; // Attack
      if (sv.find(L"_BUF") != std::wstring_view::npos || sv.find(L"_ASS") != std::wstring_view::npos) return 3; // Buff
    }
  }
  // Default fellow pet archetype from tooltip if known
  return 4; // Defensive by default for Fellow pet species
}

auto ci_charactor::pet_archetype_name() const -> const char* {
  switch (get_pet_archetype()) {
    case 3: return "Buff";
    case 4: return "Defensive";
    case 5: return "Attack";
    default: return is_fellow_pet() ? "Fellow" : "";
  }
}

auto ci_charactor::pet_archetype_name_loc() const -> std::string {
  using ext_client::sdk::ui::get_string_utf8;
  switch (get_pet_archetype()) {
    case 3: return get_string_utf8(L"UIIT_STT_PET2_PETTYPE_ASS");
    case 4: return get_string_utf8(L"UIIT_STT_PET2_PETTYPE_PRO");
    case 5: return get_string_utf8(L"UIIT_STT_PET2_PETTYPE_ENC");
    default: return get_string_utf8(L"UIIT_STT_SILKMALL_PET");
  }
}

auto ci_charactor::is_teleport() const -> bool {
  return ext_client::rtti::is_kind_of(this, "CITeleportGate");
}

auto ci_charactor::is_monster() const -> bool {
  if (is_player() || is_npc() || is_pet() || is_teleport()) {
    return false;
  }
  if (ext_client::rtti::is_kind_of(this, "CICMonster")) {
    return true;
  }
  if (get_rarity() > 0 || is_party_mob()) {
    return true;
  }
  return false;
}

auto ci_charactor::get_rarity() const -> std::uint8_t {

  return ext_client::off::field_at<std::uint8_t>(this, 0x8DC);
}

auto ci_charactor::is_party_mob() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x8DD) == 1;
}

auto ci_charactor::is_unique() const -> bool {
  const auto r = get_rarity();
  return r == 3 || r == 8;
}

auto ci_charactor::is_champion() const -> bool {
  return get_rarity() == 1;
}

auto ci_charactor::is_giant() const -> bool {
  return get_rarity() == 4;
}

auto ci_charactor::rarity_name() const -> const char* {
  switch (get_rarity()) {
    case 0: return is_party_mob() ? "Party" : "Normal";
    case 1: return is_party_mob() ? "Party Champion" : "Champion";
    case 3:
    case 8: return "Unique";
    case 4: return is_party_mob() ? "Party Giant" : "Giant";
    case 5: return "Titan";
    case 6: return "Elite";
    case 7:
    case 9: return "Strong";
    default: return "";
  }
}

auto ci_charactor::rarity_name_loc() const -> std::string {
  using ext_client::sdk::ui::get_string_utf8;
  const auto party_prefix = is_party_mob() ? std::string("Party ") : std::string{};
  switch (get_rarity()) {
    case 0: return is_party_mob() ? std::string("Party") : get_string_utf8(L"UIIT_STT_MOB_NORMAL");
    case 1: return party_prefix + get_string_utf8(L"UIIT_STT_MOB_CHAMPION");
    case 3:
    case 8: return get_string_utf8(L"UIIT_STT_MOB_UNIQUE");
    case 4: return party_prefix + get_string_utf8(L"UIIT_STT_MOB_GIANT");
    case 5: return get_string_utf8(L"UIIT_STT_MOB_TITAN");
    case 6: return get_string_utf8(L"UIIT_STT_MOB_ELITE");
    case 7:
    case 9: return get_string_utf8(L"UIIT_STT_MOB_STRONG");
    default: return "";
  }
}

// ===========================================================================
// 6. Overhead & Height Dimensions
// ===========================================================================
auto ci_charactor::get_model_height() const -> float {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return 20.0f;
  }
  // 1. Check CICharactor + 0x2E4 (height buffer copied from 0x2E0)
  const float h = ext_client::off::field_at<float>(this, 0x2E4);
  if (h > 0.1f && h < 500.0f) {
    return h;
  }
  // 2. Fallback to 0x2E0
  const float h0 = ext_client::off::field_at<float>(this, 0x2E0);
  if (h0 > 0.1f && h0 < 500.0f) {
    return h0;
  }
  // 3. Fallback: query compound_obj bounding box max_y
  auto* cobj = const_cast<ci_charactor*>(this)->get_compound_obj();
  if (cobj) {
    vector3f min_pt, max_pt;
    if (cobj->bounding_box(min_pt, max_pt)) {
      const float diff_y = max_pt.y - min_pt.y;
      if (diff_y > 0.1f && diff_y < 500.0f) {
        return diff_y;
      }
    }
  }
  return 20.0f; // Default human height
}

auto ci_charactor::get_overhead_3d_position() const -> vector3f {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return vector3f(0.0f, 0.0f, 0.0f);
  }

  // 1. Base world continuous coordinates at CIObject + 0x90
  const auto* pos_ptr = &ext_client::off::field_at<float>(this, 0x090);
  const float x = pos_ptr[0];
  const float y = pos_ptr[1];
  const float z = pos_ptr[2];
  if (x != 0.0f || y != 0.0f || z != 0.0f) {
    const float height = get_model_height();
    return vector3f(x, y + height + 2.0f, z);
  }

  // 2. Fallback: Regional position at CICharactor + 0x7C
  const auto* spos = get_position();
  if (spos) {
    const int rx = static_cast<int>(spos->region_x());
    const int ry = static_cast<int>(spos->region_y());
    if (rx > 0 && ry > 0) {
      const float x = (static_cast<float>(rx) - 128.0f) * 1920.0f + spos->x;
      const float y = spos->z + get_model_height() + 2.0f;
      const float z = (static_cast<float>(ry) - 128.0f) * 1920.0f + spos->y;
      return vector3f(x, y, z);
    }
  }

  return vector3f(0.0f, 0.0f, 0.0f);
}

auto ci_charactor::get_engine_projected_overhead(float& out_screen_x, float& out_screen_y) const -> bool {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return false;
  }
  // Native engine camera viewport projection at CICharactor + 0x5FC (X) and + 0x600 (Y)
  const float sx = ext_client::off::field_at<float>(this, 0x5FC);
  const float sy = ext_client::off::field_at<float>(this, 0x600);
  if (sx > -200.0f && sx < 4000.0f && sy > -200.0f && sy < 3000.0f) {
    out_screen_x = sx;
    out_screen_y = sy;
    return true;
  }
  return false;
}
