#include "pch.hpp"
#include "plugins/character_select/character_select_plugin.hpp"

#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "sdk/game/ctext_string_manager.hpp"
#include "sdk/game/cic_deco_character.hpp"
#include "sdk/net/packet_builder.hpp"
#include "sdk/process/cprocess.hpp"
#include "sdk/process/cps_character_select.hpp"
#include "sdk/ui/cif_static.hpp"
#include "utils/log.hpp"

#include <Windows.h>
#include <array>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::character_select {

  // =========================================================================
  // 1. Constants
  // =========================================================================
  inline constexpr int k_last_logout_left_margin = 24;
  inline constexpr int k_last_logout_button_gap = 12;
  inline constexpr int k_cif_align_left = 0;
  inline constexpr int k_last_logout_fallback_min_width = 240;
  inline constexpr std::size_t k_max_char_slots = 16;

  inline constexpr int k_greeting_emotion_anim_id = 0x32; // ANI_EMOTION01
  inline constexpr int k_greeting_anim_arg1 = 0;
  inline constexpr int k_greeting_anim_arg2 = 100;
  inline constexpr int k_greeting_anim_arg3 = 0;
  inline constexpr float k_greeting_anim_speed = 1.0f;
  inline constexpr float k_greeting_anim_start = 1.0f;
  inline constexpr int k_greeting_delay_frames = 20;

  // =========================================================================
  // 2. Structures
  // =========================================================================
  struct elapsed_logout_parts {
    std::uint32_t months = 0;
    std::uint32_t days = 0;
    std::uint32_t hours = 0;
    std::uint32_t minutes = 0;
  };

  extern int g_greeting_delay_frames_left;
  extern int g_greeting_target_slot;
  extern cic_deco_character* g_greeting_target_entity;

  // =========================================================================
  // 4. Helper Function Declarations
  // =========================================================================
  auto is_leap_year(int year) -> bool;
  auto days_in_month(int year, int month) -> int;
  auto add_one_month(SYSTEMTIME& time) -> bool;
  auto compare_system_time(const SYSTEMTIME& lhs, const SYSTEMTIME& rhs) -> int;
  auto diff_minutes_between(const SYSTEMTIME& start, const SYSTEMTIME& end) -> ULONGLONG;
  auto compute_elapsed_logout(std::uint32_t packed_time, elapsed_logout_parts& out) -> bool;


  auto selected_deco_character(cps_character_select* self, int slot) -> cic_deco_character*;
  auto clear_queued_greeting() -> void;
  auto play_greeting_on_character(cic_deco_character* entity) -> void;
  auto play_greeting_on_selected_slot(cps_character_select* self) -> void;

  auto dock_last_logout_label_left(cps_character_select* self, cif_static* label) -> void;
  auto update_last_logout_text(cps_character_select* self) -> void;

  // =========================================================================
  // 5. Named Event Handlers
  // =========================================================================
  auto handle_char_select_enter(char_select_enter_context& ctx) -> void;
  auto handle_char_select_slot_change(char_select_slot_change_context& ctx) -> void;
  auto handle_char_select_update(char_select_update_context& ctx) -> void;




  int g_greeting_delay_frames_left = -1;
  int g_greeting_target_slot = -1;
  cic_deco_character* g_greeting_target_entity = nullptr;

  // =========================================================================
  // 2. Helper Functions Implementation
  // =========================================================================
  auto is_leap_year(int year) -> bool {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  }

  auto days_in_month(int year, int month) -> int {
    static constexpr int k_days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
      return 0;
    }
    if (month == 2 && is_leap_year(year)) {
      return 29;
    }
    return k_days_per_month[month - 1];
  }

  auto add_one_month(SYSTEMTIME& time) -> bool {
    int year = time.wYear;
    int month = time.wMonth + 1;
    if (month > 12) {
      month = 1;
      ++year;
    }

    if (time.wDay > days_in_month(year, month)) {
      return false;
    }

    time.wYear = static_cast<WORD>(year);
    time.wMonth = static_cast<WORD>(month);
    return true;
  }

  auto compare_system_time(const SYSTEMTIME& lhs, const SYSTEMTIME& rhs) -> int {
    FILETIME lhs_file{};
    FILETIME rhs_file{};
    if (!SystemTimeToFileTime(&lhs, &lhs_file) || !SystemTimeToFileTime(&rhs, &rhs_file)) {
      return 0;
    }
    const auto lhs_value = (static_cast<ULONGLONG>(lhs_file.dwHighDateTime) << 32) | lhs_file.dwLowDateTime;
    const auto rhs_value = (static_cast<ULONGLONG>(rhs_file.dwHighDateTime) << 32) | rhs_file.dwLowDateTime;
    if (lhs_value < rhs_value) {
      return -1;
    }
    if (lhs_value > rhs_value) {
      return 1;
    }
    return 0;
  }

  auto diff_minutes_between(const SYSTEMTIME& start, const SYSTEMTIME& end) -> ULONGLONG {
    FILETIME start_file{};
    FILETIME end_file{};
    if (!SystemTimeToFileTime(&start, &start_file) || !SystemTimeToFileTime(&end, &end_file)) {
      return 0;
    }

    ULARGE_INTEGER start_value{};
    start_value.LowPart = start_file.dwLowDateTime;
    start_value.HighPart = start_file.dwHighDateTime;

    ULARGE_INTEGER end_value{};
    end_value.LowPart = end_file.dwLowDateTime;
    end_value.HighPart = end_file.dwHighDateTime;

    if (end_value.QuadPart <= start_value.QuadPart) {
      return 0;
    }

    return (end_value.QuadPart - start_value.QuadPart) / (10ULL * 1000ULL * 1000ULL * 60ULL);
  }

  auto compute_elapsed_logout(std::uint32_t packed_time, elapsed_logout_parts& out) -> bool {
    const WORD logout_month = static_cast<WORD>((packed_time >> 6) & 0xF);
    const WORD logout_day = static_cast<WORD>((packed_time >> 10) & 0x1F);
    const WORD logout_hour = static_cast<WORD>((packed_time >> 15) & 0x1F);
    const WORD logout_minute = static_cast<WORD>((packed_time >> 20) & 0x3F);

    if (logout_month < 1 || logout_month > 12 || logout_day == 0 || logout_hour > 23 || logout_minute > 59) {
      return false;
    }

    SYSTEMTIME now{};
    GetLocalTime(&now);

    SYSTEMTIME candidate{};
    candidate.wYear = now.wYear;
    candidate.wMonth = logout_month;
    candidate.wDay = logout_day;
    candidate.wHour = logout_hour;
    candidate.wMinute = logout_minute;
    candidate.wSecond = 0;
    candidate.wMilliseconds = 0;
    candidate.wDayOfWeek = 0;

    const int max_day = days_in_month(candidate.wYear, candidate.wMonth);
    if (logout_day > max_day) {
      return false;
    }

    while (compare_system_time(candidate, now) > 0) {
      if (candidate.wYear == 0) {
        return false;
      }
      --candidate.wYear;
    }

    if (compare_system_time(candidate, now) > 0) {
      return false;
    }

    elapsed_logout_parts result{};
    SYSTEMTIME cursor = candidate;

    while (result.months < 12) {
      SYSTEMTIME next = cursor;
      if (!add_one_month(next) || compare_system_time(next, now) > 0) {
        break;
      }
      cursor = next;
      ++result.months;
    }

    const ULONGLONG total_minutes = diff_minutes_between(cursor, now);
    result.days = static_cast<std::uint32_t>(total_minutes / (24ULL * 60ULL));
    result.hours = static_cast<std::uint32_t>((total_minutes / 60ULL) % 24ULL);
    result.minutes = static_cast<std::uint32_t>(total_minutes % 60ULL);

    out = result;
    return true;
  }


  // =========================================================================
  // 3. Character Select & Greeting Animation Logic
  // =========================================================================
  auto selected_deco_character(cps_character_select* self, int slot) -> cic_deco_character* {
    if (!self || slot == 255 || slot < 0) {
      return nullptr;
    }
    return self->get_deco_character_at(slot);
  }

  auto clear_queued_greeting() -> void {
    g_greeting_target_entity = nullptr;
    g_greeting_target_slot = -1;
    g_greeting_delay_frames_left = -1;
  }

  auto play_greeting_on_character(cic_deco_character* entity) -> void {
    if (!entity) {
      return;
    }
    log_msg("[character_select_plugin] playing char-select entity animation 0x%X on entity=%p", k_greeting_emotion_anim_id, entity);
    const bool played = entity->play_animation(k_greeting_emotion_anim_id,
                                               k_greeting_anim_arg1,
                                               k_greeting_anim_arg2,
                                               k_greeting_anim_arg3,
                                               k_greeting_anim_speed,
                                               k_greeting_anim_start);
    log_msg("[character_select_plugin] char-select entity animation %s", played ? "call returned" : "call skipped");
  }

  auto play_greeting_on_selected_slot(cps_character_select* self) -> void {
    if (!self)
      return;
    int slot = cps_character_select::get_selected_slot_index();
    if (slot == 255 || slot == -1)
      return;

    auto* entity = selected_deco_character(self, slot);
    if (entity) {
      g_greeting_target_entity = entity;
      g_greeting_target_slot = slot;
      g_greeting_delay_frames_left = k_greeting_delay_frames;
    }
  }



  auto dock_last_logout_label_left(cps_character_select* self, cif_static* label) -> void {
    if (!self || !label || !label->is_live()) {
      return;
    }

    int target_x = k_last_logout_left_margin;
    int target_w = label->get_rect_w();

    if (auto* start_button =
          reinterpret_cast<cgwnd*>(self->get_ui_child(0x0E/*select_button (14)*/, true))) {
      if (start_button->is_live()) {
        const int right_limit = start_button->get_rect_x() - k_last_logout_button_gap;
        if (right_limit > target_x) {
          target_w = right_limit - target_x;
        }
      }
    }

    if (target_w < k_last_logout_fallback_min_width) {
      target_w = k_last_logout_fallback_min_width;
    }

    label->set_position(target_x, label->get_rect_y());
    label->set_size(target_w, label->get_rect_h());
    label->set_align_h(k_cif_align_left);
    label->refresh_layout();
  }

  auto update_last_logout_text(cps_character_select* self) -> void {
    if (!self)
      return;

    int slot = cps_character_select::get_selected_slot_index();
    if (slot == 255 || slot == -1)
      return;

    pcinfo_ui* character = self->get_character_at(slot);
    if (!character)
      return;

    const std::uint32_t packed_time = character->get_packed_time();

    elapsed_logout_parts elapsed{};
    if (!compute_elapsed_logout(packed_time, elapsed)) {
      elapsed = elapsed_logout_parts{};
    }

    const wchar_t* format_str = L"Last logout : %dmonth %dday %dhour %dminute"; // Fallback
    if (auto* mgr = ctext_string_manager::get()) {
      ext_client::msvc9::wstring_ref wstr = mgr->get_text(L"UIIT_STT_LAST_LOGOUT");
      if (!wstr.empty() && wstr.data() && wstr.data()[0] != L'\0') {
        format_str = wstr.data();
      }
    }

    wchar_t buffer[260]{};
    swprintf_s(buffer, 260, format_str, elapsed.months, elapsed.days, elapsed.hours, elapsed.minutes);

    cgwnd* label =
      reinterpret_cast<cgwnd*>(self->get_ui_child(29/*last_logout_label*/, true));
    if (label && label->is_live() && cif_static::is_static(label)) {
      auto* static_label = static_cast<cif_static*>(label);

      static_label->set_text(buffer);
      dock_last_logout_label_left(self, static_label);

      label->set_anim(255, 0.5f, 0.0f, 1);
    }

    play_greeting_on_selected_slot(self);
  }

  // =========================================================================
  // 5. Named Event Handlers Implementation
  // =========================================================================
  auto handle_char_select_enter(char_select_enter_context& ctx) -> void {
    if (!core::plugin::plugin_manager::get().is_plugin_enabled("character_select")) {
      return;
    }
    if (ctx.self && ctx.msg && ctx.msg->msg_id == 0x201) {
      __try {
        update_last_logout_text(ctx.self);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
      }
    }
  }

  auto handle_char_select_slot_change(char_select_slot_change_context& ctx) -> void {
    if (!core::plugin::plugin_manager::get().is_plugin_enabled("character_select")) {
      return;
    }
    if (ctx.self) {
      __try {
        update_last_logout_text(ctx.self);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
      }
    }
  }

  auto handle_char_select_update(char_select_update_context& ctx) -> void {
    if (!core::plugin::plugin_manager::get().is_plugin_enabled("character_select")) {
      return;
    }
    if (g_greeting_delay_frames_left > 0) {
      --g_greeting_delay_frames_left;
      if (g_greeting_delay_frames_left == 0) {
        if (ctx.self && cps_character_select::get_selected_slot_index() == g_greeting_target_slot) {
          auto* entity = selected_deco_character(ctx.self, g_greeting_target_slot);
          if (entity && entity == g_greeting_target_entity) {
            play_greeting_on_character(entity);
          }
        }
        clear_queued_greeting();
      }
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("character_select", "Character Select");
    ADD_EVENT(EVENT_ON_CHAR_SELECT_ENTER, handle_char_select_enter);
    ADD_EVENT(EVENT_ON_CHAR_SELECT_SLOT_CHANGE, handle_char_select_slot_change);
    ADD_EVENT(EVENT_ON_CHAR_SELECT_UPDATE, handle_char_select_update);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::character_select
