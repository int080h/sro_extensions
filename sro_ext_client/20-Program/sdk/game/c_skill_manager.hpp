#pragma once

#include "sdk/game/cref_skill.hpp"
#include "sdk/net/packet_injection.hpp"
#include "utils/memory.hpp"
#include "utils/offsets.hpp"

#include "utils/msvc9_stl.hpp"
#include "utils/string.hpp"
#include "sdk/ui/cui_string_manager.hpp"

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// CSkillManager — Game Client Skill System Interface
// ---------------------------------------------------------------------------
class c_skill_manager {
public:
  static constexpr std::uintptr_t k_global_ref_mgr_addr = 0x0117EE20;
  static constexpr std::uintptr_t k_find_ref_skill_fn   = 0x00A75BD0;
  static constexpr std::uintptr_t k_cast_skill_ui_fn    = 0x009127A0;
  static constexpr std::uintptr_t k_is_skill_cooldown_fn= 0x00B0C2B0;
  static constexpr std::uintptr_t k_get_cooltime_mgr_fn = 0x0085E7A0;
  static constexpr std::uintptr_t k_get_runtime_mgr_fn  = 0x0085E7C0;
  static constexpr std::uintptr_t k_cg_interface_ptr    = 0x013BAE3C;

  struct s_learned_mastery {
    std::uint32_t id{0};
    std::uint32_t raw_key{0};
    std::uint8_t  level{0};
    std::string   name;
    std::string   loc_key;
  };

  // 1. Static RefSkill Definition Lookup
  [[nodiscard]] static auto get_ref_skill(std::uint32_t skill_id) -> const cref_skill* {
    if (skill_id == 0) return nullptr;
    auto* mgr = reinterpret_cast<void*>(k_global_ref_mgr_addr);
    if (!ext_client::utils::memory::is_valid_ptr(mgr)) return nullptr;

    using find_fn = const cref_skill*(__thiscall*)(void*, std::uint32_t);
    const auto fn = ext_client::off::as_fn<find_fn>(k_find_ref_skill_fn);
    if (!fn) return nullptr;

    auto* skill = fn(mgr, skill_id);
    return ext_client::utils::memory::is_valid_ptr(skill) ? skill : nullptr;
  }

  // 2A. Native CIFSkill window (*(popup + 0x7CC))
  [[nodiscard]] static auto get_skill_wnd() -> void* {
    auto** cg_if = reinterpret_cast<void***>(k_cg_interface_ptr);
    if (!cg_if || !ext_client::utils::memory::is_valid_ptr(cg_if) || !*cg_if) {
      return nullptr;
    }
    using from_interface_fn = void*(__thiscall*)(void*);
    const auto from_if = ext_client::off::as_fn<from_interface_fn>(0x008831F0);
    if (!from_if) return nullptr;
    void* popup = from_if(*cg_if);
    if (!popup || !ext_client::utils::memory::is_valid_ptr(popup)) {
      return nullptr;
    }
    void* skill_wnd = ext_client::off::field_at<void*>(popup, 0x7CC);
    return ext_client::utils::memory::is_valid_ptr(skill_wnd) ? skill_wnd : nullptr;
  }

  // 2B. Runtime Learned Skill Manager (*(CGInterface + 0x79C) via sub_85E7C0)
  [[nodiscard]] static auto get_runtime_mgr() -> void* {
    auto** cg_if = reinterpret_cast<void***>(k_cg_interface_ptr);
    if (!cg_if || !ext_client::utils::memory::is_valid_ptr(cg_if) || !*cg_if) {
      return nullptr;
    }
    using get_mgr_fn = void*(__thiscall*)(void*);
    const auto get_mgr = ext_client::off::as_fn<get_mgr_fn>(k_get_runtime_mgr_fn);
    if (!get_mgr) return nullptr;
    void* runtime_mgr = get_mgr(*cg_if);
    return ext_client::utils::memory::is_valid_ptr(runtime_mgr) ? runtime_mgr : nullptr;
  }

  // 2C. Query if skill is learned by the local player (std::map at skill_wnd + 0x380)
  [[nodiscard]] static auto is_skill_learned(std::uint32_t skill_id) -> bool {
    if (skill_id == 0) return false;
    void* skill_wnd = get_skill_wnd();
    if (skill_wnd) {
      const auto learned_map =
          ext_client::msvc9::map_view<std::uint32_t, void*>::from_object(skill_wnd, 0x380);
      if (learned_map.contains(skill_id)) return true;
    }
    void* runtime_mgr = get_runtime_mgr();
    if (runtime_mgr) {
      const auto fallback_map =
          ext_client::msvc9::map_view<std::uint32_t, void*>::from_object(runtime_mgr, 0x0C);
      if (fallback_map.contains(skill_id)) return true;
    }
    return false;
  }

  // 2D. Get all learned skill IDs (std::map at skill_wnd + 0x380)
  [[nodiscard]] static auto get_learned_skill_ids() -> std::vector<std::uint32_t> {
    std::vector<std::uint32_t> skills;
    void* skill_wnd = get_skill_wnd();
    if (skill_wnd) {
      const auto learned_map =
          ext_client::msvc9::map_view<std::uint32_t, void*>::from_object(skill_wnd, 0x380);
      skills.reserve(learned_map.size());
      for (const auto& [id, _] : learned_map) {
        if (id > 0) {
          skills.push_back(id);
        }
      }
    }
    if (skills.empty()) {
      void* runtime_mgr = get_runtime_mgr();
      if (runtime_mgr) {
        const auto fallback_map =
            ext_client::msvc9::map_view<std::uint32_t, void*>::from_object(runtime_mgr, 0x0C);
        skills.reserve(fallback_map.size());
        for (const auto& [id, _] : fallback_map) {
          if (id > 0) {
            skills.push_back(id);
          }
        }
      }
    }
    return skills;
  }

