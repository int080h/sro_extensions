#include "pch.hpp"
#include "sdk/ui/cif_slot_with_help.hpp"
#include "sdk/game/cglobal_data_manager.hpp"
#include "sdk/game/c_skill_manager.hpp"
#include "sdk/ui/cui_string_manager.hpp"
#include "utils/string.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdio>
#include <cmath>
#include <sstream>

namespace ext_client::sdk::ui {

  namespace {

    // Translation of a client key. Empty when the key is unknown: the client prints nothing then either.
    inline auto loc(const wchar_t* key) -> std::wstring {
      return ext_client::sdk::ui::get_string(key);
    }

    inline auto to_u8(const std::wstring& w) -> std::string {
      return w.empty() ? std::string{} : ext_client::utils::string::to_utf8(w.c_str());
    }

    // Decode a magic parameter dynamically using client's localized strings (sub_74E630)
    auto format_magic_param(std::uint16_t id, std::uint32_t value) -> std::string {
      const auto opt_name = cglobal_data_manager::find_magic_option_name(id);
      wchar_t wbuf[128]{};

      auto matches = [&](const wchar_t* tag) -> bool {
        return opt_name.find(tag) != std::wstring::npos;
      };

      const auto s_inc   = loc(L"PARAM_INCREASE");
      const auto s_dec   = loc(L"PARAM_DECREASE");
      const auto s_count = loc(L"UIIT_STT_COUNT");

      if (matches(L"MATTR_STR_AVATAR") || matches(L"MATTR_STR")) {
        const auto s_str = loc(L"PARAM_STR");
        swprintf_s(wbuf, L"%s %u %s", s_str.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_INT_AVATAR") || matches(L"MATTR_INT")) {
        const auto s_int = loc(L"PARAM_INT");
        swprintf_s(wbuf, L"%s %u %s", s_int.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_DUR")) {
        const auto s_dur = loc(L"PARAM_DUR");
        swprintf_s(wbuf, L"%s %u%% %s", s_dur.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_HR")) {
        const auto s_hr = loc(L"PARAM_HR");
        swprintf_s(wbuf, L"%s %u%% %s", s_hr.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_ER")) {
        const auto s_er = loc(L"PARAM_ER");
        swprintf_s(wbuf, L"%s %u%% %s", s_er.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_HP")) {
        const auto s_hp = loc(L"PARAM_HP");
        swprintf_s(wbuf, L"%s %u %s", s_hp.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_MP")) {
        const auto s_mp = loc(L"PARAM_MP");
        swprintf_s(wbuf, L"%s %u %s", s_mp.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_CRITICAL")) {
        const auto s_crit = loc(L"PARAM_CRITICAL");
        swprintf_s(wbuf, L"%s %u %s", s_crit.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_BLOCKRATE")) {
        const auto s_block = loc(L"PARAM_BLOCKING");
        swprintf_s(wbuf, L"%s %u %s", s_block.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_EVADE_BLOCK")) {
        const auto s_eb = loc(L"PARAM_IGNORE_BLOCKING");
        swprintf_s(wbuf, L"%s %u%%", s_eb.c_str(), value);
      } else if (matches(L"MATTR_EVADE_CRITICAL")) {
        const auto s_ec = loc(L"PARAM_EVADE_CRITICAL");
        swprintf_s(wbuf, L"%s %u%%", s_ec.c_str(), value);
      } else if (matches(L"MATTR_RESIST_FROSTBITE")) {
        const auto s_fz = loc(L"PARAM_FZ");
        const auto s_fb = loc(L"PARAM_FB");
        swprintf_s(wbuf, L"%s,%s %u%% %s", s_fz.c_str(), s_fb.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_ESHOCK")) {
        const auto s_es = loc(L"PARAM_ES");
        swprintf_s(wbuf, L"%s %u%% %s", s_es.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_BURN")) {
        const auto s_bu = loc(L"PARAM_BU");
        swprintf_s(wbuf, L"%s %u%% %s", s_bu.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_POISON")) {
        const auto s_ps = loc(L"PARAM_PS");
        swprintf_s(wbuf, L"%s %u%% %s", s_ps.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_ZOMBIE")) {
        const auto s_zb = loc(L"PARAM_ZB");
        swprintf_s(wbuf, L"%s %u%% %s", s_zb.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_STUN")) {
        const auto s_st = loc(L"PARAM_STUN");
        swprintf_s(wbuf, L"%s %u%% %s", s_st.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_CSMP")) {
        const auto s_cs = loc(L"PARAM_CURSIE_MP");
        swprintf_s(wbuf, L"%s %u%% %s", s_cs.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_DISEASE")) {
        const auto s_dis = loc(L"PARAM_DISEASE");
        swprintf_s(wbuf, L"%s %u%% %s", s_dis.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_SLEEP")) {
        const auto s_sl = loc(L"PARAM_SLEEP");
        swprintf_s(wbuf, L"%s %u%% %s", s_sl.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_RESIST_FEAR")) {
        const auto s_fe = loc(L"PARAM_FEAR");
        swprintf_s(wbuf, L"%s %u%% %s", s_fe.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_LUCK")) {
        const auto s_luck = loc(L"PARAM_LUCK");
        swprintf_s(wbuf, L"%s (%u %s)", s_luck.c_str(), value, s_count.c_str());
      } else if (matches(L"MATTR_SOLID")) {
        const auto s_solid = loc(L"PARAM_SOLID");
        swprintf_s(wbuf, L"%s (%u %s)", s_solid.c_str(), value, s_count.c_str());
      } else if (matches(L"MATTR_ATHANASIA")) {
        const auto s_ath = loc(L"PARAM_ATHANASIA");
        swprintf_s(wbuf, L"%s (%u %s)", s_ath.c_str(), value, s_count.c_str());
      } else if (matches(L"MATTR_ASTRAL")) {
        const auto s_ast = loc(L"PARAM_ASTRAL");
        swprintf_s(wbuf, L"%s (%u %s)", s_ast.c_str(), value, s_count.c_str());
      } else if (matches(L"MATTR_REPAIR")) {
        const auto s_rep = loc(L"PARAM_REPAIR");
        swprintf_s(wbuf, L"%s (%u %s)", s_rep.c_str(), value, s_count.c_str());
      } else if (matches(L"MATTR_REGENHPMP")) {
        const auto s_hp = loc(L"PARAM_HEAL_HP");
        const auto s_mp = loc(L"PARAM_HEAL_MP");
        swprintf_s(wbuf, L"%s/%s %u%% %s", s_hp.c_str(), s_mp.c_str(), value, s_inc.c_str());
      } else if (matches(L"MATTR_DEC_MAXDUR")) {
        const auto s_mdur = loc(L"PARAM_MAX_DURABILITY");
        swprintf_s(wbuf, L"%s %u%% %s", s_mdur.c_str(), value, s_dec.c_str());
      } else if (matches(L"MATTR_NOT_REPARABLE")) {
        const auto s_nr = loc(L"PARAM_NOT_REPAIRABLE");
        swprintf_s(wbuf, L"%s", s_nr.c_str());
      } else if (!opt_name.empty()) {
        const auto u8_name = ext_client::utils::string::to_utf8(opt_name.c_str());
        char buf[128]{};
        std::snprintf(buf, sizeof(buf), "%s: %u", u8_name.c_str(), value);
        return std::string(buf);
      } else {
        return std::string{};
      }

      return ext_client::utils::string::to_utf8(wbuf);
    }

    // Item name (sub_75DFE0): the localized name of the CRefObjItem, empty when it has none
    auto resolve_item_title(const cref_obj_item* ref) -> std::string {
      return ref ? ref->name() : std::string{};
    }

    // Devil option line (sub_74E630 handling of MATTR_NASRUN_*), falling back to the generic formatter
    auto format_devil_option(std::uint16_t id, std::uint32_t value) -> std::string {
      const auto opt_name = cglobal_data_manager::find_magic_option_name(id);
      auto has = [&](const wchar_t* tag) -> bool {
        return opt_name.find(tag) != std::wstring::npos;
      };

      const auto s_inc = loc(L"PARAM_INCREASE");
      if (has(L"MATTR_NASRUN_HPNA") || has(L"MATTR_NASRUN_MPNA")) {
        const auto stat = has(L"MATTR_NASRUN_HPNA") ? loc(L"PARAM_HP") : loc(L"PARAM_MP");
        return to_u8(loc(L"PARAM_MAX") + stat + L" " + std::to_wstring(value) + L"% " + s_inc);
      }
      if (has(L"MATTR_NASRUN_UMDU")) {
        return to_u8(std::to_wstring(value) + L"% " + loc(L"PARAM_NASRUN_UMDU") + L" (+0%)");
      }
      if (has(L"MATTR_NASRUN_BLOCKRATE")) {
        return to_u8(loc(L"PARAM_BLOCKING") + L" " + std::to_wstring(value) + L" " + s_inc + L" (+0%)");
      }
      return format_magic_param(id, value);
    }

    // sub_74CE80: description key lives at ref + 0x7C. Devil spirits try "<key>_<level>" from the
    // item's opt level downwards until a translation exists.
    auto resolve_item_description(const cref_obj_item* ref, const cso_item* item) -> std::string {
      if (!ref) return "";
      const auto key = ref->description_key();
      if (key.empty()) return "";

      if (item && ref->is_devil_spirit()) {
        for (int lvl = item->opt_level(); lvl >= 0; --lvl) {
          const auto candidate = key + L"_" + std::to_wstring(lvl);
          const auto text = loc(candidate.c_str());
          if (!text.empty()) return to_u8(text);
        }
      }
      return to_u8(loc(key.c_str()));
    }

    auto mastery_name_key(std::int32_t type) -> const wchar_t* {
      switch (type) {
        case 257: return L"UIIT_STT_MASTERY_VI";
        case 258: return L"UIIT_STT_MASTERY_HEUK";
        case 259: return L"UIIT_STT_MASTERY_PA";
        case 276: return L"UIIT_STT_MASTERY_GI";
        case 277: return L"UIIT_CTL_FORCE_SKILL";
        case 513: return L"UIIT_STT_WARRIOR";
        case 514: return L"UIIT_STT_WIZARD";
        case 515: return L"UIIT_STT_ROG";
        case 516: return L"UIIT_STT_WARLOCK";
        case 517: return L"UIIT_STT_BARD";
        case 518: return L"UIIT_STT_CLERIC";
        default:  return nullptr;
      }
    }

    // sub_766B90: requirement table, STR/INT, gender and race lines
    auto resolve_requirements(const cref_obj_item* ref, item_tooltip_data& d) -> void {
      auto add = [&](const std::wstring& line) {
        if (!line.empty()) d.requirement_lines.push_back(to_u8(line));
      };

      const auto race = ref->race_code();
      for (std::size_t i = 0; i < 4; ++i) {
        const auto type = ref->requirement_type(i);
        if (type == -1) continue;
        const auto value = ref->requirement_value(i);

        if (type == 1) {
          if (d.req_level == 0 && value > 0) d.req_level = static_cast<std::uint32_t>(value);
          add(loc(L"PARAM_REQ_LV") + L" " + std::to_wstring(value));
        } else if (type == 2) {
          const wchar_t* fmt = (race == 0) ? L"UIIT_STT_CLASS_MERCHANT_" : (race == 1 ? L"UIIT_STT_CLASS_EU_MERCHANT_" : nullptr);
          if (fmt) {
            const auto rank_key = std::wstring(fmt) + std::to_wstring(value);
            add(loc(L"UIIT_STT_CHAR_JOBGRADE") + L": " + loc(rank_key.c_str()));
          }
        } else if (type == 3 || type == 4) {
          add(loc(L"UIIT_STT_CONFLICT_JOB_LEVEL") + L": " + std::to_wstring(value));
        } else if (type == 10) {
          add(loc(L"UIIT_STT_GUILD_REQUIRE_LEVEL") + L" " + std::to_wstring(value));
        } else if (const auto* mastery_key = mastery_name_key(type)) {
          add(loc(L"PARAM_MASTERY_LEVEL") + L": " + loc(mastery_key) + L" " +
              loc(L"PARAM_MASTERY") + L" " + std::to_wstring(value));
        }
      }

      if (ref->required_str() != 0) {
        add(loc(L"PARAM_STR") + L" " + std::to_wstring(ref->required_str()));
      }
      if (ref->required_int() != 0) {
        add(loc(L"PARAM_INT") + L" " + std::to_wstring(ref->required_int()));
      }

      // Gender (0 = Female, 1 = Male, 2 = unrestricted, 3/4/5 = pet types)
      std::wstring gender;
      switch (ref->gender_code()) {
        case 0: gender = loc(L"UIO_NEWCHAR_CTL_FEMALE"); break;
        case 1: gender = loc(L"UIO_NEWCHAR_CTL_MALE"); break;
        case 3:
          d.is_pet_item = true;
          d.pet_type_name = to_u8(loc(L"UIIT_STT_PET2_PETTYPE_ASS"));
          break;
        case 4:
          d.is_pet_item = true;
          d.pet_type_name = to_u8(loc(L"UIIT_STT_PET2_PETTYPE_PRO"));
          break;
        case 5:
          d.is_pet_item = true;
          d.pet_type_name = to_u8(loc(L"UIIT_STT_PET2_PETTYPE_ENC"));
          break;
        default: break;
      }
      if (!gender.empty()) {
        d.gender_name = to_u8(gender);
        add(gender);
      }

      // Race (0 = Chinese, 1 = European, 2 = Arabian, 3 = unrestricted)
      std::wstring race_text;
      switch (race) {
        case 0: race_text = loc(L"UIO_NEWCHAR_CTL_CHINESE"); break;
        case 1: race_text = loc(L"UIO_NEWCHAR_CTL_EUROPEAN"); break;
        case 2: race_text = loc(L"UIO_NEWCHAR_CTL_ARABIAN"); break;
        default: break;
      }
      if (!race_text.empty()) {
        d.race_name = to_u8(race_text);
        add(race_text);
      }
    }

    // sub_74A0A0 (magic option slots) and sub_768770 (attachment wear state)
    auto build_avatar_lines(const cref_obj_item* ref, item_tooltip_data& d) -> void {
      const auto label = loc(L"UIIT_STT_AVATAR_MAGICOPTION_MAXCOUNT");
      const auto unit  = loc(L"UIIT_STT_UNIT");
      d.avatar_magic_slots_text = to_u8(label + L": " + std::to_wstring(ref->avatar_magic_slot_count()) + unit);

      if (ref->native_tid4() == 2) {
        const auto attach = loc(L"UIIT_STT_SILKMALL_ATTACH");
        const auto state = ref->avatar_attach_wear_flag()
          ? loc(L"UIIT_STT_AVATAR_WEAR")
          : loc(L"UIIT_STT_AVATAR_NOT_WEAR");
        d.avatar_wear_text = to_u8(attach + L": " + state);
      }
    }

    // sub_75E9E0: devil option sections and awaken period
    auto build_devil_lines(const cref_obj_item* ref, const cso_item* item, item_tooltip_data& d) -> void {
      if (!item) return;

      cglobal_data_manager::devil_option_entry entries[48]{};
      const int count = cglobal_data_manager::query_devil_options(item->ref_id(), item->opt_level(), entries, 48);

      static constexpr const wchar_t* k_heading_keys[3] = {
        L"UIIT_STT_NASRUN_BASIC_OPTION", L"UIIT_STT_NASRUN_ADD_OPTION", L"UIIT_STT_NASRUN_MAGIC_OPTION"
      };
      static constexpr std::uint32_t k_colors[3] = {
        tooltip_argb::white, tooltip_argb::devil_add, tooltip_argb::magic
      };

      for (int g = 0; g < 3; ++g) {
        tooltip_section section{};
        section.heading = to_u8(loc(k_heading_keys[g]));
        section.heading_argb = tooltip_argb::heading;
        for (int i = 0; i < count; ++i) {
          const auto& e = entries[i];
          if (e.group != g) continue;
          const auto text = e.generic ? format_devil_option(e.opt_id, e.value) : to_u8(loc(e.key));
          if (!text.empty()) {
            section.lines.push_back({text, k_colors[g]});
          }
        }
        if (!section.lines.empty()) {
          d.devil_sections.push_back(std::move(section));
        }
      }

      // Own magic options are listed under the "magic option" heading (sub_9D5D80 -> sub_75C2D0)
      if (item->magic_param_count() != 0) {
        d.blues_heading = to_u8(loc(k_heading_keys[2]));
      }

      // Awaken period: state 0 = default period of the template, state 1 = remaining time
      const auto state = item->awaken_state();
      if (state == 0) {
        d.awaken_header = to_u8(loc(L"UIIT_STT_NASRUN_AWAKE_TIME"));
        const auto period = ref->awaken_period_seconds();
        if (period > 0) {
          d.awaken_text = to_u8(std::to_wstring(period / 86400) + loc(L"PARAM_DAY"));
          d.is_devil_spirit_active = true;
          d.is_expired = false;
        } else {
          d.awaken_text = to_u8(loc(L"UIIT_STT_NASRUN_NOT_AWAKE"));
          d.is_devil_spirit_active = false;
          d.is_expired = true;
        }
      } else if (state == 1) {
        d.awaken_header = to_u8(loc(L"UIIT_STT_NASRUN_AWAKE_TIME"));
        const auto secs = item->awaken_remaining_ms() / 1000;
        if (secs > 0) {
          d.is_devil_spirit_active = true;
          d.is_expired = false;
          const auto days  = secs / 86400;
          const auto hours = (secs % 86400) / 3600;
          const auto mins  = ((secs % 86400) % 3600) / 60;
          d.awaken_text = to_u8(
            std::to_wstring(days)  + loc(L"PARAM_DAY")  + L" " +
            std::to_wstring(hours) + loc(L"PARAM_HOUR") + L" " +
            std::to_wstring(mins)  + loc(L"PARAM_MINUTE"));
        } else {
          d.is_devil_spirit_active = false;
          d.is_expired = true;
          d.awaken_text = to_u8(loc(L"UIIT_STT_NASRUN_AWAKE_TIMEOVER"));
        }
      }
    }

  } // namespace

  // ---------------------------------------------------------------------------
  // Native localization resolvers
  // ---------------------------------------------------------------------------

  auto resolve_native_item_type_name(const cref_obj_item* ref) -> std::string {
    if (!ref || !ext_client::utils::memory::is_valid_ptr(ref)) return "";

    wchar_t key_buf[128]{};
    const void* code_name_ptr = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(ref) + 0x008);
    if (!cglobal_data_manager::resolve_item_type_key(ref->type_id(), code_name_ptr, key_buf, 128)) return "";
    return to_u8(loc(key_buf));
  }

  auto resolve_sox_subtitle(const cref_obj_item* ref) -> std::string {
    if (!ref || !ref->is_sox()) return "";

    const auto cb = ref->class_byte();
    const auto tier = ref->sox_tier();
    const wchar_t* key = nullptr;
    if (cb >= 34) {
      key = (tier == 0) ? L"UIIT_STT_UPGRADE_ITEM_GRADE_MAGIC"
          : (tier == 1) ? L"UIIT_STT_UPGRADE_ITEM_GRADE_RARE"
                        : L"UIIT_STT_UPGRADE_ITEM_GRADE_LEGEND";
    } else if (tier == 0) {
      key = (cb >= 31) ? L"PARAM_RARE_FIRST2" : L"PARAM_RARE_FIRST";
    } else {
      key = (tier == 1) ? L"PARAM_RARE_SECOND" : L"PARAM_RARE_THIRD";
    }
    return to_u8(loc(key));
  }

  auto resolve_armor_position_name(std::uint8_t pos) -> std::string {
    const wchar_t* key = nullptr;
    switch (pos) {
      case 1: key = L"UIIT_STT_ARMOR_POSITION_HEAD"; break;
      case 2: key = L"UIIT_STT_ARMOR_POSITION_SHOULDER"; break;
      case 3: key = L"UIIT_STT_ARMOR_POSITION_BREAST"; break;
      case 4: key = L"UIIT_STT_ARMOR_POSITION_LEG"; break;
      case 5: key = L"UIIT_STT_ARMOR_POSITION_HAND"; break;
      case 6: key = L"UIIT_STT_ARMOR_POSITION_FOOT"; break;
      default: return "";
    }
    return to_u8(loc(key));
  }

  // ---------------------------------------------------------------------------
  // Tooltip builders (dispatcher sub_76C0E0)
  // ---------------------------------------------------------------------------

  namespace {

    auto build_gear_lines(const cref_obj_item* ref, item_tooltip_data& d) -> void {
      d.degree       = ref->degree();
      d.attack_range = ref->attack_range();
      if (ref->is_sox()) {
        d.subtitle = resolve_sox_subtitle(ref);
      }
    }

    auto read_safe_uintptr(std::uintptr_t addr) -> std::uintptr_t {
      __try {
        return *reinterpret_cast<std::uintptr_t*>(addr);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
      }
    }

    auto extract_attached_skill_ids(const cref_obj_item* ref, std::vector<std::uint32_t>& out_ids) -> void {
      if (!ref) return;
      const std::uintptr_t vec_ptr = read_safe_uintptr(reinterpret_cast<std::uintptr_t>(ref) + 1352);
      if (!vec_ptr || !ext_client::utils::memory::is_valid_ptr(reinterpret_cast<void*>(vec_ptr))) return;

      const std::uintptr_t first = read_safe_uintptr(vec_ptr + 4);
      const std::uintptr_t last = read_safe_uintptr(vec_ptr + 8);

      if (first && last && last >= first && (last - first) <= 32 * 20) {
        for (std::uintptr_t cur = first; cur < last; cur += 32) {
          const std::uint32_t skill_id = static_cast<std::uint32_t>(read_safe_uintptr(cur + 28));
          if (skill_id != 0) {
            out_ids.push_back(skill_id);
          }
        }
      }
    }

    auto fill_from_ref(const cref_obj_item* ref, const cso_item* item, item_tooltip_data& d) -> void {
      if (!ref) return;

      d.code_name = ext_client::utils::string::to_utf8(ref->code_name().c_str());
      d.slot_name = resolve_native_item_type_name(ref);
      d.description = resolve_item_description(ref, item);
      d.icon_path = ref->icon_path();
      d.is_sox = ref->is_sox();

      // Check if item is a Pet / Fellow summon scroll (sub_49FB30 / sub_76A9D0 / sub_76A250)
      if (ref->is_cos_pet() || (ref->gender_code() >= 3 && ref->gender_code() <= 5) ||
          d.code_name.rfind("ITEM_COS_", 0) == 0 || d.code_name.rfind("ITEM_PET_", 0) == 0) {
        d.is_pet_item = true;
      }

      if (ref->is_equipment()) {
        switch (ref->native_tid3()) {
          case 1: case 2: case 3: case 9: case 10: case 11:   // armor        sub_768B10
            d.is_armor = true;
            build_gear_lines(ref, d);
            d.armor_position_name = resolve_armor_position_name(ref->armor_position());
            break;
          case 4:                                              // shield       sub_7693D0
            d.is_shield = true;
            build_gear_lines(ref, d);
            break;
          case 5: case 12:                                     // accessory    sub_768EF0
            d.is_accessory = true;
            build_gear_lines(ref, d);
            break;
          case 6:                                              // weapon       sub_769160
            d.is_weapon = true;
            build_gear_lines(ref, d);
            break;
          case 7:                                              // job suit     sub_768550 (reduced tooltip)
            break;
          case 13:                                             // avatar       sub_768770
            d.is_avatar = true;
            build_avatar_lines(ref, d);
            break;
          case 14:                                             // devil spirit sub_75E9E0
            d.is_devil_spirit = true;
            build_devil_lines(ref, item, d);
            break;
          default:
            break;
        }
      }

      resolve_requirements(ref, d);

      // Pet details resolution (sub_76A250 / sub_76A9D0)
      if (d.is_pet_item) {
        // Pet type (Buff / Defensive / Attack)
        if (d.pet_type_name.empty()) {
          std::int32_t type_code = ref->gender_code();
          if (type_code < 3 || type_code > 5) {
            // Check attached char id from ref + 1352 (sub_76A250)
            const std::uintptr_t vec_ptr = read_safe_uintptr(reinterpret_cast<std::uintptr_t>(ref) + 1352);
            if (vec_ptr && ext_client::utils::memory::is_valid_ptr(reinterpret_cast<void*>(vec_ptr))) {
              const std::uintptr_t first = read_safe_uintptr(vec_ptr + 4);
              if (first) {
                const std::uint32_t char_id = static_cast<std::uint32_t>(read_safe_uintptr(first + 4));
                if (char_id) {
                  auto* ref_char = reinterpret_cast<std::uint8_t*>(cglobal_data_manager::get_ref_char(char_id));
                  if (ref_char && ext_client::utils::memory::is_valid_ptr(ref_char)) {
                    // sub_ABE120(ref_char) + 452 = ref_char + 8 + 452 = ref_char + 460
                    type_code = *reinterpret_cast<std::int32_t*>(ref_char + 460);
                  }
                }
              }
            }
          }

          if (type_code == 3) {
            d.pet_type_name = to_u8(loc(L"UIIT_STT_PET2_PETTYPE_ASS"));
          } else if (type_code == 4) {
            d.pet_type_name = to_u8(loc(L"UIIT_STT_PET2_PETTYPE_PRO"));
          } else if (type_code == 5) {
            d.pet_type_name = to_u8(loc(L"UIIT_STT_PET2_PETTYPE_ENC"));
          } else {
            d.pet_type_name = to_u8(loc(L"UIIT_STT_PET2_PETTYPE_PRO"));
          }
        }

        // Pet name (UIIT_STT_COSNEWUI_TITLE fallback to "Noname")
        d.pet_name = to_u8(loc(L"UIIT_STT_COSNEWUI_TITLE"));
        if (d.pet_name.empty()) d.pet_name = "Noname";

        // Pet level (defaults to 1 or item runtime level)
        if (item && item->pet_level() > 0) {
          d.pet_level = item->pet_level();
        } else {
          d.pet_level = (d.req_level > 0) ? d.req_level : 1;
        }

        // Pet condition
        if (item && item->pet_condition() == 4) {
          d.pet_condition = to_u8(loc(L"UIIT_STT_COSNEWUI_TOOLTIP_PETSTATE_DEAD"));
        } else {
          d.pet_condition = to_u8(loc(L"UIIT_STT_COSNEWUI_TOOLTIP_PETSTATE_NORMAL"));
        }
        if (d.pet_condition.empty()) d.pet_condition = "Normal";
      } else {
        // 1. Recovery items, potions, vigor grains, and cure pills (sub_74B950)
        if (ref->is_consumable_family() && (ref->native_tid3() == 1 || ref->native_tid3() == 2 || ref->native_tid3() == 3)) {
          const auto p1 = ref->param1();
          const auto p2 = ref->param2();
          const auto p3 = ref->param3();
          const auto p4 = ref->param4();
          const auto p5 = ref->param5();
          const auto p6 = ref->param6();

          // Check if Pet HGP potion (native_tid4 == 9)
          if (ref->native_tid4() == 9) {
            if (p1 > 0) {
              const auto s_hgp = to_u8(loc(L"PARAM_HEAL_HGP"));
              d.effect_lines.push_back({(!s_hgp.empty() ? s_hgp : "HGP recovery") + " [" + std::to_string(p1) + "%]", tooltip_argb::white});
            }
          } else if (ref->native_tid4() == 12) {
            // Open market time
            if (p1 > 0) {
              const auto s_omt = to_u8(loc(L"UIIT_STT_OPEN_MARKET_TIME"));
              const auto s_day = to_u8(loc(L"PARAM_DAY"));
              d.effect_lines.push_back({(!s_omt.empty() ? s_omt : "Open Market Time") + ": " + std::to_string(p1 / 86400) + " " + (!s_day.empty() ? s_day : "Day"), tooltip_argb::white});
            }
          } else if (ref->native_tid3() == 1 || ref->native_tid3() == 2) {
            // HP / MP / Vigor Recovery Potions & Grains
            const auto s_hp = to_u8(loc(L"PARAM_HEAL_HP"));
            const auto s_mp = to_u8(loc(L"PARAM_HEAL_MP"));
            const std::string hp_lbl = !s_hp.empty() ? s_hp : "HP recovery";
            const std::string mp_lbl = !s_mp.empty() ? s_mp : "MP recovery";

            // Flat HP recovery (Param1)
            if (p1 > 0) {
              d.effect_lines.push_back({hp_lbl + " " + std::to_string(p1), tooltip_argb::white});
            }
            // Percentage HP recovery (Param2, e.g. Vigor grain 25 %)
            if (p2 > 0) {
              d.effect_lines.push_back({hp_lbl + " " + std::to_string(p2) + " %", tooltip_argb::white});
            }
            // Flat MP recovery (Param3)
            if (p3 > 0) {
              d.effect_lines.push_back({mp_lbl + " " + std::to_string(p3), tooltip_argb::white});
            }
            // Percentage MP recovery (Param4, e.g. Vigor grain 25 %)
            if (p4 > 0) {
              d.effect_lines.push_back({mp_lbl + " " + std::to_string(p4) + " %", tooltip_argb::white});
            }

            // Universal / Cure pills (sub_74B950)
            if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0) {
              const auto s_cure = to_u8(loc(L"PARAM_CURE_STATE"));
              d.effect_lines.push_back({(!s_cure.empty() ? s_cure : "Bad status recovery") + " " + std::to_string(p1), tooltip_argb::white});
            } else {
              if (p1 > 0 && p2 == 0 && p3 == 0 && p4 == 0 && ref->native_tid3() == 2) {
                const auto s_fz = to_u8(loc(L"PARAM_CURE_FROZEN_LV"));
                if (!s_fz.empty()) d.effect_lines.push_back({s_fz + " " + std::to_string(p1), tooltip_argb::white});
              }
              if (p2 > 0 && p1 == 0 && p3 == 0 && p4 == 0 && ref->native_tid3() == 2) {
                const auto s_fb = to_u8(loc(L"PARAM_CURE_FROSTBITE_LV"));
                if (!s_fb.empty()) d.effect_lines.push_back({s_fb + " " + std::to_string(p2), tooltip_argb::white});
              }
              if (p3 > 0 && p1 == 0 && p2 == 0 && p4 == 0 && ref->native_tid3() == 2) {
                const auto s_bn = to_u8(loc(L"PARAM_CURE_BURN_LV"));
                if (!s_bn.empty()) d.effect_lines.push_back({s_bn + " " + std::to_string(p3), tooltip_argb::white});
              }
              if (p4 > 0 && p1 == 0 && p2 == 0 && p3 == 0 && ref->native_tid3() == 2) {
                const auto s_sh = to_u8(loc(L"PARAM_CURE_ESHOCK_LV"));
                if (!s_sh.empty()) d.effect_lines.push_back({s_sh + " " + std::to_string(p4), tooltip_argb::white});
              }
              if (p5 > 0) {
                const auto s_ps = to_u8(loc(L"PARAM_CURE_POISON_LV"));
                if (!s_ps.empty()) d.effect_lines.push_back({s_ps + " " + std::to_string(p5), tooltip_argb::white});
              }
              if (p6 > 0) {
                const auto s_zb = to_u8(loc(L"PARAM_CURE_ZOMBIE_LV"));
                if (!s_zb.empty()) d.effect_lines.push_back({s_zb + " " + std::to_string(p6), tooltip_argb::white});
              }
            }
          }
        }

        // 2. Attached skills / buff effects on consumables & scrolls (sub_769640 / sub_75BA30)
        std::vector<std::uint32_t> attached_skills;
        extract_attached_skill_ids(ref, attached_skills);
        for (const auto sid : attached_skills) {
          const auto s_details = c_skill_manager::get_skill_details(sid);
          if (!s_details.effects_text.empty()) {
            std::stringstream ss(s_details.effects_text);
            std::string eline;
            while (std::getline(ss, eline)) {
              while (!eline.empty() && (eline.back() == '\r' || eline.back() == ' ')) eline.pop_back();
              if (!eline.empty()) {
                d.effect_lines.push_back({eline, tooltip_argb::white});
              }
            }
          }
        }
      }
    }

  } // namespace

  auto extract_tooltip_data_from_ref(const cref_obj_item* ref, std::uint8_t opt_level) -> item_tooltip_data {
    item_tooltip_data d{};
    if (!ref) return d;
    d.title     = resolve_item_title(ref);
    d.opt_level = opt_level;
    d.requirement_lines.reserve(8);
    fill_from_ref(ref, nullptr, d);

    return d;
  }

  auto extract_tooltip_data(const cso_item* item) -> item_tooltip_data {
    item_tooltip_data d{};
    if (!item || !item->is_valid()) return d;

    auto* ref = item->get_ref_item();
    d.title     = resolve_item_title(ref);
    d.opt_level = item->opt_level();
    d.stack_count = item->count();
    d.adv_elixir_level = item->adv_elixir_level();

    if (ref && ref->is_equipment()) {
      d.current_durability = item->durability();
      const auto var_raw = item->variance_raw();
      d.durability_pct   = static_cast<std::uint8_t>(var_raw & 0x1F); // 0..31
    }

    d.requirement_lines.reserve(8);
    fill_from_ref(ref, item, d);

    // Computed Stats Structure (Only valid for equipment items: sub_4992A0 / sub_76C0E0)
    if (ref && ref->is_equipment()) {
      const auto* stats = item->stats();
      if (stats) {
        d.min_phy_atk = stats->min_phy_atk;
        d.max_phy_atk = stats->max_phy_atk;
        d.phy_atk_pct = stats->phy_atk_pct;

        d.min_mag_atk = stats->min_mag_atk;
        d.max_mag_atk = stats->max_mag_atk;
        d.mag_atk_pct = stats->mag_atk_pct;

        d.phy_def     = stats->phy_def;
        d.phy_def_pct = stats->phy_def_pct;

        d.mag_def     = stats->mag_def;
        d.mag_def_pct = stats->mag_def_pct;

        d.min_phy_reinforce = stats->min_phy_reinforce;
        d.max_phy_reinforce = stats->max_phy_reinforce;
        d.phy_reinforce_pct = stats->phy_reinforce_pct;

        d.min_mag_reinforce = stats->min_mag_reinforce;
        d.max_mag_reinforce = stats->max_mag_reinforce;
        d.mag_reinforce_pct = stats->mag_reinforce_pct;

        d.max_durability = stats->max_durability;
        d.hit_rate       = stats->hit_rate;
        d.hit_pct        = stats->hit_pct;

        d.parry_rate     = stats->parry_rate;
        d.parry_pct      = stats->parry_pct;

        d.critical       = stats->critical;
        d.critical_pct   = stats->critical_pct;

        d.blocking_rate  = stats->blocking_rate;
        d.blocking_pct   = stats->blocking_pct;
      }

      // Sockets
      const auto sockets = item->sockets();
      d.sockets.reserve(sockets.size());
      for (std::size_t i = 0; i < sockets.size(); ++i) {
        char sock_buf[64]{};
        std::snprintf(sock_buf, sizeof(sock_buf), "Socket #%zu: Stone ID %u (Param: %u)", i + 1, sockets[i].stone_id, sockets[i].param);
        d.sockets.emplace_back(sock_buf);
      }

      // Magic Options (Blues)
      const auto params = item->magic_params();
      d.blues.reserve(params.size());
      for (const auto& mp : params) {
        const auto line = format_magic_param(mp.id, mp.value);
        if (!line.empty()) d.blues.push_back(line);
      }
    }

    return d;
  }

} // namespace ext_client::sdk::ui
