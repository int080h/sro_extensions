#include "pch.hpp"
#include "plugins/target_window/target_window_plugin.hpp"

#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "render/menu_builder.hpp"
#include "core/core_plugin_manager.hpp"
#include "utils/offsets.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/cif_gauge.hpp"
#include "sdk/ui/cif_target_window.hpp"
#include "utils/log.hpp"

#include <Windows.h>
#include <imgui.h>
#include <cstdio>

using ext_client::render::menu::menu_builder;
using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::target_window {

  // =========================================================================
  // 1. Helper Functions
  // =========================================================================
  auto target_id_active(void* target_id) -> bool;
  auto hp_percent_from_gauge(const cif_gauge* gauge) -> int;
  auto draw_outlined_text(ImDrawList* draw_list, ImVec2 pos, ImU32 color, const char* text) -> void;
  auto render_hp_overlay() -> void;

  // =========================================================================
  // 2. Named Event Handlers
  // =========================================================================
  auto handle_populate_target(populate_target_context& ctx) -> void;
  auto handle_populate_special_mob(populate_special_mob_context& ctx) -> void;
  auto handle_update_special_mob(update_special_mob_context& ctx) -> void;
  auto handle_menu_overlay(menu_draw_context& ctx) -> void;
  auto handle_menu(menu_builder& ui) -> void;

  // =========================================================================
  // 1. Helper Functions Implementation
  // =========================================================================
  auto target_id_active(void* target_id) -> bool {
    return target_id != nullptr && reinterpret_cast<std::uintptr_t>(target_id) != 0;
  }

  auto hp_percent_from_gauge(const cif_gauge* gauge) -> int {
    if (!gauge) {
      return -1;
    }
    return gauge->get_current_percent();
  }

  auto draw_outlined_text(ImDrawList* draw_list, ImVec2 pos, ImU32 color, const char* text) -> void {
    const ImU32 shadow = IM_COL32(0, 0, 0, 220);
    draw_list->AddText(ImVec2(pos.x + 1.f, pos.y + 1.f), shadow, text);
    draw_list->AddText(ImVec2(pos.x - 1.f, pos.y + 1.f), shadow, text);
    draw_list->AddText(ImVec2(pos.x + 1.f, pos.y - 1.f), shadow, text);
    draw_list->AddText(ImVec2(pos.x - 1.f, pos.y - 1.f), shadow, text);
    draw_list->AddText(pos, color, text);
  }

  auto render_hp_overlay() -> void {
    const auto& cfg = ext_client::core::config::data().target_window;
    if (!cfg.enabled || !cfg.show_hp_percent) {
      return;
    }

    void* panel = cif_target_window::active();
    if (!panel || !cif_target_window::is_live_target_panel(panel)) {
      return;
    }

    cif_gauge* gauge = cif_target_window::hp_gauge(panel);
    if (!gauge) {
      return;
    }

    const int percent = hp_percent_from_gauge(gauge);
    if (percent < 0) {
      return;
    }

    char text[16]{};
    snprintf(text, sizeof(text), "%d%%", percent);

    const cgwnd_bounds bar = gauge->get_bounds();
    if (bar.w <= 0 || bar.h <= 0) {
      return;
    }

    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const ImVec2 pos(static_cast<float>(bar.x) + (static_cast<float>(bar.w) - text_size.x) * 0.5f,
                     static_cast<float>(bar.y) + (static_cast<float>(bar.h) - text_size.y) * 0.5f);

    draw_outlined_text(ImGui::GetForegroundDrawList(), pos, IM_COL32(255, 255, 255, 255), text);
  }

  // =========================================================================
  // 2. Named Event Handlers Implementation
  // =========================================================================
  auto handle_populate_target(populate_target_context& ctx) -> void {
    if (!target_id_active(ctx.target_id)) {
      cif_target_window::clear_active_panel();
    }
  }

  auto handle_populate_special_mob(populate_special_mob_context& ctx) -> void {
    if (target_id_active(ctx.target_id)) {
      cif_target_window::note_active_panel(ctx.self);
    } else {
      cif_target_window::clear_active_panel();
    }
  }

  auto handle_update_special_mob(update_special_mob_context& ctx) -> void {
    if (cif_target_window::is_live_target_panel(ctx.self)) {
      cif_target_window::note_active_panel(ctx.self);
    }
  }

  auto handle_menu_overlay(menu_draw_context& /*ctx*/) -> void {
    render_hp_overlay();
  }

  auto handle_menu(menu_builder& ui) -> void {
    ui.section("Target HP Display Settings");

    auto& tw = ext_client::core::config::data().target_window;
    ui.checkbox("Enable HUD HP Overlay", &tw.enabled);
    ui.checkbox("Display HP Percent Texts", &tw.show_hp_percent);
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("target_hp", "Target HUD");

    ADD_EVENT(EVENT_ON_MENU, handle_menu);
    ADD_EVENT(EVENT_ON_POPULATE_TARGET, handle_populate_target);
    ADD_EVENT(EVENT_ON_POPULATE_SPECIAL_MOB, handle_populate_special_mob);
    ADD_EVENT(EVENT_ON_UPDATE_SPECIAL_MOB, handle_update_special_mob);
    ADD_EVENT(EVENT_ON_MENU_OVERLAY, handle_menu_overlay);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::target_window