  struct s_raw_skill_data {
    std::uint32_t skill_id{0};
    std::uint32_t series_id{0};
    std::uint8_t  level{1};
    std::uint8_t  is_active{1};
    std::uint32_t mastery_id{0};
    std::uint8_t  req_mastery_level{0};
    bool          is_job_skill{false};
    std::int32_t  branch_index{0};
    std::int32_t  slot_index{0};
    std::uint32_t req_str{0};
    std::uint32_t req_int{0};
    std::uint32_t req_sp{0};
    std::uint32_t req_weapon_1{0};
    std::uint32_t req_weapon_2{0};
    std::uint32_t hp_cost{0};
    std::uint32_t mp_cost{0};
    std::uint16_t hp_ratio_cost{0};
    std::uint16_t mp_ratio_cost{0};
    float         cast_time_sec{0.0f};
    float         cooldown_sec{0.0f};
    float         duration_sec{0.0f};
    float         range_meters{0.0f};
    std::int32_t  phy_atk_min{0};
    std::int32_t  phy_atk_max{0};
    std::int32_t  mag_atk_min{0};
    std::int32_t  mag_atk_max{0};
    float         atk_ratio{0.0f};
    std::uint32_t cooldown_ms{0};
    char          icon_buf[260]{};
    wchar_t       name_key[128]{};
    wchar_t       desc_key[256]{};
    wchar_t       localized_name[128]{};
    wchar_t       localized_desc[512]{};
    char          effects_buf[1024]{};
    bool          has_next_level{false};
    bool          is_max_level{false};
    std::uint8_t  next_req_mastery_level{0};
    std::uint32_t next_mastery_id{0};
    std::uint32_t next_req_str{0};
    std::uint32_t next_req_int{0};
    std::uint32_t next_req_sp{0};
    struct s_raw_prereq {
      char          name[128]{};
      std::uint8_t  req_level{0};
      std::uint32_t skill_id{0};
    };
    std::vector<s_raw_prereq> next_prereqs;
  };

