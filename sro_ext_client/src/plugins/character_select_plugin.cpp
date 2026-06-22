#include "plugins/character_select_plugin.hpp"
#include "core/core_plugin_manager.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::character_select {

  // =========================================================================
  // 1. Global Variables Definition
  // =========================================================================
  std::array<std::uint32_t, k_max_char_slots> g_cached_logout_times{};
  std::array<bool, k_max_char_slots> g_cached_logout_valid{};

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

  // --- Packet Buffer Parsing Helpers ---
  auto read_u8_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::uint8_t& out) -> bool {
    if (cursor + 1 > payload.size()) {
      return false;
    }
    out = payload[cursor++];
    return true;
  }

  auto read_u16_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::uint16_t& out) -> bool {
    if (cursor + 2 > payload.size()) {
      return false;
    }
    out = static_cast<std::uint16_t>(payload[cursor] | (payload[cursor + 1] << 8));
    cursor += 2;
    return true;
  }

  auto read_u32_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::uint32_t& out) -> bool {
    if (cursor + 4 > payload.size()) {
      return false;
    }
    out = static_cast<std::uint32_t>(payload[cursor]) | (static_cast<std::uint32_t>(payload[cursor + 1]) << 8) |
          (static_cast<std::uint32_t>(payload[cursor + 2]) << 16) | (static_cast<std::uint32_t>(payload[cursor + 3]) << 24);
    cursor += 4;
    return true;
  }

  auto skip_bytes(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::size_t count) -> bool {
    if (cursor + count > payload.size()) {
      return false;
    }
    cursor += count;
    return true;
  }

  auto skip_string_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor) -> bool {
    std::uint16_t length = 0;
    return read_u16_at(payload, cursor, length) && skip_bytes(payload, cursor, length);
  }

  // =========================================================================
  // 3. Character Select & Greeting Animation Logic
  // =========================================================================
  auto selected_deco_character(cps_character_select* self, int slot) -> cic_deco_character* {
    if (!self || slot == 255 || slot < 0) {
      return nullptr;
    }

    pcinfo_ui* character = self->character_at(slot);
    if (!character) {
      return nullptr;
    }

    auto* info = static_cast<cps_character_select::s_character_info*>(character);
    return cic_deco_character::from_ptr(static_cast<cic_deco_character*>(info));
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
    int slot = cps_character_select::selected_slot_index();
    if (slot == 255 || slot == -1)
      return;

    auto* entity = selected_deco_character(self, slot);
    if (entity) {
      g_greeting_target_entity = entity;
      g_greeting_target_slot = slot;
      g_greeting_delay_frames_left = k_greeting_delay_frames;
    }
  }

  // =========================================================================
  // 4. Last Logout Rendering Logic
  // =========================================================================
  auto cache_logout_times_from_b007(cmsg_stream_buffer* packet) -> void {
    if (!packet || packet->opcode() != ext_client::offsets::cps_character_select::packets::char_list_notify) {
      return;
    }

    const auto payload = packet->extract_payload();
    if (payload.size() < 3 || payload[0] != ext_client::offsets::cps_character_select::char_list_notify::refresh_list ||
        payload[1] != 1) {
      return;
    }

    g_cached_logout_valid.fill(false);

    std::size_t cursor = 2;
    std::uint8_t char_count = 0;
    if (!read_u8_at(payload, cursor, char_count)) {
      return;
    }

    for (std::size_t slot = 0; slot < char_count && slot < k_max_char_slots; ++slot) {
      std::uint32_t ignored_u32 = 0;
      std::uint16_t ignored_u16 = 0;
      std::uint8_t ignored_u8 = 0;
      std::uint32_t packed_time = 0;

      if (!read_u32_at(payload, cursor, ignored_u32) || !skip_string_at(payload, cursor) || !skip_string_at(payload, cursor) ||
          !read_u8_at(payload, cursor, ignored_u8) || !read_u8_at(payload, cursor, ignored_u8) || !skip_bytes(payload, cursor, 8) ||
          !read_u16_at(payload, cursor, ignored_u16) || !read_u16_at(payload, cursor, ignored_u16) ||
          !read_u16_at(payload, cursor, ignored_u16) || !read_u32_at(payload, cursor, ignored_u32) ||
          !read_u32_at(payload, cursor, ignored_u32) || !read_u32_at(payload, cursor, ignored_u32) ||
          !read_u16_at(payload, cursor, ignored_u16) || !read_u8_at(payload, cursor, ignored_u8) ||
          !read_u32_at(payload, cursor, packed_time)) {
        return;
      }

      if (ignored_u8 != 0 && !read_u32_at(payload, cursor, ignored_u32)) {
        return;
      }

      if (!read_u8_at(payload, cursor, ignored_u8)) {
        return;
      }
      std::uint8_t has_extra_string = 0;
      if (!read_u8_at(payload, cursor, has_extra_string)) {
        return;
      }
      if (has_extra_string != 0 && !skip_string_at(payload, cursor)) {
        return;
      }

      if (!read_u8_at(payload, cursor, ignored_u8)) {
        return;
      }

      std::uint8_t item_count = 0;
      if (!read_u8_at(payload, cursor, item_count)) {
        return;
      }
      for (std::size_t i = 0; i < item_count; ++i) {
        if (!read_u32_at(payload, cursor, ignored_u32) || !read_u8_at(payload, cursor, ignored_u8)) {
          return;
        }
      }

      std::uint8_t avatar_count = 0;
      if (!read_u8_at(payload, cursor, avatar_count)) {
        return;
      }
      for (std::size_t i = 0; i < avatar_count; ++i) {
        if (!read_u32_at(payload, cursor, ignored_u32) || !read_u8_at(payload, cursor, ignored_u8)) {
          return;
        }
      }

      g_cached_logout_times[slot] = packed_time;
      g_cached_logout_valid[slot] = true;
      log_msg("[character_select_plugin] cached slot %u packed_time=0x%08X from B007", static_cast<unsigned>(slot), packed_time);
    }
  }

  auto dock_last_logout_label_left(cps_character_select* self, cif_static* label) -> void {
    if (!self || !label) {
      return;
    }

    int target_x = k_last_logout_left_margin;
    int target_w = label->rect_w();

    if (auto* start_button =
          reinterpret_cast<cgwnd*>(self->get_ui_child(ext_client::offsets::cps_character_select::ui_ids::select_button, true))) {
      const int right_limit = start_button->rect_x() - k_last_logout_button_gap;
      if (right_limit > target_x) {
        target_w = right_limit - target_x;
      }
    }

    if (target_w < k_last_logout_fallback_min_width) {
      target_w = k_last_logout_fallback_min_width;
    }

    label->set_position(target_x, label->rect_y());
    label->set_size(target_w, label->rect_h());
    label->set_align_h(k_cif_align_left);
    label->refresh_layout();
  }

  auto update_last_logout_text(cps_character_select* self) -> void {
    if (!self)
      return;

    int slot = cps_character_select::selected_slot_index();
    if (slot == 255 || slot == -1)
      return;

    pcinfo_ui* character = self->character_at(slot);
    if (!character)
      return;

    std::uint32_t packed_time = 0;
    if (slot >= 0 && static_cast<std::size_t>(slot) < k_max_char_slots && g_cached_logout_valid[slot]) {
      packed_time = g_cached_logout_times[slot];
    } else {
      packed_time = character->packed_time;
    }

    elapsed_logout_parts elapsed{};
    if (!compute_elapsed_logout(packed_time, elapsed)) {
      elapsed = elapsed_logout_parts{};
    }

    const wchar_t* format_str = L"Last logout : %dmonth %dday %dhour %dminute"; // Fallback
    ext_client::msvc9::wstring_ref wstr = cgame_text::get()->get_text(L"UIIT_STT_LAST_LOGOUT");
    if (!wstr.empty()) {
      format_str = wstr.data();
    }

    wchar_t buffer[260]{};
    swprintf_s(buffer, 260, format_str, elapsed.months, elapsed.days, elapsed.hours, elapsed.minutes);

    cgwnd* label =
      reinterpret_cast<cgwnd*>(self->get_ui_child(ext_client::offsets::cps_character_select::ui_ids::last_logout_label, true));
    if (label) {
      auto* static_label = cif_static::from(label);
      if (!static_label) {
        return;
      }

      static_label->set_text(buffer);
      dock_last_logout_label_left(self, static_label);

      label->set_anim(255, 0.5f, 0.0f, 1);
    }

    play_greeting_on_selected_slot(self);
  }

  // =========================================================================
  // 5. Named Event Handlers Implementation
  // =========================================================================
  auto handle_char_select_enter(void* raw_ctx) -> void {
    auto* ctx = static_cast<char_select_enter_context*>(raw_ctx);
    if (ctx && ctx->msg) {
      if (ctx->msg->msg_id == ext_client::offsets::cps_character_select::wm_user::last_logout) {
        update_last_logout_text(ctx->self);
      }
    }
  }

  auto handle_char_select_slot_change(void* raw_ctx) -> void {
    auto* ctx = static_cast<char_select_slot_change_context*>(raw_ctx);
    if (ctx && ctx->self) {
      update_last_logout_text(ctx->self);
    }
  }

  auto handle_char_select_msg_recv(void* raw_ctx) -> void {
    auto* ctx = static_cast<char_select_msg_recv_context*>(raw_ctx);
    if (ctx && ctx->packet) {
      cache_logout_times_from_b007(ctx->packet);
    }
  }

  auto handle_char_select_update(void* raw_ctx) -> void {
    auto* ctx = static_cast<char_select_update_context*>(raw_ctx);
    if (ctx && ctx->self) {
      if (g_greeting_target_entity) {
        if (g_greeting_delay_frames_left > 0) {
          --g_greeting_delay_frames_left;
          if (g_greeting_delay_frames_left == 0) {
            auto* current_entity = selected_deco_character(ctx->self, cps_character_select::selected_slot_index());
            if (current_entity == g_greeting_target_entity && g_greeting_target_slot == cps_character_select::selected_slot_index()) {
              play_greeting_on_character(g_greeting_target_entity);
            }
            clear_queued_greeting();
          }
        }
      }
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("character_select", "Character Select Customizer", "Customizes character select screen, slot changes, and greeting animations.");

    ADD_PLUGIN_EVENT("character_select", event_type::on_char_select_enter, handle_char_select_enter);
    ADD_PLUGIN_EVENT("character_select", event_type::on_char_select_slot_change, handle_char_select_slot_change);
    ADD_PLUGIN_EVENT("character_select", event_type::on_char_select_msg_recv, handle_char_select_msg_recv);
    ADD_PLUGIN_EVENT("character_select", event_type::on_char_select_update, handle_char_select_update);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::character_select
