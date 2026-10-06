#include "pch.hpp"
#include "plugins/character_select/character_select_plugin.hpp"

#include "core/event_bus.hpp"
#include "core/plugin_manager.hpp"
#include "sdk/game/ctext_string_manager.hpp"
#include "sdk/game/cic_deco_character.hpp"
#include "sdk/net/packet_builder.hpp"
#include "sdk/process/cprocess.hpp"
#include "sdk/process/cps_character_select.hpp"
#include "sdk/types/packed_time.hpp"
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

  extern int g_greeting_delay_frames_left;
  extern int g_greeting_target_slot;

  // =========================================================================
  // 2. Helper Function Declarations
  // =========================================================================
  auto selected_deco_character(cps_character_select* self, int slot) -> cic_deco_character*;
  auto clear_queued_greeting() -> void;
  auto play_greeting_on_character(cic_deco_character* entity) -> void;
  auto play_greeting_on_selected_slot(cps_character_select* self) -> void;

  auto dock_last_logout_label_left(cps_character_select* self, cif_static* label) -> void;
  auto update_last_logout_text(cps_character_select* self) -> void;

  // =========================================================================
  // 3. Named Event Handlers
  // =========================================================================
  auto handle_char_select_enter(char_select_enter_context& ctx) -> void;
  auto handle_char_select_slot_change(char_select_slot_change_context& ctx) -> void;
  auto handle_char_select_update(char_select_update_context& ctx) -> void;

  int g_greeting_delay_frames_left = -1;
  int g_greeting_target_slot = -1;

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

    if (auto* start_button = self->get_ui_child(0x0E/*select_button (14)*/, true)) {
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

    const std::uint32_t raw_time = character->get_packed_time();

    sdk::elapsed_logout_parts elapsed{};
    if (!sdk::compute_elapsed_logout(sdk::packed_time(raw_time), elapsed)) {
      elapsed = sdk::elapsed_logout_parts{};
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

    cgwnd* label = self->get_ui_child(29/*last_logout_label*/, true);
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
      update_last_logout_text(ctx.self);
    }
  }

  auto handle_char_select_slot_change(char_select_slot_change_context& ctx) -> void {
    if (!core::plugin::plugin_manager::get().is_plugin_enabled("character_select")) {
      return;
    }
    if (ctx.self) {
      update_last_logout_text(ctx.self);
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
          if (entity) {
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