  static auto safe_get_skill_data(void* ref_mgr, std::uint32_t skill_id) -> void* {
    using get_skill_data_fn = void*(__thiscall*)(void*, std::uint32_t);
    const auto get_skill_data = ext_client::off::as_fn<get_skill_data_fn>(0x00A93BD0);
    if (!get_skill_data || !ref_mgr) return nullptr;
    __try {
      return get_skill_data(ref_mgr, skill_id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      return nullptr;
    }
  }

  static auto safe_get_skill_by_level(void* ref_mgr, std::uint32_t series_id, std::uint8_t level) -> void* {
    using get_skill_by_level_fn = void*(__thiscall*)(void*, std::uint32_t, std::uint8_t);
    const auto get_skill_by_level = ext_client::off::as_fn<get_skill_by_level_fn>(0x00A93FB0);
    if (!get_skill_by_level || !ref_mgr) return nullptr;
    __try {
      return get_skill_by_level(ref_mgr, series_id, level);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      return nullptr;
    }
  }

  static auto raw_format_effects_seh(void* sdata, void* fn_init_ptr, void* fn_format_ptr, void* fn_free_ptr, wchar_t* out_wbuf, std::size_t max_count) -> bool {
    using init_fn = void*(__thiscall*)(void*);
    using format_fn = void(__thiscall*)(void*, int, void*, int);
    using free_fn = void(__thiscall*)(void*);
    const auto fn_init = reinterpret_cast<init_fn>(fn_init_ptr);
    const auto fn_format = reinterpret_cast<format_fn>(fn_format_ptr);
    const auto fn_free = reinterpret_cast<free_fn>(fn_free_ptr);
    bool ok = false;
    __try {
      alignas(16) char tbuf[160] = {0};
      fn_init(tbuf);
      fn_format(sdata, 2, tbuf, 0); // flags = 2 (all combat & debuff effects)
      const auto ef_ref = ext_client::msvc9::wstring_ref::from(tbuf + 0x54);
      if (!ef_ref.empty() && ef_ref.data()) {
        ef_ref.copy_to(out_wbuf, max_count);
        ok = true;
      }
      fn_free(tbuf);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      ok = false;
    }
    return ok;
  }

  static auto safe_format_effects(void* sdata, char* out_buf, std::size_t max_bytes) -> bool {
    if (!sdata || !out_buf || max_bytes == 0) return false;
    out_buf[0] = '\0';
    void* fn_init = reinterpret_cast<void*>(0x00759AD0);
    void* fn_format = reinterpret_cast<void*>(0x00ABAFF0);
    void* fn_free = reinterpret_cast<void*>(0x00757700);

    wchar_t temp_wbuf[1024] = {0};
    if (raw_format_effects_seh(sdata, fn_init, fn_format, fn_free, temp_wbuf, sizeof(temp_wbuf) / sizeof(wchar_t))) {
      if (temp_wbuf[0] != L'\0') {
        const std::string u8 = ext_client::utils::string::to_utf8(temp_wbuf);
        std::strncpy(out_buf, u8.c_str(), max_bytes - 1);
        out_buf[max_bytes - 1] = '\0';
        return true;
      }
    }
    return false;
  }

  static auto safe_get_localized_text(void* text_mgr, void* key_ptr, wchar_t* out_buf, std::size_t max_count) -> bool {
    if (!text_mgr || !key_ptr || !out_buf || max_count == 0) return false;
    out_buf[0] = L'\0';
    using get_text_from_wstr_fn = void*(__thiscall*)(void*, void*);
    const auto get_text = ext_client::off::as_fn<get_text_from_wstr_fn>(0x009E4E80);
    if (!get_text) return false;
    __try {
      void* res_ptr = get_text(text_mgr, key_ptr);
      if (res_ptr && ext_client::utils::memory::is_valid_ptr(res_ptr)) {
        const auto ref = ext_client::msvc9::wstring_ref::from(res_ptr);
        if (!ref.empty()) {
          return ref.copy_to(out_buf, max_count);
        }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return false;
  }

  static auto query_runtime_skill_data(void* ref_mgr, std::uint32_t skill_id, s_raw_skill_data* out) -> bool {
    if (!out || !ref_mgr) return false;
    void* sdata = safe_get_skill_data(ref_mgr, skill_id);
    if (!sdata || !ext_client::utils::memory::is_valid_ptr(sdata)) return false;

    out->skill_id = skill_id;
    out->series_id = ext_client::off::field_at<std::uint32_t>(sdata, 0x7C0);
    out->level = ext_client::off::field_at<std::uint8_t>(sdata, 0x81C);
    out->is_active = ext_client::off::field_at<std::uint8_t>(sdata, 0x81D);
    out->mastery_id = ext_client::off::field_at<std::uint32_t>(sdata, 0x860);
    out->req_mastery_level = ext_client::off::field_at<std::uint8_t>(sdata, 0x868);
    out->is_job_skill = (ext_client::off::field_at<std::uint32_t>(sdata, 0x148) != 0);

    // Silkroad Native Layout (sub_77C820):
    // [sdata + 0x7BC + 0xF8] = [sdata + 0x8B4] is vertical row/branch index (0, 1, 2, ...)
    // [sdata + 0x7BC + 0xFC] = [sdata + 0x8B8] is horizontal column/slot index (0, 1, 2, ...)
    const auto raw_8b4 = ext_client::off::field_at<std::int32_t>(sdata, 0x8B4);
    const auto raw_8b8 = ext_client::off::field_at<std::int32_t>(sdata, 0x8B8);
    const auto raw_8b0 = ext_client::off::field_at<std::int32_t>(sdata, 0x8B0);
    const auto raw_8ac = ext_client::off::field_at<std::int32_t>(sdata, 0x8AC);

    out->branch_index = (raw_8b4 >= 0) ? raw_8b4 : ((raw_8b0 >= 0) ? raw_8b0 : 0);
    out->slot_index   = (raw_8b8 >= 0) ? raw_8b8 : ((raw_8ac >= 0) ? raw_8ac : 0);

    out->req_str = ext_client::off::field_at<std::uint32_t>(sdata, 0x86C);
    out->req_int = ext_client::off::field_at<std::uint32_t>(sdata, 0x870);
    out->req_sp = ext_client::off::field_at<std::uint32_t>(sdata, 0x884);
    out->req_weapon_1 = ext_client::off::field_at<std::uint32_t>(sdata, 0x894);
    out->req_weapon_2 = ext_client::off::field_at<std::uint32_t>(sdata, 0x898);
    out->hp_cost = ext_client::off::field_at<std::uint32_t>(sdata, 0x89C);
    out->mp_cost = ext_client::off::field_at<std::uint32_t>(sdata, 0x8A0);
    out->hp_ratio_cost = ext_client::off::field_at<std::uint16_t>(sdata, 0x8A4);
    out->mp_ratio_cost = ext_client::off::field_at<std::uint16_t>(sdata, 0x8A6);

    out->cast_time_sec = ext_client::off::field_at<float>(sdata, 0x234);
    if (out->cast_time_sec > 50.0f) {
      out->cast_time_sec /= 1000.0f;
    }
    out->cooldown_sec = ext_client::off::field_at<float>(sdata, 0x238);
    out->duration_sec = ext_client::off::field_at<float>(sdata, 0x23C);
    out->range_meters = ext_client::off::field_at<float>(sdata, 0x240);
    out->phy_atk_min = ext_client::off::field_at<std::int32_t>(sdata, 0x268);
    out->phy_atk_max = ext_client::off::field_at<std::int32_t>(sdata, 0x26C);
    out->mag_atk_min = ext_client::off::field_at<std::int32_t>(sdata, 0x270);
    out->mag_atk_max = ext_client::off::field_at<std::int32_t>(sdata, 0x274);
    out->atk_ratio = ext_client::off::field_at<float>(sdata, 0x278);
    out->cooldown_ms = ext_client::off::field_at<std::uint32_t>(sdata, 0x550);

    // Combat effects pointer at +0x04 (sub_ABAFF0 physical / magical attack power & ratio)
    void* v10_ptr = ext_client::off::field_at<void*>(sdata, 0x04);
    if (v10_ptr && ext_client::utils::memory::is_valid_ptr(v10_ptr)) {
      const auto* v10 = static_cast<const std::uint32_t*>(v10_ptr);
      const std::uint32_t flags = v10[0];
      const std::uint32_t ratio = v10[1];
      const std::int32_t min_val = static_cast<std::int32_t>(v10[2]);
      const std::int32_t max_val = static_cast<std::int32_t>(v10[3]);
      if (flags & 4) { // Physical
        out->phy_atk_min = min_val;
        out->phy_atk_max = max_val;
        if (ratio > 0) out->atk_ratio = static_cast<float>(ratio) / 100.0f;
      } else if (flags & 8) { // Magical
        out->mag_atk_min = min_val;
        out->mag_atk_max = max_val;
        if (ratio > 0) out->atk_ratio = static_cast<float>(ratio) / 100.0f;
      }
    }

    // Icon path at +0x8C4
    const auto* icon_str = reinterpret_cast<const ext_client::msvc9::string*>(
        reinterpret_cast<std::uintptr_t>(sdata) + 0x8C4);
    if (icon_str && icon_str->c_str() && *icon_str->c_str() != '\0') {
      const char* src = icon_str->c_str();
      std::size_t i = 0;
      for (; i + 1 < sizeof(out->icon_buf) && src[i] != '\0'; ++i) {
        out->icon_buf[i] = src[i];
      }
      out->icon_buf[i] = '\0';
    }

    // Name key (msvc9::wstring) at +0x8E0
    const auto* name_obj = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(sdata) + 0x8E0);
    const auto name_ref = ext_client::msvc9::wstring_ref::from(name_obj);
    if (!name_ref.empty()) {
      name_ref.copy_to(out->name_key, sizeof(out->name_key) / sizeof(wchar_t));
    }

    // Desc key (msvc9::wstring) at +0x918
    const auto* desc_obj = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(sdata) + 0x918);
    const auto desc_ref = ext_client::msvc9::wstring_ref::from(desc_obj);
    if (!desc_ref.empty()) {
      desc_ref.copy_to(out->desc_key, sizeof(out->desc_key) / sizeof(wchar_t));
    }

    // Resolve localized text directly through game client's sub_9E4E80 (CTextStringManager)
    void* text_mgr = reinterpret_cast<void*>(0x0117EDA8);
    if (ext_client::utils::memory::is_valid_ptr(text_mgr)) {
      safe_get_localized_text(text_mgr, reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(sdata) + 0x8E0),
                              out->localized_name, sizeof(out->localized_name) / sizeof(wchar_t));
      safe_get_localized_text(text_mgr, reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(sdata) + 0x918),
                              out->localized_desc, sizeof(out->localized_desc) / sizeof(wchar_t));
    }

