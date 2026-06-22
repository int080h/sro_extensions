#pragma once

#include <Windows.h>
#include <array>
#include <cstdio>
#include <cwchar>
#include <vector>
#include <string>

#include "core/core_event_manager.hpp"
#include "sdk/cps_character_select.hpp"
#include "sdk/cif_static.hpp"
#include "sdk/cgame_text.hpp"
#include "sdk/cprocess.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "sdk/cic_deco_character.hpp"

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

  // =========================================================================
  // 3. Global Variables (Extern)
  // =========================================================================
  extern std::array<std::uint32_t, k_max_char_slots> g_cached_logout_times;
  extern std::array<bool, k_max_char_slots> g_cached_logout_valid;

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

  auto read_u8_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::uint8_t& out) -> bool;
  auto read_u16_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::uint16_t& out) -> bool;
  auto read_u32_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::uint32_t& out) -> bool;
  auto skip_bytes(const std::vector<std::uint8_t>& payload, std::size_t& cursor, std::size_t count) -> bool;
  auto skip_string_at(const std::vector<std::uint8_t>& payload, std::size_t& cursor) -> bool;

  auto selected_deco_character(cps_character_select* self, int slot) -> cic_deco_character*;
  auto clear_queued_greeting() -> void;
  auto play_greeting_on_character(cic_deco_character* entity) -> void;
  auto play_greeting_on_selected_slot(cps_character_select* self) -> void;

  auto cache_logout_times_from_b007(cmsg_stream_buffer* packet) -> void;
  auto dock_last_logout_label_left(cps_character_select* self, cif_static* label) -> void;
  auto update_last_logout_text(cps_character_select* self) -> void;

  // =========================================================================
  // 5. Named Event Handlers
  // =========================================================================
  auto handle_char_select_enter(void* raw_ctx) -> void;
  auto handle_char_select_slot_change(void* raw_ctx) -> void;
  auto handle_char_select_msg_recv(void* raw_ctx) -> void;
  auto handle_char_select_update(void* raw_ctx) -> void;

} // namespace ext_client::plugins::character_select