    // 1. Query native skill effects formatted by engine (sub_ABAFF0, flags = 2)
    safe_format_effects(sdata, out->effects_buf, sizeof(out->effects_buf));

    // 2. Query next level requirements & prerequisite skills (sub_A93FB0)
    if (out->series_id > 0) {
      void* next_sdata = safe_get_skill_by_level(ref_mgr, out->series_id, static_cast<std::uint8_t>(out->level + 1));
      if (next_sdata && ext_client::utils::memory::is_valid_ptr(next_sdata)) {
        out->has_next_level = true;
        out->next_req_mastery_level = ext_client::off::field_at<std::uint8_t>(next_sdata, 0x868);
        out->next_mastery_id = ext_client::off::field_at<std::uint32_t>(next_sdata, 0x860);
        out->next_req_str = ext_client::off::field_at<std::uint32_t>(next_sdata, 0x86C);
        out->next_req_int = ext_client::off::field_at<std::uint32_t>(next_sdata, 0x870);
        out->next_req_sp  = ext_client::off::field_at<std::uint32_t>(next_sdata, 0x884);

        // Prerequisite skills list at next_sdata + 0xB04
        void* list_head_ptr = ext_client::off::field_at<void*>(next_sdata, 0xB04);
        if (list_head_ptr && ext_client::utils::memory::is_valid_ptr(list_head_ptr)) {
          void* cur_node = *reinterpret_cast<void**>(list_head_ptr);
          int count = 0;
          while (cur_node && cur_node != list_head_ptr && count < 6 &&
                 ext_client::utils::memory::is_valid_ptr(cur_node)) {
            void* prereq_sdata = *reinterpret_cast<void**>(reinterpret_cast<std::uintptr_t>(cur_node) + 8);
            if (prereq_sdata && ext_client::utils::memory::is_valid_ptr(prereq_sdata)) {
              s_raw_skill_data::s_raw_prereq pr{};
              pr.req_level = ext_client::off::field_at<std::uint8_t>(prereq_sdata, 0x81C);
              pr.skill_id = ext_client::off::field_at<std::uint32_t>(prereq_sdata, 0x00);

              wchar_t pr_wname[128] = {0};
              if (text_mgr && safe_get_localized_text(text_mgr,
                                                      reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(prereq_sdata) + 0x8E0),
                                                      pr_wname, sizeof(pr_wname) / sizeof(wchar_t))) {
                const auto u8 = ext_client::utils::string::to_utf8(pr_wname);
                std::strncpy(pr.name, u8.c_str(), sizeof(pr.name) - 1);
              }
              if (pr.name[0] == '\0') {
                std::snprintf(pr.name, sizeof(pr.name), "Skill");
              }
              out->next_prereqs.push_back(pr);
            }
            cur_node = *reinterpret_cast<void**>(cur_node);
            count++;
          }
        }
      } else {
        out->is_max_level = true;
      }
    }

    return true;
  }

  inline static auto get_weapon_name(std::uint32_t type) -> std::string {
    switch (type) {
      case 2:  return ext_client::sdk::ui::get_string_utf8(L"PARAM_WEAPON_SWORD", "Sword");
      case 3:  return ext_client::sdk::ui::get_string_utf8(L"PARAM_WEAPON_BLADE", "Blade");
      case 4:  return ext_client::sdk::ui::get_string_utf8(L"PARAM_WEAPON_SPEAR", "Spear");
      case 5:  return ext_client::sdk::ui::get_string_utf8(L"PARAM_WEAPON_TBLADE", "Glavie");
      case 6:  return ext_client::sdk::ui::get_string_utf8(L"PARAM_WEAPON_BOW", "Bow");
      case 7:  return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_ONEHANDSWORD", "1H Sword");
      case 8:  return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_TWOHANDSWORD", "2H Sword");
      case 9:  return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_DUELAXE", "Dual Axe");
      case 10: return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_DARKSTAFF", "Warlock Staff");
      case 11: return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_TWOHANDSTAFF", "Two-handed Staff");
      case 12: return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_CROSSBOW", "Crossbow");
      case 13: return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_DAGGER", "Dagger");
      case 14: return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_HARP", "Harp");
      case 15: return ext_client::sdk::ui::get_string_utf8(L"UIO_NEWCHAR_STT_EU_ONEHANDSTAFF", "One-handed Staff");
      case 16: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_SHIELD", "Shield");
      default: return "";
    }
  }

  inline static auto get_mastery_name_by_id(std::uint32_t mastery_id) -> std::string {
    switch (mastery_id) {
      // European Masteries (0x201 - 0x206)
      case 513: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_WARRIOR", "Warrior");
      case 514: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_WIZARD", "Wizard");
      case 515: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_ROG", "Rogue");
      case 516: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_WARLOCK", "Warlock");
      case 517: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_BARD", "Bard");
      case 518: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_CLERIC", "Cleric");

      // Chinese Weapon Masteries
      case 1:
      case 257: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_MASTERY_VI", "Bicheon");
      case 2:
      case 258: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_MASTERY_HEUK", "Heuksal");
      case 3:
      case 259: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_MASTERY_PA", "Pacheon");

      // Chinese Force Masteries
      case 4:
      case 260: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_MASTERY_HAN", "Cold");
      case 5:
      case 261: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_MASTERY_PUNG", "Lightning");
      case 6:
      case 262: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_MASTERY_HWA", "Fire");
      case 7:
      case 276: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_MASTERY_GI", "Force");

      // Job Masteries
      case 1000:
      case 277:
      case 264:
      case 255:
      case 101:
      case 102:
      case 103:
      case 115: return ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_CONFLICT_JOB_SKILL", "Job Skills");

      default:  break;
    }
    return "Mastery";
  }

  static auto get_mastery_row_count(std::uint32_t mastery_id) -> int {
    if (mastery_id == 0) return 0;
    __try {
      using get_row_config_fn = void*(__thiscall*)(void*, std::uint32_t);
      const auto get_row_config = ext_client::off::as_fn<get_row_config_fn>(0x00A73FE0);
      void* mgr = reinterpret_cast<void*>(0x0117EE20);
      if (get_row_config && mgr) {
        std::uint32_t keys[3];
        int num_keys = 0;
        if (mastery_id > 0xFFFF) {
          keys[num_keys++] = mastery_id;
          const std::uint32_t base = mastery_id >> 16;
          keys[num_keys++] = (base << 16) | 0xFFFF;
          keys[num_keys++] = (base << 16);
        } else {
          keys[num_keys++] = (mastery_id << 16) | 0xFFFF;
          keys[num_keys++] = (mastery_id << 16);
          keys[num_keys++] = mastery_id;
        }

        for (int k = 0; k < num_keys; ++k) {
          void* v4 = get_row_config(mgr, keys[k]);
          if (v4 && ext_client::utils::memory::is_valid_ptr(v4)) {
            // In Silkroad sub_77D700: v5 = *(_DWORD *)(sub_AA6D30(v4) + 36); where sub_AA6D30(v4) is v4 + 4.
            // So v4 + 4 + 36 = v4 + 40 is the exact row count!
            const auto rows = *reinterpret_cast<const std::int32_t*>(reinterpret_cast<std::uintptr_t>(v4) + 40);
            if (rows > 0 && rows < 60) {
              return rows;
            }
          }
        }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
  }

  static auto get_mastery_row_emblem(std::uint32_t mastery_id, int row_idx, std::string* out_icon) -> bool {
    if (mastery_id == 0 || row_idx < 0) return false;
    __try {
      using get_row_fn = void*(__thiscall*)(void*, std::uint32_t, unsigned int);
      const auto get_row = ext_client::off::as_fn<get_row_fn>(0x00A73F60);
      void* mgr = reinterpret_cast<void*>(0x0117EE20);
      if (get_row && mgr) {
        std::uint32_t keys[3];
        int num_keys = 0;
        if (mastery_id > 0xFFFF) {
          keys[num_keys++] = mastery_id;
          const std::uint32_t base = mastery_id >> 16;
          keys[num_keys++] = (base << 16) | 0xFFFF;
          keys[num_keys++] = (base << 16);
        } else {
          keys[num_keys++] = (mastery_id << 16) | 0xFFFF;
          keys[num_keys++] = (mastery_id << 16);
          keys[num_keys++] = mastery_id;
        }

        for (int k = 0; k < num_keys; ++k) {
          void* v4 = get_row(mgr, keys[k], static_cast<unsigned int>(row_idx));
          if (v4 && ext_client::utils::memory::is_valid_ptr(v4)) {
            // sub_ABB9C0(v4) is v4 + 4.
            // In sub_77EED0: sub_40AC10(v5 + 36, 0, -1) copies msvc9::string at (v4 + 4) + 36 = v4 + 40.
            const auto* icon_str = reinterpret_cast<const ext_client::msvc9::string*>(
                reinterpret_cast<std::uintptr_t>(v4) + 40);
            if (icon_str && icon_str->c_str() && *icon_str->c_str() != '\0') {
              if (out_icon) *out_icon = icon_str->c_str();
              return true;
            }
          }
        }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return false;
  }

  struct s_skill_details {
    std::uint32_t id{0};
    std::uint32_t series_id{0};
    std::string   code_name;
    std::string   name;
    std::string   description;
    std::string   icon_path;
    std::uint8_t  level{1};
    bool          is_active{true};
    std::uint32_t mastery_id{0};
    std::string   mastery_name;
    std::uint8_t  req_mastery_level{0};
    bool          is_job_skill{false};
    std::int32_t  branch_index{0};
    std::int32_t  slot_index{0};
    std::uint32_t req_str{0};
    std::uint32_t req_int{0};
    std::uint32_t req_sp{0};
    std::string   req_weapon;
    std::uint32_t req_weapon_1{0};
    std::uint32_t req_weapon_2{0};
    std::uint32_t skill_type{1};
    float         cast_time_sec{0.0f};
    float         cooldown_sec{0.0f};
    float         duration_sec{0.0f};
    float         range_meters{0.0f};
    std::uint32_t mp_cost{0};
    std::uint32_t hp_cost{0};
    std::uint16_t hp_ratio_cost{0};
    std::uint16_t mp_ratio_cost{0};
    std::int32_t  phy_atk_min{0};
    std::int32_t  phy_atk_max{0};
    std::int32_t  mag_atk_min{0};
    std::int32_t  mag_atk_max{0};
    float         atk_ratio{0.0f};
    std::uint32_t cooldown_ms{0};
    bool          is_learned{false};
    std::string   effects_text;
    bool          has_next_level{false};
    bool          is_max_level{false};
    std::uint8_t  next_req_mastery_level{0};
    std::string   next_mastery_name;
    std::uint32_t next_req_str{0};
    std::uint32_t next_req_int{0};
    std::uint32_t next_req_sp{0};
    struct s_prereq_info {
      std::string   name;
      std::uint8_t  req_level{0};
      std::uint32_t skill_id{0};
    };
    std::vector<s_prereq_info> next_prereqs;
  };

  [[nodiscard]] static auto get_skill_details(std::uint32_t skill_id) -> s_skill_details {
    s_skill_details d{};
    d.id = skill_id;

    auto* ref_mgr = reinterpret_cast<void*>(k_global_ref_mgr_addr);
    if (ext_client::utils::memory::is_valid_ptr(ref_mgr)) {
      s_raw_skill_data raw{};
      if (query_runtime_skill_data(ref_mgr, skill_id, &raw)) {
        d.series_id = raw.series_id;
        d.level = raw.level > 0 ? raw.level : 1;
        d.is_active = (raw.is_active != 0);
        d.skill_type = raw.is_active ? 1 : 0;
        d.mastery_id = raw.mastery_id;
        d.req_mastery_level = raw.req_mastery_level;
        d.is_job_skill = raw.is_job_skill;
        d.branch_index = raw.branch_index;
        d.slot_index = raw.slot_index;
        d.req_str = raw.req_str;
        d.req_int = raw.req_int;
        d.req_sp = raw.req_sp;
        d.req_weapon_1 = raw.req_weapon_1;
        d.req_weapon_2 = raw.req_weapon_2;
        d.hp_cost = raw.hp_cost;
        d.mp_cost = raw.mp_cost;
        d.hp_ratio_cost = raw.hp_ratio_cost;
        d.mp_ratio_cost = raw.mp_ratio_cost;
        d.cast_time_sec = raw.cast_time_sec;
        d.cooldown_sec = raw.cooldown_sec;
        d.duration_sec = raw.duration_sec;
        d.range_meters = raw.range_meters;
        d.phy_atk_min = raw.phy_atk_min;
        d.phy_atk_max = raw.phy_atk_max;
        d.mag_atk_min = raw.mag_atk_min;
        d.mag_atk_max = raw.mag_atk_max;
        d.atk_ratio = raw.atk_ratio;
        d.cooldown_ms = (raw.cooldown_ms > 0) ? raw.cooldown_ms : static_cast<std::uint32_t>(raw.cooldown_sec * 1000.0f);
        d.icon_path = raw.icon_buf;

        // Resolve localized Name
        if (raw.localized_name[0] != L'\0') {
          d.name = ext_client::utils::string::to_utf8(raw.localized_name);
        }
        if (d.name.empty() && raw.name_key[0] != L'\0') {
          d.code_name = ext_client::utils::string::to_utf8(raw.name_key);
          const auto loc = ext_client::sdk::ui::get_string(raw.name_key);
          if (!loc.empty()) {
            d.name = ext_client::utils::string::to_utf8(loc.c_str());
          } else {
            d.name = d.code_name;
          }
        } else if (raw.name_key[0] != L'\0') {
          d.code_name = ext_client::utils::string::to_utf8(raw.name_key);
        }

        // Resolve localized Description
        if (raw.localized_desc[0] != L'\0') {
          d.description = ext_client::utils::string::to_utf8(raw.localized_desc);
        }
        if (d.description.empty() && raw.desc_key[0] != L'\0') {
          const auto dloc = ext_client::sdk::ui::get_string(raw.desc_key);
          if (!dloc.empty()) {
            d.description = ext_client::utils::string::to_utf8(dloc.c_str());
          }
        }

        // Weapon Requirements text
        const std::string w1 = get_weapon_name(raw.req_weapon_1);
        const std::string w2 = get_weapon_name(raw.req_weapon_2);
        if (!w1.empty() && !w2.empty()) {
          d.req_weapon = w1 + ", " + w2;
        } else if (!w1.empty()) {
          d.req_weapon = w1;
        } else if (!w2.empty()) {
          d.req_weapon = w2;
        }

        // Mastery name
        if (d.mastery_id > 0) {
          d.mastery_name = get_mastery_name_by_id(d.mastery_id);
        }

        // Engine-formatted Effects & Next level conditions
        d.effects_text = raw.effects_buf;
        d.has_next_level = raw.has_next_level;
        d.is_max_level = raw.is_max_level;
        d.next_req_mastery_level = raw.next_req_mastery_level;
        d.next_mastery_name = (raw.next_mastery_id > 0) ? get_mastery_name_by_id(raw.next_mastery_id) : d.mastery_name;
        d.next_req_str = raw.next_req_str;
        d.next_req_int = raw.next_req_int;
        d.next_req_sp  = raw.next_req_sp;
        for (const auto& pr : raw.next_prereqs) {
          s_skill_details::s_prereq_info pinfo;
          pinfo.name = pr.name;
          pinfo.req_level = pr.req_level;
          pinfo.skill_id = pr.skill_id;
          d.next_prereqs.push_back(pinfo);
        }
      }
    }

    if (d.name.empty()) {
      d.name = "Skill #" + std::to_string(skill_id);
    }
    d.is_learned = is_skill_learned(skill_id);
    return d;
  }

  // 2E. Get all learned skill masteries (std::map at skill_wnd + 0x374)
  [[nodiscard]] static auto get_learned_masteries() -> std::vector<s_learned_mastery> {
    std::vector<s_learned_mastery> result;
    void* skill_wnd = get_skill_wnd();
    if (!skill_wnd) return result;

    const auto mastery_map =
        ext_client::msvc9::map_view<std::uint32_t, void*>::from_object(skill_wnd, 0x374);

    for (const auto& [raw_key, struct_ptr] : mastery_map) {
      if (raw_key == 0 || !struct_ptr || !ext_client::utils::memory::is_valid_ptr(struct_ptr)) {
        continue;
      }
      // In Silkroad, map key is (mastery_id << 16) | branch_index, or 65601535 for Job, or raw mastery_id
      const std::uint32_t base_id = (raw_key > 0xFFFF) ? (raw_key >> 16) : raw_key;
      const std::uint8_t lvl = ext_client::off::field_at<std::uint8_t>(struct_ptr, 0x04);

      // Check if we already have an entry for base_id
      auto it = std::find_if(result.begin(), result.end(), [base_id](const s_learned_mastery& item) {
        return item.id == base_id;
      });

      if (it != result.end()) {
        if (lvl > it->level) {
          it->level = lvl;
        }
        continue;
      }

      s_learned_mastery m;
      m.id = base_id;
      m.raw_key = raw_key;
      m.level = lvl;

      // struct_ptr + 8 is a direct pointer to CMasteryData
      void* mastery_data = ext_client::off::field_at<void*>(struct_ptr, 0x08);
      if (mastery_data && ext_client::utils::memory::is_valid_ptr(mastery_data)) {
        // mastery_data + 0x0C is std::wstring name_key (e.g. L"UIIT_STT_WIZARD")
        const auto* wstr_obj = reinterpret_cast<const void*>(
            reinterpret_cast<std::uintptr_t>(mastery_data) + 0x0C);
        const auto ref_key = ext_client::msvc9::wstring_ref::from(wstr_obj);
        if (!ref_key.empty() && ref_key.data() && *ref_key.data() != L'\0') {
          m.loc_key = ext_client::utils::string::to_utf8(ref_key.data());
          const auto loc_str = ext_client::sdk::ui::get_string(ref_key.data());
          if (!loc_str.empty()) {
            m.name = ext_client::utils::string::to_utf8(loc_str.c_str());
          } else {
            const std::wstring sn_key = L"SN_" + std::wstring(ref_key.data());
            const auto sn_str = ext_client::sdk::ui::get_string(sn_key.c_str());
            if (!sn_str.empty()) {
              m.name = ext_client::utils::string::to_utf8(sn_str.c_str());
            }
          }
        }
      }

      // Explicit game-localization fallback via get_mastery_name_by_id
      if (m.name.empty() || m.name == "Mastery") {
        m.name = get_mastery_name_by_id(base_id);
      }

      if (m.name.empty() || m.name == "Mastery") {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Mastery %u", base_id);
        m.name = buf;
      }
      result.push_back(std::move(m));
    }
    return result;
  }

  [[nodiscard]] static auto get_player_mastery_level(std::uint32_t mastery_id) -> std::uint8_t {
    if (mastery_id == 0) return 0;
    const auto masteries = get_learned_masteries();
    for (const auto& m : masteries) {
      if (m.id == mastery_id) return m.level;
    }
    return 0;
  }

  [[nodiscard]] static auto call_native_total_mastery_fn(void* map_ptr) -> int {
    using get_total_fn = int(__fastcall*)(void*, int);
    const auto fn = ext_client::off::as_fn<get_total_fn>(0x00B10010);
    if (!fn) return 0;
    __try {
      return fn(map_ptr, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      return 0;
    }
  }

  // 2F. Native Total Mastery Level Calculation (via sub_B10010)
  [[nodiscard]] static auto get_total_mastery_level() -> std::uint32_t {
    void* skill_wnd = get_skill_wnd();
    if (skill_wnd) {
      void* map_ptr = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(skill_wnd) + 0x374);
      const int total = call_native_total_mastery_fn(map_ptr);
      if (total > 0) return static_cast<std::uint32_t>(total);
    }
    // Fallback: sum non-job masteries
    std::uint32_t sum = 0;
    for (const auto& m : get_learned_masteries()) {
      if (m.id == 1000 || m.id == 255 || m.id == 277 || m.id == 264 || m.id == 115) continue;
      sum += m.level;
    }
    return sum;
  }

  // 2G. Silkroad Native Mastery Cap (min(2 * player_level, dword_1151CFC) for EU, 300 for Chinese)
  [[nodiscard]] static auto get_max_mastery_cap(bool has_europe, std::uint8_t player_level) -> std::uint32_t {
    std::uint32_t cap = 300;
    const auto* cap_ptr = reinterpret_cast<const std::uint32_t*>(0x01151CFC);
    if (cap_ptr && ext_client::utils::memory::is_valid_ptr(cap_ptr)) {
      cap = *cap_ptr;
    }
    if (has_europe && player_level > 0) {
      const std::uint32_t eu_cap = static_cast<std::uint32_t>(player_level) * 2;
      if (eu_cap < cap) {
        cap = eu_cap;
      }
    }
    return cap;
  }

  // 2E. Get CoolTime Manager (*(CGInterface + 0x794) via sub_85E7A0)
  [[nodiscard]] static auto get_cooltime_mgr() -> void* {
    auto** cg_if = reinterpret_cast<void***>(k_cg_interface_ptr);
    if (!cg_if || !ext_client::utils::memory::is_valid_ptr(cg_if) || !*cg_if) {
      return nullptr;
    }
    using get_mgr_fn = void*(__thiscall*)(void*);
    const auto get_mgr = ext_client::off::as_fn<get_mgr_fn>(k_get_cooltime_mgr_fn);
    if (!get_mgr) return nullptr;
    void* cooltime_mgr = get_mgr(*cg_if);
    return ext_client::utils::memory::is_valid_ptr(cooltime_mgr) ? cooltime_mgr : nullptr;
  }

  // 2E. Remaining cooldown in milliseconds (sub_B0C2B0: returns remaining ms, or <= 0 if ready)
  [[nodiscard]] static auto get_cooldown_remaining_ms(std::uint32_t skill_id) -> int {
    if (skill_id == 0) return 0;
    void* cooltime_mgr = get_cooltime_mgr();
    if (!cooltime_mgr) return 0;

    using check_cd_fn = int(__thiscall*)(void*, std::uint32_t);
    const auto check_cd = ext_client::off::as_fn<check_cd_fn>(k_is_skill_cooldown_fn);
    if (!check_cd) return 0;

    return check_cd(cooltime_mgr, skill_id);
  }

  [[nodiscard]] static auto is_skill_ready(std::uint32_t skill_id) -> bool {
    return get_cooldown_remaining_ms(skill_id) <= 0;
  }

  // 2F. Cooldown Status Check (boolean)
  [[nodiscard]] static auto is_on_cooldown(std::uint32_t skill_id) -> bool {
    return !is_skill_ready(skill_id);
  }

  // 3. Native In-Game Skill Cast (Simulates Quickbar Action Dispatcher)
  static auto cast_skill_native(std::uint32_t skill_id) -> bool {
    if (skill_id == 0) return false;
    using cast_fn = int(__stdcall*)(std::uint32_t, char);
    const auto fn = ext_client::off::as_fn<cast_fn>(k_cast_skill_ui_fn);
    if (!fn) return false;
    return fn(skill_id, 1) != 0;
  }

  // 4. Low-Level Packet Injection Skill Cast (Opcode 0x7074)
  static auto send_cast_skill_packet(std::uint32_t skill_id, std::uint32_t target_id = 0) -> bool {
    return ext_client::net::injection::send_cast_skill(skill_id, target_id);
  }
};
