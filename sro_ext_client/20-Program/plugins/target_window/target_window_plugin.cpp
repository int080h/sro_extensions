#include "pch.hpp"
#include "plugins/target_window/target_window_plugin.hpp"

#include "core/config.hpp"
#include "core/event_bus.hpp"
#include "core/plugin_manager.hpp"
#include "render/menu_builder.hpp"
#include "sdk/game/ccompound_obj.hpp"
#include "sdk/game/ccontroler.hpp"
#include "sdk/game/centity_manager.hpp"
#include "sdk/game/ci_charactor.hpp"
#include "sdk/game/cic_user.hpp"
#include "sdk/game/cic_player.hpp"
#include "sdk/game/cso_item.hpp"
#include "sdk/game/c_skill_manager.hpp"
#include "sdk/game/cref_skill.hpp"
#include "sdk/ui/cif_slot_with_help.hpp"
#include "render/item_tooltip_renderer.hpp"
#include "render/item_icon_renderer.hpp"
#include "sdk/ui/cui_string_manager.hpp"
#include "sdk/net/cmsg.hpp"
#include "sdk/net/cmsg_stream_buffer.hpp"
#include "sdk/net/msg_define.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/cif_gauge.hpp"
#include "sdk/ui/cif_static.hpp"
#include "sdk/ui/cif_target_window.hpp"
#include "utils/log.hpp"
#include "utils/memory.hpp"
#include "utils/offsets.hpp"
#include "utils/string.hpp"

#include <Windows.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>

using ext_client::render::menu::menu_builder;
using ext_client::utils::log_msg;
using ext_client::utils::memory::is_game_ptr;
using ext_client::utils::memory::is_readable_ptr;
using namespace ext_client::core::event;

namespace ext_client::plugins::target_window {

  // =========================================================================
  // Target of Target (ToT) Tracking Data
  // =========================================================================
  struct combat_target_entry {
    std::uint32_t target_uid = 0;
    std::uint64_t timestamp_ms = 0;
  };

  namespace {
    std::mutex g_tot_mutex;
    std::unordered_map<std::uint32_t, combat_target_entry> g_entity_targets;
    bool s_inspect_window_open = false;
    std::uint32_t s_inspect_target_uid = 0;

    template <typename T>
    auto format_thousands(T val) -> std::string {
      std::string s;
      if constexpr (std::is_signed_v<T>) {
        if (val < 0) {
          return "-" + format_thousands(static_cast<std::make_unsigned_t<T>>(-val));
        }
        s = std::to_string(val);
      } else {
        s = std::to_string(val);
      }
      int n = static_cast<int>(s.length()) - 3;
      while (n > 0) {
        s.insert(static_cast<std::size_t>(n), ",");
        n -= 3;
      }
      return s;
    }

    auto now_ms() -> std::uint64_t {
      return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch()
        ).count()
      );
    }

    auto is_in_world() -> bool {
      auto* local_player = cic_user::get_local_player();
      if (!local_player || !is_game_ptr(local_player)) {
        return false;
      }
      const char* proc_name = ccontroler::active_child_process_name();
      if (proc_name) {
        if (std::strcmp(proc_name, "CPSTitle") == 0 ||
            std::strcmp(proc_name, "CPSCharacterSelect") == 0 ||
            std::strcmp(proc_name, "CPSCharacterCreateEurope") == 0 ||
            std::strcmp(proc_name, "CPSCharacterCreateChina") == 0 ||
            std::strcmp(proc_name, "CPSLogo") == 0 ||
            std::strcmp(proc_name, "CPSVersionCheck") == 0 ||
            std::strcmp(proc_name, "CPSRestart") == 0 ||
            std::strcmp(proc_name, "CPSQuit") == 0) {
          return false;
        }
      }
      return true;
    }

    auto get_packet_payload(const packet_context& ctx) -> std::vector<std::uint8_t> {
      if (ctx.layer == packet_layer::stream && ctx.stream_msg) {
        return ctx.stream_msg->extract_payload();
      }
      if (ctx.layer == packet_layer::cmsg && ctx.cmsg_msg) {
        return ctx.cmsg_msg->extract_payload();
      }
      return {};
    }

    auto position_is_usable(const vector3f& pos) -> bool {
      return std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z) &&
             (pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f);
    }

    auto get_entity_world_pos(ci_charactor* ent, vector3f& out_pos) -> bool {
      if (!ent || !is_readable_ptr(ent)) {
        return false;
      }

      // 1. Direct render position at CIObject+0x90 (continuous world coordinates)
      const auto* pos = &ext_client::off::field_at<float>(ent, 0x90);
      if (is_readable_ptr(pos) && is_readable_ptr(pos + 2)) {
        out_pos.x = pos[0];
        out_pos.y = pos[1];
        out_pos.z = pos[2];
        if (position_is_usable(out_pos)) {
          return true;
        }
      }

      // 2. Compound object world transform matrix (_41, _42, _43)
      auto* cobj = ent->get_compound_obj();
      if (cobj && is_game_ptr(cobj)) {
        const auto* mat = cobj->world_matrix();
        if (mat && is_game_ptr(mat)) {
          out_pos.x = mat->_41;
          out_pos.y = mat->_42;
          out_pos.z = mat->_43;
          if (position_is_usable(out_pos)) {
            return true;
          }
        }
      }

      // 3. Fallback: Regional position at CICharactor+0x7C
      const auto* spos = ent->get_position();
      if (spos && is_readable_ptr(spos)) {
        const int rx = static_cast<int>(spos->region_x());
        const int ry = static_cast<int>(spos->region_y());
        if (rx > 0 && ry > 0) {
          out_pos.x = (static_cast<float>(rx) - 128.0f) * 1920.0f + spos->x;
          out_pos.y = spos->z;
          out_pos.z = (static_cast<float>(ry) - 128.0f) * 1920.0f + spos->y;
          if (position_is_usable(out_pos)) {
            return true;
          }
        }
      }

      return false;
    }
  } // namespace

  // =========================================================================
  // 1. Helper Functions
  // =========================================================================
  auto hp_percent_from_gauge(const cif_gauge* gauge) -> int {
    if (!gauge) {
      return -1;
    }
    return gauge->get_current_percent();
  }

  auto draw_outlined_text(ImDrawList* draw_list, ImVec2 pos, ImU32 color, const char* text) -> void {
    if (!draw_list || !text) {
      return;
    }
    const ImU32 shadow = IM_COL32(0, 0, 0, 220);
    draw_list->AddText(ImVec2(pos.x + 1.f, pos.y + 1.f), shadow, text);
    draw_list->AddText(ImVec2(pos.x - 1.f, pos.y + 1.f), shadow, text);
    draw_list->AddText(ImVec2(pos.x + 1.f, pos.y - 1.f), shadow, text);
    draw_list->AddText(ImVec2(pos.x - 1.f, pos.y - 1.f), shadow, text);
    draw_list->AddText(pos, color, text);
  }

  // =========================================================================
  // 2. Target HUD Overlay & Enhancements (Feature 8)
  // =========================================================================
  auto render_target_overlay() -> void {
    const auto& cfg = ext_client::core::config::data().target_window;
    if (!cfg.enabled) {
      return;
    }

    if (!is_in_world()) {
      return;
    }

    cif_target_window* panel = cif_target_window::active();
    if (!panel) {
      return;
    }

    cif_gauge* gauge = panel->hp_gauge();
    if (!gauge) {
      return;
    }

    const cgwnd_bounds bar = gauge->get_bounds();
    if (bar.w <= 0 || bar.h <= 0) {
      return;
    }

    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    if (!draw_list) {
      return;
    }

    // -----------------------------------------------------------------------
    // A. Native HP Percentage Overlay inside HP bar
    // -----------------------------------------------------------------------
    if (cfg.show_hp_percent) {
      const int percent = hp_percent_from_gauge(gauge);
      if (percent >= 0) {
        char text[16]{};
        std::snprintf(text, sizeof(text), "%d%%", percent);
        const ImVec2 text_size = ImGui::CalcTextSize(text);
        const ImVec2 pos(static_cast<float>(bar.x) + (static_cast<float>(bar.w) - text_size.x) * 0.5f,
                         static_cast<float>(bar.y) + (static_cast<float>(bar.h) - text_size.y) * 0.5f);
        draw_outlined_text(draw_list, pos, IM_COL32(255, 255, 255, 255), text);
      }
    }

    // Resolve target entity
    ci_charactor* target_ent = nullptr;
    const std::uint32_t slot_id = panel ? panel->target_slot_id() : 0;
    if (slot_id > 0) {
      target_ent = centity_manager::resolve_by_uid_or_slot(slot_id);
    }
    if (!target_ent) {
      if (auto* iface = ::cg_interface::get()) {
        target_ent = iface->target_entity();
      }
    }
    if (!target_ent || !ext_client::utils::memory::is_game_ptr(target_ent)) {
      return;
    }

    auto* local_player = cic_user::get_local_player();
    const std::uint32_t local_player_uid = local_player ? local_player->get_unique_id() : 0;
    const std::uint64_t current_time_ms = now_ms();

    // -----------------------------------------------------------------------
    // B. Exact 3D Distance in Meters
    // -----------------------------------------------------------------------
    float dist_meters = -1.0f;
    if (local_player) {
      vector3f p_pos{}, t_pos{};
      if (get_entity_world_pos(local_player, p_pos) && get_entity_world_pos(target_ent, t_pos)) {
        const float dx = t_pos.x - p_pos.x;
        const float dy = t_pos.y - p_pos.y;
        const float dz = t_pos.z - p_pos.z;
        dist_meters = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.1f;
      }
    }

    if (cfg.show_distance && dist_meters >= 0.0f) {
      char dist_str[32]{};
      std::snprintf(dist_str, sizeof(dist_str), "%.1fm", dist_meters);

      // Color code by combat range
      ImU32 dist_col = IM_COL32(100, 255, 120, 255); // Green (Melee / close)
      if (dist_meters > 30.0f) {
        dist_col = IM_COL32(255, 70, 70, 255);       // Red (Out of skill range)
      } else if (dist_meters > 18.0f) {
        dist_col = IM_COL32(255, 160, 40, 255);      // Orange (Long range)
      } else if (dist_meters > 6.0f) {
        dist_col = IM_COL32(255, 230, 60, 255);      // Yellow (Mid range)
      }

      // Draw distance tag right to the right of the HP gauge
      const ImVec2 dist_pos(static_cast<float>(bar.x + bar.w) + 6.0f,
                            static_cast<float>(bar.y) + (static_cast<float>(bar.h) - ImGui::GetFontSize()) * 0.5f);
      draw_outlined_text(draw_list, dist_pos, dist_col, dist_str);
    }

    // -----------------------------------------------------------------------
    // C. Inspect Trigger Button (Beside Target Frame)
    // -----------------------------------------------------------------------
    if (target_ent->is_player()) {
      const ImVec2 btn_pos(static_cast<float>(bar.x + bar.w) + 52.0f, static_cast<float>(bar.y) - 2.0f);
      ImGui::SetNextWindowPos(btn_pos);
      ImGui::SetNextWindowBgAlpha(0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      if (ImGui::Begin("##TargetInspectAnchor", nullptr,
                       ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.16f, 0.24f, 0.88f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.35f, 0.58f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.48f, 0.75f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.35f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 2.0f));

        if (ImGui::Button("Inspect")) {
          s_inspect_target_uid = target_ent->get_unique_id();
          s_inspect_window_open = !s_inspect_window_open;
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
      }
      ImGui::End();
      ImGui::PopStyleVar(2);
    }

    // -----------------------------------------------------------------------
    // D. Contextual Target-of-Target (ToT) Badge
    // -----------------------------------------------------------------------
    if (cfg.show_tot || cfg.show_target_details) {
      const std::uint32_t target_uid = target_ent->get_unique_id();
      std::uint32_t tot_uid = 0;
      bool tot_active = false;

      {
        std::lock_guard lock(g_tot_mutex);
        auto it = g_entity_targets.find(target_uid);
        if (it != g_entity_targets.end()) {
          if ((current_time_ms - it->second.timestamp_ms) <= 12000) {
            tot_uid = it->second.target_uid;
            tot_active = true;
          }
        }
      }

      std::string badge_text;
      ImU32 badge_color = IM_COL32(200, 220, 255, 240);

      // Combat Aggro / ToT indicator
      if (cfg.show_tot && tot_active && tot_uid != 0) {
        if (local_player_uid != 0 && tot_uid == local_player_uid) {
          badge_text = "[Attacking You]";
          const float pulse = 0.6f + 0.4f * std::sin(static_cast<float>(current_time_ms) * 0.008f);
          badge_color = IM_COL32(255, static_cast<int>(50 * pulse), static_cast<int>(50 * pulse), 255);
        } else {
          auto* tot_ent = centity_manager::find_entity_by_uid(tot_uid);
          if (tot_ent && is_readable_ptr(tot_ent)) {
            const auto* raw_tot_name = tot_ent->get_display_name();
            badge_text = ">> " + (raw_tot_name ? ext_client::utils::string::to_utf8(raw_tot_name) : "Target");
            badge_color = IM_COL32(255, 205, 70, 255);
          }
        }
      }

      // Extra archetype details
      if (cfg.show_target_details) {
        std::string class_str;
        if (target_ent->is_pet()) {
          const auto arch = target_ent->pet_archetype_name_loc();
          const std::uint32_t level = target_ent->get_level();
          char pet_buf[96]{};
          std::snprintf(pet_buf, sizeof(pet_buf), "[%s - Lv.%u]", !arch.empty() ? arch.c_str() : "Pet", level);
          class_str = pet_buf;
        } else if (target_ent->is_monster()) {
          if (target_ent->get_rarity() > 0 || target_ent->is_party_mob()) {
            const auto r_name = target_ent->rarity_name_loc();
            char mob_buf[96]{};
            std::snprintf(mob_buf, sizeof(mob_buf), "[%s]", !r_name.empty() ? r_name.c_str() : "Special");
            class_str = mob_buf;
          }
        }

        if (!class_str.empty()) {
          if (!badge_text.empty()) {
            badge_text += "  " + class_str;
          } else {
            badge_text = class_str;
            badge_color = IM_COL32(180, 210, 245, 220);
          }
        }
      }

      // Render pill only when meaningful badge is present
      if (!badge_text.empty()) {
        const ImVec2 text_size = ImGui::CalcTextSize(badge_text.c_str());
        constexpr float pad_x = 8.0f;
        constexpr float pad_y = 3.0f;

        const float panel_x = static_cast<float>(bar.x);
        const float panel_y = static_cast<float>(bar.y + bar.h) + 4.0f;
        const float panel_w = text_size.x + (pad_x * 2.0f);
        const float panel_h = text_size.y + (pad_y * 2.0f);

        const ImVec2 p_min(panel_x, panel_y);
        const ImVec2 p_max(panel_x + panel_w, panel_y + panel_h);

        draw_list->AddRectFilled(p_min, p_max, IM_COL32(14, 18, 26, 220), 4.0f);
        draw_list->AddRect(p_min, p_max, IM_COL32(65, 85, 115, 200), 4.0f, 0, 1.0f);

        const ImVec2 text_pos(panel_x + pad_x, panel_y + pad_y);
        draw_outlined_text(draw_list, text_pos, badge_color, badge_text.c_str());
      }
    }
  }

  // =========================================================================
  // 3. Named Event Handlers Implementation
  // =========================================================================
  auto handle_set_child_process(set_child_process_context& /*ctx*/) -> void {
    s_inspect_window_open = false;
    s_inspect_target_uid = 0;
    std::lock_guard lock(g_tot_mutex);
    g_entity_targets.clear();
  }

  auto handle_char_select_enter(char_select_enter_context& /*ctx*/) -> void {
    s_inspect_window_open = false;
    s_inspect_target_uid = 0;
    std::lock_guard lock(g_tot_mutex);
    g_entity_targets.clear();
  }

  auto handle_packet(packet_context& ctx) -> void {
    // 1. Packet 0x3068 (SERVER_ENTITY_DAMAGE) - Track who attacks whom
    if (ctx.opcode == ext_client::net::msg::SERVER_ENTITY_DAMAGE) {
      const auto payload = get_packet_payload(ctx);
      if (payload.size() >= 13) {
        std::uint32_t attacker_uid = 0;
        std::uint32_t defender_uid = 0;
        std::memcpy(&attacker_uid, &payload[0], sizeof(attacker_uid));
        std::memcpy(&defender_uid, &payload[4], sizeof(defender_uid));

        if (attacker_uid != 0 && defender_uid != 0) {
          std::lock_guard lock(g_tot_mutex);
          g_entity_targets[attacker_uid] = {defender_uid, now_ms()};
        }
      }
      return;
    }

    // 2. Packet 0x7045 / 0xB045 (Target Selection)
    if (ctx.opcode == 0x7045 || ctx.opcode == 0xB045) {
      const auto payload = get_packet_payload(ctx);
      if (payload.size() >= 4) {
        std::uint32_t target_uid = 0;
        std::memcpy(&target_uid, &payload[0], sizeof(target_uid));
        auto* local_player = cic_user::get_local_player();
        if (local_player && local_player->get_unique_id() != 0) {
          std::lock_guard lock(g_tot_mutex);
          g_entity_targets[local_player->get_unique_id()] = {target_uid, now_ms()};
        }
      }
    }
  }

  // =========================================================================
  // 4. Equipment Inspector Window with Live Hover Tooltips
  // =========================================================================
  auto render_equipment_inspector() -> void {
    if (!is_in_world()) {
      s_inspect_window_open = false;
      s_inspect_target_uid = 0;
      return;
    }

    auto& cfg = ext_client::core::config::data().target_window;
    if (!s_inspect_window_open && !cfg.show_equipment_inspector) {
      return;
    }

    auto* local_player = cic_player::local();

    // Resolve target entity: prioritize s_inspect_target_uid, fallback to native target entity or active target panel
    ci_charactor* target_ent = nullptr;
    if (s_inspect_target_uid != 0) {
      target_ent = centity_manager::find_entity_by_uid(s_inspect_target_uid);
    }
    if (!target_ent) {
      auto* target_panel = cif_target_window::active();
      const std::uint32_t target_slot = target_panel ? target_panel->target_slot_id() : 0;
      if (target_slot > 0) {
        target_ent = centity_manager::resolve_by_uid_or_slot(target_slot);
      }
      if (!target_ent) {
        if (auto* iface = ::cg_interface::get()) {
          target_ent = iface->target_entity();
        }
      }
    }

    // Auto-close check if inspector was opened for a target
    if (s_inspect_window_open) {
      bool should_close = false;
      if (!target_ent || !target_ent->is_player()) {
        should_close = true;
      } else if (local_player) {
        vector3f p_pos{}, t_pos{};
        if (get_entity_world_pos(local_player, p_pos) && get_entity_world_pos(target_ent, t_pos)) {
          const float dx = t_pos.x - p_pos.x;
          const float dy = t_pos.y - p_pos.y;
          const float dz = t_pos.z - p_pos.z;
          const float dist_meters = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.1f;
          if (dist_meters > 15.0f) {
            should_close = true;
          }
        }
      }

      if (should_close) {
        s_inspect_window_open = false;
        s_inspect_target_uid = 0;
        if (!cfg.show_equipment_inspector) {
          return;
        }
      }
    }

    if (!s_inspect_window_open && !cfg.show_equipment_inspector) {
      return;
    }

    cic_user* target_player = (target_ent && target_ent->is_player()) ? static_cast<cic_user*>(target_ent) : nullptr;

    ImGui::SetNextWindowSize(ImVec2(620.0f, 660.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(520.0f, 480.0f), ImVec2(1200.0f, 1000.0f));

    std::string win_title = "Character Inspector";
    if (s_inspect_window_open && target_player && target_player != local_player) {
      const wchar_t* raw_tname = target_player->user_name();
      const auto t_name = (raw_tname && ext_client::utils::memory::is_readable_ptr(raw_tname))
                            ? ext_client::utils::string::to_utf8(raw_tname)
                            : std::string{};
      win_title = "Character Inspector - " + (t_name.empty() ? "Target Player" : t_name) + " (Lv." + std::to_string(target_player->get_level()) + ")";
    } else if (local_player) {
      const auto p_name = ext_client::utils::string::to_utf8(local_player->name());
      win_title = "Character Inspector - " + (p_name.empty() ? "My Character" : p_name) + " (Lv." + std::to_string(local_player->level()) + ")";
    }

    // Silkroad dark slate palette
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.08f, 0.12f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.12f, 0.16f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.18f, 0.25f, 0.38f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.48f, 0.65f, 0.45f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    bool is_open = true;
    const bool window_drawn = ImGui::Begin(win_title.c_str(), &is_open, ImGuiWindowFlags_None);
    if (!is_open) {
      s_inspect_window_open = false;
      cfg.show_equipment_inspector = false;
      s_inspect_target_uid = 0;
    }

    if (window_drawn) {
      if (!local_player) {
        ImGui::TextDisabled("No character data available. Enter the game world first.");
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
        return;
      }

      static int current_main_tab = 0;
      static int s_request_tab = -1;
      static std::uint32_t s_last_inspected_uid = 0;
      if (s_inspect_window_open && s_inspect_target_uid != 0 && s_inspect_target_uid != s_last_inspected_uid) {
        s_last_inspected_uid = s_inspect_target_uid;
        s_request_tab = 4; // Target Player tab
      }

      if (s_request_tab == 4 && (!target_player || target_player == local_player)) {
        s_request_tab = 0;
      }

      // ---------------------------------------------------------------------
      // Top Identity Banner
      // ---------------------------------------------------------------------
      const auto p_name = ext_client::utils::string::to_utf8(local_player->name());
      const auto g_name = ext_client::utils::string::to_utf8(local_player->guild_name());
      const auto player_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_PARTYMATCH_PSEARCH_OBJECTCOMBAT", "Player");

      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "%s", p_name.empty() ? player_lbl.c_str() : p_name.c_str());
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Lv.%u", local_player->level());

      if (!g_name.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.40f, 0.82f, 1.0f, 1.0f), "<%s>", g_name.c_str());
      }

      if (local_player->is_in_job_mode()) {
        const auto j_name = local_player->job_name_loc();
        if (!j_name.empty()) {
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.40f, 1.0f), "[%s Lv.%u]", j_name.c_str(), local_player->job_level());
        }
      }

      if (local_player->is_in_pvp()) {
        const auto cape_name = local_player->pvp_cape_name_loc();
        if (!cape_name.empty()) {
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.40f, 1.0f), "[%s]", cape_name.c_str());
        }
      }

      if (target_player && target_player != local_player) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 180.0f);
        const auto t_name = ext_client::utils::string::to_utf8(target_player->user_name());
        std::string switch_label = current_main_tab == 4 ? "View Self" : ("Inspect " + (t_name.empty() ? "Target" : t_name));
        if (ImGui::Button(switch_label.c_str())) {
          s_request_tab = current_main_tab == 4 ? 0 : 4;
        }
      }

      ImGui::Separator();

      // ---------------------------------------------------------------------
      // Navigation Tabs
      // ---------------------------------------------------------------------
      if (ImGui::BeginTabBar("MainInspectorTabs", ImGuiTabBarFlags_None)) {
        // ---------------------------------------------------------------------
        // TAB 0: Overview & Stats
        // ---------------------------------------------------------------------
        const ImGuiTabItemFlags tab0_flags = (s_request_tab == 0) ? ImGuiTabItemFlags_SetSelected : 0;
        if (ImGui::BeginTabItem("Overview & Stats", nullptr, tab0_flags)) {
          current_main_tab = 0;
          const auto stats = local_player->get_combat_stats();

          // 1. Vitals & Resources
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Vitals & Resources");
          ImGui::Spacing();

          // HP Bar
          const float hp_ratio = stats.max_hp > 0 ? static_cast<float>(stats.cur_hp) / static_cast<float>(stats.max_hp) : 0.0f;
          char hp_buf[64];
          std::snprintf(hp_buf, sizeof(hp_buf), "HP: %s / %s (%.1f%%)",
                        format_thousands(stats.cur_hp).c_str(),
                        format_thousands(stats.max_hp).c_str(),
                        hp_ratio * 100.0f);
          ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.85f, 0.18f, 0.22f, 1.0f));
          ImGui::ProgressBar(hp_ratio, ImVec2(-1.0f, 18.0f), hp_buf);
          ImGui::PopStyleColor();

          // MP Bar
          const float mp_ratio = stats.max_mp > 0 ? static_cast<float>(stats.cur_mp) / static_cast<float>(stats.max_mp) : 0.0f;
          char mp_buf[64];
          std::snprintf(mp_buf, sizeof(mp_buf), "MP: %s / %s (%.1f%%)",
                        format_thousands(stats.cur_mp).c_str(),
                        format_thousands(stats.max_mp).c_str(),
                        mp_ratio * 100.0f);
          ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.18f, 0.52f, 0.92f, 1.0f));
          ImGui::ProgressBar(mp_ratio, ImVec2(-1.0f, 18.0f), mp_buf);
          ImGui::PopStyleColor();

          // EXP Bar (Matches Silkroad Native Character Window)
          const float exp_ratio = stats.next_exp > 0 ? static_cast<float>(static_cast<double>(stats.exp) / static_cast<double>(stats.next_exp)) : 0.0f;
          char exp_buf[128];
          std::snprintf(exp_buf, sizeof(exp_buf), "EXP: %s / %s (%.2f%%)",
                        format_thousands(stats.exp).c_str(),
                        format_thousands(stats.next_exp).c_str(),
                        exp_ratio * 100.0f);
          ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.25f, 0.78f, 0.35f, 1.0f));
          ImGui::ProgressBar(std::clamp(exp_ratio, 0.0f, 1.0f), ImVec2(-1.0f, 18.0f), exp_buf);
          ImGui::PopStyleColor();

          ImGui::Spacing();

          // Currencies & Status Row
          if (ImGui::BeginTable("CurrenciesRow", 4, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("EXP", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("SP", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Gold", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Berserk", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const auto exp_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_EXP", "EXP");
            ImGui::TextDisabled("%s:", exp_lbl.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s", format_thousands(stats.exp).c_str());

            ImGui::TableSetColumnIndex(1);
            const auto sp_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_SKILLPOINT", "SP");
            ImGui::TextDisabled("%s:", sp_lbl.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", format_thousands(stats.sp).c_str());
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("Skill Points (SP): %s\nSP Experience: %u / 400 (%.1f%%)",
                                format_thousands(stats.sp).c_str(),
                                stats.sp_exp,
                                (static_cast<float>(stats.sp_exp) / 400.0f) * 100.0f);
            }

            ImGui::TableSetColumnIndex(2);
            const auto gold_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_GOLD", "Gold");
            ImGui::TextDisabled("%s:", gold_lbl.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", format_thousands(stats.gold).c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::TextDisabled("Berserk:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.2f, 1.0f), "Lv.%u", stats.hwan_level);

            ImGui::EndTable();
          }

          ImGui::Separator();
          ImGui::Spacing();

          // 2. Base Attributes & Balance
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Base Attributes & Balance");
          ImGui::Spacing();

          if (ImGui::BeginTable("AttrBalanceTable", 2, ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("STR_Col", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("INT_Col", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // Left: Physical
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Strength (STR):");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", format_thousands(stats.str).c_str());

            char phy_bal_buf[32];
            std::snprintf(phy_bal_buf, sizeof(phy_bal_buf), "Phy Balance: %.0f %%", stats.phy_balance_pct);
            const float phy_ratio = std::clamp(stats.phy_balance_pct / 100.0f, 0.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.35f, 0.2f, 1.0f));
            ImGui::ProgressBar(phy_ratio, ImVec2(-1.0f, 16.0f), phy_bal_buf);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("Physical Balance: %.1f%%\nSilkroad Engine Cap: 120.0%%\nRaw Calculated: %.1f%%",
                                stats.phy_balance_pct, stats.raw_phy_balance_pct);
            }

            // Right: Magical
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Intelligence (INT):");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", format_thousands(stats.int_).c_str());

            char mag_bal_buf[32];
            std::snprintf(mag_bal_buf, sizeof(mag_bal_buf), "Mag Balance: %.0f %%", stats.mag_balance_pct);
            const float mag_ratio = std::clamp(stats.mag_balance_pct / 100.0f, 0.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 0.95f, 1.0f));
            ImGui::ProgressBar(mag_ratio, ImVec2(-1.0f, 16.0f), mag_bal_buf);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("Magical Balance: %.1f%%\nSilkroad Engine Cap: 120.0%%\nRaw Calculated: %.1f%%",
                                stats.mag_balance_pct, stats.raw_mag_balance_pct);
            }

            ImGui::EndTable();
          }

          ImGui::Spacing();
          if (ImGui::BeginTable("StatPointsHonorRow", 2, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("StatPointsCol", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("HonorCol", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const auto stat_point_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_ATTR_REMAINDER", "Stat Point");
            ImGui::TextDisabled("%s:", stat_point_lbl.c_str());
            ImGui::SameLine();
            if (stats.stat_points > 0) {
              ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "+%u", stats.stat_points);
            } else {
              ImGui::TextDisabled("0");
            }

            ImGui::TableSetColumnIndex(1);
            const auto honor_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_HONOR_POINT", "Honor Point");
            ImGui::TextDisabled("%s:", honor_lbl.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "%s", stats.honor_points.c_str());

            ImGui::EndTable();
          }

          ImGui::Separator();
          ImGui::Spacing();

          // 3. Job Identity & Progression (matches native Character window)
          const auto job_header_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_JOB", "Job Identity & Progression");
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "%s", job_header_lbl.c_str());
          ImGui::Spacing();

          if (ImGui::BeginTable("JobIdentityTable", 3, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("JobAliasCol", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("JobLevelCol", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("JobExpCol", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // Job Alias
            ImGui::TableSetColumnIndex(0);
            const auto alias_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_JOB_ALIAS", "Job alias");
            ImGui::TextDisabled("%s:", alias_lbl.c_str());
            ImGui::SameLine();
            const auto alias_str = ext_client::utils::string::to_utf8(stats.job_alias.c_str());
            const auto none_lbl = "<" + ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_NONE", "None") + ">";
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.5f, 1.0f), "%s", alias_str.empty() ? none_lbl.c_str() : alias_str.c_str());

            // Job Level
            ImGui::TableSetColumnIndex(1);
            const auto jlvl_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_JOB_LEVEL", "Job level");
            ImGui::TextDisabled("%s:", jlvl_lbl.c_str());
            ImGui::SameLine();
            if (stats.job_level > 0 || !stats.job_name.empty()) {
              ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.0f), "%s Lv.%u", stats.job_name.c_str(), stats.job_level);
            } else {
              ImGui::TextDisabled("%s", none_lbl.c_str());
            }

            // Job Experience
            ImGui::TableSetColumnIndex(2);
            const auto jexp_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_JOB_EXP", "Job experience");
            ImGui::TextDisabled("%s:", jexp_lbl.c_str());
            ImGui::SameLine();
            char jexp_buf[64];
            std::snprintf(jexp_buf, sizeof(jexp_buf), "%.0f%%", stats.job_exp_ratio * 100.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.88f, 0.55f, 0.20f, 1.0f));
            ImGui::ProgressBar(stats.job_exp_ratio, ImVec2(-1.0f, 16.0f), jexp_buf);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("Job EXP: %s / %s (%.2f%%)",
                                format_thousands(stats.job_exp).c_str(),
                                format_thousands(stats.job_max_exp).c_str(),
                                stats.job_exp_ratio * 100.0f);
            }

            ImGui::EndTable();
          }

          ImGui::Separator();
          ImGui::Spacing();

          // 4. Combat Power (Engine Attributes)
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Combat Power (Engine Attributes)");
          ImGui::Spacing();

          if (ImGui::BeginTable("CombatStatsTable", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Combat Attribute", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Physical", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Magical", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            // Row 1: Attack Power
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Attack Power:");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%s ~ %s", format_thousands(stats.min_phy_atk).c_str(), format_thousands(stats.max_phy_atk).c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s ~ %s", format_thousands(stats.min_mag_atk).c_str(), format_thousands(stats.max_mag_atk).c_str());

            // Row 2: Defense
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Defense Power:");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%s", format_thousands(stats.phy_def).c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", format_thousands(stats.mag_def).c_str());

            // Row 3: Hit & Parry
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Combat Rating:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Hit Rate: %u", stats.hit_rate);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Parry Rate: %u", stats.parry_rate);

            ImGui::EndTable();
          }

          ImGui::EndTabItem();
        }

        // ---------------------------------------------------------------------
        // TAB 1: Equipments
        // ---------------------------------------------------------------------
        const ImGuiTabItemFlags tab1_flags = (s_request_tab == 1) ? ImGuiTabItemFlags_SetSelected : 0;
        if (ImGui::BeginTabItem("Equipments", nullptr, tab1_flags)) {
          current_main_tab = 1;
        static int equip_sub_tab = 0;
        const auto tab_normal = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_AVATAR_VIEW_EQUIPSLOT", "Equipment");
        const auto tab_avatar = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_AVATAR_VIEW_AVATARSLOT", "Avatar");
        const auto tab_job    = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_CONFLICT_JOB_SLOT_VIEW", "Job Equipment");

        if (ImGui::BeginTabBar("LocalEquipTabs")) {
          if (ImGui::BeginTabItem(tab_normal.c_str())) {
            equip_sub_tab = 0;
            ImGui::EndTabItem();
          }
          if (ImGui::BeginTabItem(tab_avatar.c_str())) {
            equip_sub_tab = 1;
            ImGui::EndTabItem();
          }
          if (ImGui::BeginTabItem(tab_job.c_str())) {
            equip_sub_tab = 2;
            ImGui::EndTabItem();
          }
          ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Hover any equipped slot to view complete item attributes & blues:");
        ImGui::Spacing();

        const auto col_slot = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_ARMOR_POSITION", "Slot");
        const auto col_item = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_ITEM_TYPE", "Item");
        const auto empty_str = "<" + ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_SOCKET_EMPTY_SLOT", "Empty") + ">";

        if (equip_sub_tab == 0) {
          // Equipment (13 slots)
          if (ImGui::BeginTable("EquipGrid", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(col_slot.c_str(), ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(col_item.c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (std::uint8_t slot_idx = 0; slot_idx < 13; ++slot_idx) {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
              ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "%s", cic_player::equipment_slot_name_loc(slot_idx).c_str());

              ImGui::TableSetColumnIndex(1);
              auto* item = local_player->get_equipped_item(slot_idx);
              char id_str[32];
              std::snprintf(id_str, sizeof(id_str), "##eq_slot_%u", slot_idx);

              if (item && item->is_valid()) {
                const auto data = ext_client::sdk::game::extract_tooltip_data(item);
                std::string btn_label = data.title;
                if (data.opt_level > 0) {
                  btn_label += " (+" + std::to_string(data.opt_level) + ")";
                }

                ext_client::render::render_item_slot_from_data(id_str, data, ImVec2(24.0f, 24.0f), true);
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

                ImVec4 text_col = !data.subtitle.empty() ? ImVec4(1.0f, 0.82f, 0.15f, 1.0f) :
                                  ((data.opt_level > 0) ? ImVec4(1.0f, 0.85f, 0.25f, 1.0f) : ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                char sel_id[36];
                std::snprintf(sel_id, sizeof(sel_id), "##eq_sel_%u", slot_idx);
                ImGui::Selectable((btn_label + sel_id).c_str(), false, ImGuiSelectableFlags_None);
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                  ext_client::render::render_item_tooltip_from_data(data);
                }
              } else {
                ext_client::render::render_item_slot(id_str, nullptr, ImVec2(24.0f, 24.0f), false);
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
                ImGui::TextDisabled("%s", empty_str.c_str());
              }
            }
            ImGui::EndTable();
          }
        } else if (equip_sub_tab == 1) {
          // Avatar (5 slots)
          if (ImGui::BeginTable("AvatarGrid", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(col_slot.c_str(), ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn(col_item.c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (std::uint8_t slot_idx = 0; slot_idx < 5; ++slot_idx) {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
              ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "%s", cic_player::avatar_slot_name_loc(slot_idx).c_str());

              ImGui::TableSetColumnIndex(1);
              auto* item = local_player->get_avatar_item(slot_idx);
              char id_str[32];
              std::snprintf(id_str, sizeof(id_str), "##av_slot_%u", slot_idx);

              if (item && item->is_valid()) {
                const auto data = ext_client::sdk::game::extract_tooltip_data(item);
                std::string btn_label = data.title;
                if (data.opt_level > 0) {
                  btn_label += " (+" + std::to_string(data.opt_level) + ")";
                }

                ext_client::render::render_item_slot_from_data(id_str, data, ImVec2(24.0f, 24.0f), true);
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

                ImVec4 text_col = !data.subtitle.empty() ? ImVec4(1.0f, 0.82f, 0.15f, 1.0f) :
                                  ((data.opt_level > 0) ? ImVec4(1.0f, 0.85f, 0.25f, 1.0f) : ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                char sel_id[36];
                std::snprintf(sel_id, sizeof(sel_id), "##av_sel_%u", slot_idx);
                ImGui::Selectable((btn_label + sel_id).c_str(), false, ImGuiSelectableFlags_None);
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                  ext_client::render::render_item_tooltip_from_data(data);
                }
              } else {
                ext_client::render::render_item_slot(id_str, nullptr, ImVec2(24.0f, 24.0f), false);
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
                ImGui::TextDisabled("%s", empty_str.c_str());
              }
            }
            ImGui::EndTable();
          }
        } else if (equip_sub_tab == 2) {
          // Job Equipment (11 slots)
          if (ImGui::BeginTable("JobGrid", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(col_slot.c_str(), ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn(col_item.c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (std::uint8_t slot_idx = 0; slot_idx < 11; ++slot_idx) {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
              ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.6f, 1.0f), "%s", cic_player::job_slot_name_loc(slot_idx).c_str());

              ImGui::TableSetColumnIndex(1);
              auto* item = local_player->get_job_equipped_item(slot_idx);
              char id_str[32];
              std::snprintf(id_str, sizeof(id_str), "##job_slot_%u", slot_idx);

              if (item && item->is_valid()) {
                const auto data = ext_client::sdk::game::extract_tooltip_data(item);
                std::string btn_label = data.title;
                if (data.opt_level > 0) {
                  btn_label += " (+" + std::to_string(data.opt_level) + ")";
                }

                ext_client::render::render_item_slot_from_data(id_str, data, ImVec2(24.0f, 24.0f), true);
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

                ImVec4 text_col = !data.subtitle.empty() ? ImVec4(1.0f, 0.82f, 0.15f, 1.0f) :
                                  ((data.opt_level > 0) ? ImVec4(1.0f, 0.85f, 0.25f, 1.0f) : ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, text_col);
                char sel_id[36];
                std::snprintf(sel_id, sizeof(sel_id), "##job_sel_%u", slot_idx);
                ImGui::Selectable((btn_label + sel_id).c_str(), false, ImGuiSelectableFlags_None);
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                  ext_client::render::render_item_tooltip_from_data(data);
                }
              } else {
                ext_client::render::render_item_slot(id_str, nullptr, ImVec2(24.0f, 24.0f), false);
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
                ImGui::TextDisabled("%s", empty_str.c_str());
              }
            }
            ImGui::EndTable();
          }
        }
        ImGui::EndTabItem();
      }

      // ---------------------------------------------------------------------
      // TAB 2: Inventory (Bag)
      // ---------------------------------------------------------------------
      const ImGuiTabItemFlags tab2_flags = (s_request_tab == 2) ? ImGuiTabItemFlags_SetSelected : 0;
      if (ImGui::BeginTabItem("Inventory (Bag)", nullptr, tab2_flags)) {
        current_main_tab = 2;
        static int bag_page = 0; // 0=Page 1 (0-44), 1=Page 2 (45-89), 2=Page 3 (90-134)
        static char bag_filter[64] = "";

        const auto total_bag_slots = local_player->get_bag_item_count();

        // Controls bar: Page selectors and search box
        ImGui::Spacing();
        ImGui::TextDisabled("Bag Page:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Page 1 (1-45)", bag_page == 0)) bag_page = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("Page 2 (46-90)", bag_page == 1)) bag_page = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton("Page 3 (91-135)", bag_page == 2)) bag_page = 2;

        ImGui::SameLine(ImGui::GetWindowWidth() - 200.0f);
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputTextWithHint("##BagFilter", "Filter Items...", bag_filter, sizeof(bag_filter));

        const std::string filter_str = bag_filter;
        std::string filter_lower = filter_str;
        std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const std::size_t page_start = static_cast<std::size_t>(bag_page) * 45;
        const std::size_t page_end   = page_start + 45;

        // Count occupied slots in this page
        std::size_t occupied_page = 0;
        for (std::size_t i = page_start; i < page_end && i < total_bag_slots; ++i) {
          if (auto* it = local_player->get_bag_item(i)) {
            occupied_page++;
          }
        }

        ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "Page %d Slots: %zu / 45 Occupied (Total Bag Capacity: %zu)",
                           bag_page + 1, occupied_page, total_bag_slots);
        ImGui::Spacing();

        // 9 columns x 5 rows grid (exact Silkroad layout)
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
        for (int row = 0; row < 5; ++row) {
          for (int col = 0; col < 9; ++col) {
            const std::size_t slot_idx = page_start + static_cast<std::size_t>(row * 9 + col);
            char slot_str_id[32];
            std::snprintf(slot_str_id, sizeof(slot_str_id), "##bag_slot_%zu", slot_idx);

            auto* item = (slot_idx < total_bag_slots) ? local_player->get_bag_item(slot_idx) : nullptr;

            if (item && item->is_valid()) {
              const auto data = ext_client::sdk::game::extract_tooltip_data(item);

              bool matches_filter = true;
              if (!filter_lower.empty()) {
                std::string title_lower = data.title;
                std::transform(title_lower.begin(), title_lower.end(), title_lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                matches_filter = (title_lower.find(filter_lower) != std::string::npos);
              }

              if (!matches_filter) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.25f);
              }

              ext_client::render::render_item_slot_from_data(slot_str_id, data, ImVec2(36.0f, 36.0f), true);

              if (!matches_filter) {
                ImGui::PopStyleVar();
              }
            } else {
              // Empty slot
              ext_client::render::render_item_slot(slot_str_id, nullptr, ImVec2(36.0f, 36.0f), false);
            }

            if (col < 8) {
              ImGui::SameLine();
            }
          }
        }
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Hover any inventory slot to view full item parameters, blues, and stats.");
        ImGui::EndTabItem();
      }

      // ---------------------------------------------------------------------
      // TAB 3: Skills & Masteries (Mimics Native Silkroad CIFSkill Window)
      // ---------------------------------------------------------------------
      const ImGuiTabItemFlags tab3_flags = (s_request_tab == 3) ? ImGuiTabItemFlags_SetSelected : 0;
      if (ImGui::BeginTabItem("Skills & Masteries", nullptr, tab3_flags)) {
        current_main_tab = 3;
        const auto masteries = c_skill_manager::get_learned_masteries();
        const auto skill_ids = c_skill_manager::get_learned_skill_ids();

        static int skill_category = 0; // 0=All, 1=Attack, 2=Buff, 3=Passive
        static int s_skill_view_mode = 0; // 0 = Native Tree (8 slots/series), 1 = Table View
        static int s_active_archetype = -1; // 0=Melee/Weapon, 1=Caster/Force, 2=Buff, 3=Job, 4=All
        static std::uint32_t s_active_mastery_id = 0;
        static char skill_search[64] = "";

        // Build detailed skill list
        std::vector<c_skill_manager::s_skill_details> detailed_skills;
        detailed_skills.reserve(skill_ids.size());
        for (const auto sid : skill_ids) {
          detailed_skills.push_back(c_skill_manager::get_skill_details(sid));
        }

        // Search string lowercase
        const std::string s_search = skill_search;
        std::string search_lower = s_search;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        // Total mastery levels (calculated natively via sub_B10010)
        const std::uint32_t total_mastery_lvl = c_skill_manager::get_total_mastery_level();

        // Local player Skill Points (SP)
        const std::uint32_t player_sp = (local_player) ? local_player->sp() : 0;

        // Check if character has European or Chinese masteries
        bool has_europe = false;
        bool has_china = false;
        for (const auto& m : masteries) {
          if (m.id >= 513 && m.id <= 518) has_europe = true;
          if ((m.id >= 1 && m.id <= 7) || (m.id >= 257 && m.id <= 263 && m.id != 513) || m.id == 276) has_china = true;
        }
        if (!has_europe && !has_china) {
          has_europe = true; // default
        }

        const std::uint32_t max_mastery_cap = c_skill_manager::get_max_mastery_cap(has_europe, local_player ? local_player->level() : 0);

        // Define Archetypes
        struct s_archetype_def {
          int id;
          std::string label;
          std::vector<std::uint32_t> mastery_ids;
        };

        std::vector<s_archetype_def> archetypes;
        if (has_europe) {
          archetypes.push_back({0, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_WARRIOR_SKILL", "Melee"), {513, 515}});
          archetypes.push_back({1, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_WIZARD_SKILL", "Caster"), {514, 516}});
          archetypes.push_back({2, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_BARD_SKILL", "Buff"), {517, 518}});
          archetypes.push_back({3, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_CONFLICT_MERCHANT", "Job"), {1000, 277, 264, 255, 115, 101, 102, 103}});
          archetypes.push_back({4, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_PARTYMATCH_PSEARCH_OBJECTALL", "All"), {}});
        } else {
          archetypes.push_back({0, ext_client::sdk::ui::get_string_utf8(L"SN_TAB_WEAPON", "Weapon"), {1, 2, 3, 257, 258, 259}});
          archetypes.push_back({1, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_FORCE_SKILL", "Force"), {4, 5, 6, 7, 260, 261, 262, 276}});
          archetypes.push_back({2, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_CONFLICT_MERCHANT", "Job"), {1000, 277, 264, 255, 115, 101, 102, 103}});
          archetypes.push_back({3, ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_PARTYMATCH_PSEARCH_OBJECTALL", "All"), {}});
        }

        // Auto-select initial archetype & mastery
        if (s_active_archetype == -1) {
          std::uint32_t best_mid = 0;
          std::uint8_t best_lvl = 0;
          for (const auto& m : masteries) {
            if (m.id == 1000 || m.id == 255 || m.id == 277 || m.id == 264) continue;
            if (m.level > best_lvl) {
              best_lvl = m.level;
              best_mid = m.id;
            }
          }
          if (best_mid == 0 && !masteries.empty()) {
            best_mid = masteries.front().id;
          }

          s_active_mastery_id = best_mid;
          s_active_archetype = 0;
          for (const auto& arch : archetypes) {
            for (const auto mid : arch.mastery_ids) {
              if (mid == best_mid) {
                s_active_archetype = arch.id;
                break;
              }
            }
          }
        }

        const bool is_job_archetype = (has_europe && s_active_archetype == 3) || (!has_europe && s_active_archetype == 2);
        const bool is_job_active = is_job_archetype || (s_active_mastery_id == 1000 || s_active_mastery_id == 277 ||
                                                        s_active_mastery_id == 264 || s_active_mastery_id == 255 ||
                                                        s_active_mastery_id == 115);

        // =====================================================================
        // 1. TOP ARCHETYPE TABS (Native Silkroad Style)
        // =====================================================================
        ImGui::Spacing();
        for (std::size_t ai = 0; ai < archetypes.size(); ++ai) {
          const auto& arch = archetypes[ai];
          const bool is_arch_active = (s_active_archetype == arch.id);

          if (is_arch_active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.35f, 0.58f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.42f, 0.68f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.78f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.18f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.20f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.35f, 0.48f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.78f, 0.88f, 1.0f));
          }

          char arch_btn_id[64];
          std::snprintf(arch_btn_id, sizeof(arch_btn_id), "%s##arch_%d", arch.label.c_str(), arch.id);
          if (ImGui::Button(arch_btn_id, ImVec2(76.0f, 26.0f))) {
            s_active_archetype = arch.id;
            if (!arch.mastery_ids.empty()) {
              s_active_mastery_id = arch.mastery_ids.front();
              for (const auto mid : arch.mastery_ids) {
                for (const auto& m : masteries) {
                  if (m.id == mid && m.level > 0) {
                    s_active_mastery_id = mid;
                    break;
                  }
                }
              }
            } else {
              s_active_mastery_id = 0; // All
            }
          }
          ImGui::PopStyleColor(4);

          if (ai + 1 < archetypes.size()) {
            ImGui::SameLine(0.0f, 4.0f);
          }
        }

        // View mode toggle on the far right of archetype bar
        ImGui::SameLine(ImGui::GetWindowWidth() - 170.0f);
        if (ImGui::RadioButton("Skill Tree", s_skill_view_mode == 0)) s_skill_view_mode = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("Table", s_skill_view_mode == 1)) s_skill_view_mode = 1;

        // =====================================================================
        // 2. MASTERY SUB-TABS (Under Active Archetype)
        // =====================================================================
        ImGui::Spacing();
        std::vector<std::uint32_t> sub_tab_mids;
        if (is_job_archetype) {
          sub_tab_mids = {1000};
        } else {
          for (const auto& arch : archetypes) {
            if (arch.id == s_active_archetype) {
              sub_tab_mids = arch.mastery_ids;
              break;
            }
          }
        }

        if (sub_tab_mids.empty()) {
          // When 'All' archetype is active:
          sub_tab_mids.push_back(0);
          for (const auto& m : masteries) {
            if (m.level > 0 || m.id == 1000) {
              sub_tab_mids.push_back(m.id);
            }
          }
          if (sub_tab_mids.size() == 1) {
            for (const auto& m : masteries) {
              sub_tab_mids.push_back(m.id);
            }
          }
          bool has_job_sub = false;
          for (const auto mid : sub_tab_mids) {
            if (mid == 1000) { has_job_sub = true; break; }
          }
          if (!has_job_sub) {
            for (const auto& sk : detailed_skills) {
              if (sk.is_job_skill || sk.mastery_id == 1000) {
                sub_tab_mids.push_back(1000);
                break;
              }
            }
          }
        } else if (!has_europe && !is_job_archetype) {
          // For Chinese archetypes, filter duplicate IDs that aren't in masteries
          std::vector<std::uint32_t> filtered_mids;
          for (const auto mid : sub_tab_mids) {
            bool in_masteries = false;
            for (const auto& m : masteries) {
              if (m.id == mid) {
                in_masteries = true;
                break;
              }
            }
            if (in_masteries) {
              filtered_mids.push_back(mid);
            }
          }
          if (!filtered_mids.empty()) {
            sub_tab_mids = filtered_mids;
          }
        }

        // Safety sync: ensure s_active_mastery_id is valid within sub_tab_mids
        bool is_mid_in_subtabs = false;
        for (const auto mid : sub_tab_mids) {
          if (mid == s_active_mastery_id) {
            is_mid_in_subtabs = true;
            break;
          }
        }
        if (!is_mid_in_subtabs && !sub_tab_mids.empty()) {
          s_active_mastery_id = sub_tab_mids.front();
          for (const auto mid : sub_tab_mids) {
            for (const auto& m : masteries) {
              if (m.id == mid && m.level > 0) {
                s_active_mastery_id = mid;
                break;
              }
            }
          }
        }

        if (!sub_tab_mids.empty()) {
          ImGui::BeginGroup();
          for (std::size_t mi = 0; mi < sub_tab_mids.size(); ++mi) {
            const auto mid = sub_tab_mids[mi];
            const std::string m_name = (mid == 0) ? "All" : c_skill_manager::get_mastery_name_by_id(mid);
            std::uint8_t m_lvl = 0;
            if (is_job_active || mid == 1000 || mid == 277 || mid == 264 || mid == 255) {
              m_lvl = local_player ? local_player->job_level() : 0;
            } else if (mid > 0) {
              for (const auto& m : masteries) {
                if (m.id == mid) {
                  m_lvl = m.level;
                  break;
                }
              }
            }

            const bool is_this_m_active = (s_active_mastery_id == mid);

            if (is_this_m_active) {
              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.40f, 0.65f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.48f, 0.75f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 0.85f, 1.0f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            } else {
              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.13f, 0.19f, 0.7f));
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.22f, 0.32f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.30f, 0.42f, 0.4f));
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.75f, 0.82f, 1.0f));
            }

            char sub_label[64];
            if (mid == 0) {
              std::snprintf(sub_label, sizeof(sub_label), "All Skills##sub_m_0");
            } else if (m_lvl > 0) {
              std::snprintf(sub_label, sizeof(sub_label), "%s (Lv.%u)##sub_m_%u", m_name.c_str(), m_lvl, mid);
            } else {
              std::snprintf(sub_label, sizeof(sub_label), "%s##sub_m_%u", m_name.c_str(), mid);
            }

            if (ImGui::Button(sub_label, ImVec2(0.0f, 22.0f))) {
              s_active_mastery_id = mid;
            }
            ImGui::PopStyleColor(4);

            if (mi + 1 < sub_tab_mids.size()) {
              ImGui::SameLine(0.0f, 6.0f);
            }
          }
          ImGui::EndGroup();
        }

        // =====================================================================
        // 3. MASTERY HEADER BANNER (Emblem + Name + Level Badge)
        // =====================================================================
        ImGui::Spacing();
        std::string active_m_name = (s_active_mastery_id > 0)
                                        ? c_skill_manager::get_mastery_name_by_id(s_active_mastery_id)
                                        : "All";
        std::uint8_t active_m_level = 0;
        if (is_job_active || s_active_mastery_id == 1000 || s_active_mastery_id == 277 || s_active_mastery_id == 264 || s_active_mastery_id == 255) {
          active_m_name = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_CONFLICT_JOB_SKILL", "Job Skills");
          active_m_level = local_player ? local_player->job_level() : 0;
        } else {
          for (const auto& m : masteries) {
            if (m.id == s_active_mastery_id) {
              active_m_level = m.level;
              break;
            }
          }
        }

        const ImVec2 head_p0 = ImGui::GetCursorScreenPos();
        const float head_w = ImGui::GetContentRegionAvail().x;
        const float head_h = 36.0f;
        const ImVec2 head_p1(head_p0.x + head_w, head_p0.y + head_h);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Background: Silkroad dark metallic recessed bar
        dl->AddRectFilled(head_p0, head_p1, IM_COL32(12, 18, 28, 240), 4.0f);
        dl->AddRect(head_p0, head_p1, IM_COL32(40, 60, 88, 200), 4.0f, 0, 1.5f);

        // Circular Emblem disc on left
        const ImVec2 emblem_center(head_p0.x + 18.0f, head_p0.y + 18.0f);
        dl->AddCircleFilled(emblem_center, 12.0f, IM_COL32(20, 45, 75, 255));
        dl->AddCircle(emblem_center, 12.0f, IM_COL32(80, 160, 240, 255), 0, 1.5f);

        const char emblem_glyph[2] = {active_m_name.empty() ? 'A' : active_m_name[0], '\0'};
        const ImVec2 g_sz = ImGui::CalcTextSize(emblem_glyph);
        dl->AddText(ImVec2(emblem_center.x - g_sz.x * 0.5f, emblem_center.y - g_sz.y * 0.5f),
                    IM_COL32(200, 235, 255, 255), emblem_glyph);

        // Mastery Name text
        char title_buf[128];
        if (is_job_active || s_active_mastery_id == 1000 || s_active_mastery_id == 277 || s_active_mastery_id == 264 || s_active_mastery_id == 255) {
          std::snprintf(title_buf, sizeof(title_buf), "%s", active_m_name.c_str());
        } else if (s_active_mastery_id > 0) {
          if (active_m_name.find("Mastery") == std::string::npos &&
              active_m_name.find("mastery") == std::string::npos &&
              active_m_name.find("Skills") == std::string::npos &&
              active_m_name.find("skills") == std::string::npos) {
            std::snprintf(title_buf, sizeof(title_buf), "%s Mastery", active_m_name.c_str());
          } else {
            std::snprintf(title_buf, sizeof(title_buf), "%s", active_m_name.c_str());
          }
        } else {
          std::snprintf(title_buf, sizeof(title_buf), "All Learned Skills");
        }
        dl->AddText(ImVec2(head_p0.x + 36.0f, head_p0.y + 9.0f), IM_COL32(230, 240, 255, 255), title_buf);

        // Level Badge on right
        if (s_active_mastery_id > 0 || is_job_active) {
          char lvl_box_buf[32];
          std::snprintf(lvl_box_buf, sizeof(lvl_box_buf), "Lv %u", active_m_level);
          const ImVec2 lvl_text_sz = ImGui::CalcTextSize(lvl_box_buf);
          const float badge_w = lvl_text_sz.x + 16.0f;
          const ImVec2 b0(head_p1.x - badge_w - 8.0f, head_p0.y + 6.0f);
          const ImVec2 b1(head_p1.x - 8.0f, head_p0.y + head_h - 6.0f);
          dl->AddRectFilled(b0, b1, IM_COL32(8, 12, 18, 220), 3.0f);
          dl->AddRect(b0, b1, IM_COL32(60, 90, 130, 180), 3.0f);
          dl->AddText(ImVec2(b0.x + 8.0f, b0.y + 4.0f), IM_COL32(80, 220, 255, 255), lvl_box_buf);
        } else {
          char total_buf[64];
          std::snprintf(total_buf, sizeof(total_buf), "%zu Skills", detailed_skills.size());
          const ImVec2 lvl_text_sz = ImGui::CalcTextSize(total_buf);
          const float badge_w = lvl_text_sz.x + 16.0f;
          const ImVec2 b0(head_p1.x - badge_w - 8.0f, head_p0.y + 6.0f);
          const ImVec2 b1(head_p1.x - 8.0f, head_p0.y + head_h - 6.0f);
          dl->AddRectFilled(b0, b1, IM_COL32(8, 12, 18, 220), 3.0f);
          dl->AddRect(b0, b1, IM_COL32(60, 90, 130, 180), 3.0f);
          dl->AddText(ImVec2(b0.x + 8.0f, b0.y + 4.0f), IM_COL32(80, 220, 255, 255), total_buf);
        }

        ImGui::Dummy(ImVec2(head_w, head_h));
        ImGui::Spacing();

        // Filter / Search Row
        ImGui::SetNextItemWidth(130.0f);
        ImGui::InputTextWithHint("##SkillTreeSearch", "Search Skill...", skill_search, sizeof(skill_search));
        ImGui::SameLine(0.0f, 12.0f);

        if (ImGui::RadioButton("All##cat", skill_category == 0)) skill_category = 0;
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::RadioButton("Attack##cat", skill_category == 1)) skill_category = 1;
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::RadioButton("Buff##cat", skill_category == 2)) skill_category = 2;
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::RadioButton("Passive##cat", skill_category == 3)) skill_category = 3;

        ImGui::Spacing();

        // =====================================================================
        // 4. SKILL TREE / BRANCH ROWS (View Mode 0) or TABLE VIEW (View Mode 1)
        // =====================================================================
        if (s_skill_view_mode == 0) {
          ImGui::BeginChild("NativeSkillTreeScroll", ImVec2(0.0f, 320.0f), true, ImGuiWindowFlags_None);

          // 1. Filter active skills for the current view
          std::vector<c_skill_manager::s_skill_details> active_skills;
          active_skills.reserve(detailed_skills.size());

          for (const auto& sk : detailed_skills) {
            const bool is_this_skill_job = sk.is_job_skill || sk.mastery_id == 1000 || sk.mastery_id == 277 ||
                                           sk.mastery_id == 264 || sk.mastery_id == 255 || sk.mastery_id == 115 ||
                                           ((sk.mastery_id > 0xFFFF) && ((sk.mastery_id >> 16) == 1000 || (sk.mastery_id >> 16) == 277 || (sk.mastery_id >> 16) == 264 || (sk.mastery_id >> 16) == 255 || (sk.mastery_id >> 16) == 115));

            if (is_job_active) {
              if (!is_this_skill_job) {
                continue;
              }
            } else if (s_active_mastery_id != 0) {
              const std::uint32_t sk_mid = (sk.mastery_id > 0xFFFF) ? (sk.mastery_id >> 16) : sk.mastery_id;
              if (is_this_skill_job || sk_mid != s_active_mastery_id) {
                continue;
              }
            }
            if (skill_category == 1 && sk.skill_type != 1) continue;
            if (skill_category == 2 && sk.skill_type != 2) continue;
            if (skill_category == 3 && sk.skill_type != 0) continue;
            if (!search_lower.empty()) {
              std::string name_l = sk.name;
              std::transform(name_l.begin(), name_l.end(), name_l.begin(),
                             [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
              if (name_l.find(search_lower) == std::string::npos && sk.code_name.find(search_lower) == std::string::npos) {
                continue;
              }
            }
            active_skills.push_back(sk);
          }

          if (active_skills.empty()) {
            ImGui::Spacing();
            ImGui::TextDisabled("No skills match the selected mastery and filter.");
          } else {
            // Silkroad Native Skill Board Layout:
            // Query exact mastery row count from game engine (e.g. Wizard = 9 rows, Bicheon = 8 rows)
            int engine_rows = (s_active_mastery_id > 0) ? c_skill_manager::get_mastery_row_count(s_active_mastery_id) : 0;
            int max_skill_row = -1;
            for (const auto& sk : active_skills) {
              if (sk.branch_index > max_skill_row) {
                max_skill_row = sk.branch_index;
              }
            }
            int num_rows = std::max(engine_rows, max_skill_row + 1);
            if (num_rows <= 0) {
              num_rows = active_skills.empty() ? 0 : 1;
            }

            const float slot_sz = 36.0f;
            const float gap = 4.0f;

            for (int b = 0; b < num_rows; ++b) {
              // Determine number of slots in this row (standard Silkroad bar has 8 slots: 0..7)
              int row_max_slot = 7;
              for (const auto& sk : active_skills) {
                if (sk.branch_index == b && sk.slot_index > row_max_slot) {
                  row_max_slot = std::min(sk.slot_index, 11);
                }
              }
              const int num_slots_in_row = row_max_slot + 1;

              // Collect the slots for row b
              std::vector<const c_skill_manager::s_skill_details*> chain_slots(num_slots_in_row, nullptr);
              const c_skill_manager::s_skill_details* branch_rep = nullptr;

              for (const auto& sk : active_skills) {
                if (sk.branch_index == b) {
                  if (!branch_rep && !sk.icon_path.empty()) {
                    branch_rep = &sk;
                  }
                  if (sk.slot_index >= 0 && sk.slot_index < num_slots_in_row) {
                    const auto* cur = chain_slots[sk.slot_index];
                    if (!cur || sk.level > cur->level) {
                      chain_slots[sk.slot_index] = &sk;
                    }
                  }
                }
              }

              // Fallback for any skill in this row with unmapped slot_index
              for (const auto& sk : active_skills) {
                if (sk.branch_index == b && (sk.slot_index < 0 || sk.slot_index >= num_slots_in_row)) {
                  for (int c = 0; c < num_slots_in_row; ++c) {
                    if (!chain_slots[c]) {
                      chain_slots[c] = &sk;
                      break;
                    }
                  }
                }
              }

              ImGui::PushID(b);

              const ImVec2 row_p0 = ImGui::GetCursorScreenPos();
              const float row_w = ImGui::GetContentRegionAvail().x;
              const float row_h = slot_sz + 6.0f;
              const ImVec2 row_p1(row_p0.x + row_w, row_p0.y + row_h);
              ImDrawList* row_dl = ImGui::GetWindowDrawList();

              auto* bar_tex = ext_client::render::get_texture("interface\\skill\\skl_mastery_bar.ddj");
              if (bar_tex) {
                row_dl->AddImage(reinterpret_cast<ImTextureID>(bar_tex), row_p0, row_p1);
              } else {
                row_dl->AddRectFilled(row_p0, row_p1, IM_COL32(14, 18, 26, 140), 2.0f);
                row_dl->AddRect(row_p0, row_p1, IM_COL32(28, 38, 52, 100), 2.0f);
              }

              ImGui::SetCursorScreenPos(ImVec2(row_p0.x + 4.0f, row_p0.y + 3.0f));

              // 1. Series Emblem on Left (Native Silkroad emblem or representative skill icon)
              const ImVec2 emb_p0 = ImGui::GetCursorScreenPos();
              const ImVec2 emb_p1(emb_p0.x + slot_sz, emb_p0.y + slot_sz);

              row_dl->AddRectFilled(emb_p0, emb_p1, IM_COL32(40, 32, 16, 220), 3.0f);
              row_dl->AddRect(emb_p0, emb_p1, IM_COL32(212, 175, 55, 230), 3.0f, 0, 1.5f);

              std::string emblem_icon_path;
              if (s_active_mastery_id > 0) {
                c_skill_manager::get_mastery_row_emblem(s_active_mastery_id, b, &emblem_icon_path);
              }
              if (emblem_icon_path.empty() && branch_rep && !branch_rep->icon_path.empty()) {
                emblem_icon_path = branch_rep->icon_path;
              }

              auto* emb_tex = !emblem_icon_path.empty()
                                  ? ext_client::render::get_texture(emblem_icon_path)
                                  : nullptr;
              if (emb_tex) {
                row_dl->AddImage(reinterpret_cast<ImTextureID>(emb_tex),
                                 ImVec2(emb_p0.x + 2.0f, emb_p0.y + 2.0f),
                                 ImVec2(emb_p1.x - 2.0f, emb_p1.y - 2.0f));
              } else {
                char s_num[8];
                std::snprintf(s_num, sizeof(s_num), "%d", b + 1);
                const ImVec2 sn_sz = ImGui::CalcTextSize(s_num);
                row_dl->AddText(ImVec2(emb_p0.x + (slot_sz - sn_sz.x) * 0.5f, emb_p0.y + (slot_sz - sn_sz.y) * 0.5f),
                                IM_COL32(255, 220, 120, 255), s_num);
              }

              ImGui::InvisibleButton("##branch_emb", ImVec2(slot_sz, slot_sz));
              if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Series %d", b + 1);
                if (branch_rep) {
                  ImGui::TextDisabled("Skill Chain: %s", branch_rep->name.c_str());
                }
                ImGui::EndTooltip();
              }

              ImGui::SameLine(0.0f, 8.0f);

              // 2. Horizontal Slots for this Series (Native Silkroad Layout)
              for (int slot_idx = 0; slot_idx < num_slots_in_row; ++slot_idx) {
                const auto* sk_ptr = chain_slots[slot_idx];
                if (sk_ptr) {
                  char slot_str_id[32];
                  std::snprintf(slot_str_id, sizeof(slot_str_id), "##sk_t_%u_%d_%d", sk_ptr->id, b, slot_idx);
                  ext_client::render::render_skill_slot(slot_str_id, *sk_ptr, ImVec2(slot_sz, slot_sz), true);
                } else {
                  ext_client::render::render_empty_skill_slot(ImVec2(slot_sz, slot_sz));
                }

                if (slot_idx + 1 < num_slots_in_row) {
                  ImGui::SameLine(0.0f, gap);
                }
              }

              ImGui::SetCursorScreenPos(ImVec2(row_p0.x, row_p1.y + 4.0f));
              ImGui::PopID();
            }

            // Fallback: If any active skills were outside [0, num_rows), render them in an extra series row
            std::vector<const c_skill_manager::s_skill_details*> orphan_skills;
            for (const auto& sk : active_skills) {
              if (sk.branch_index < 0 || sk.branch_index >= num_rows) {
                orphan_skills.push_back(&sk);
              }
            }
            if (!orphan_skills.empty()) {
              const int extra_b = num_rows;
              ImGui::PushID(extra_b);

              const ImVec2 row_p0 = ImGui::GetCursorScreenPos();
              const float row_w = ImGui::GetContentRegionAvail().x;
              const float row_h = slot_sz + 6.0f;
              const ImVec2 row_p1(row_p0.x + row_w, row_p0.y + row_h);
              ImDrawList* row_dl = ImGui::GetWindowDrawList();

              auto* bar_tex = ext_client::render::get_texture("interface\\skill\\skl_mastery_bar.ddj");
              if (bar_tex) {
                row_dl->AddImage(reinterpret_cast<ImTextureID>(bar_tex), row_p0, row_p1);
              } else {
                row_dl->AddRectFilled(row_p0, row_p1, IM_COL32(14, 18, 26, 140), 2.0f);
              }

              ImGui::SetCursorScreenPos(ImVec2(row_p0.x + 4.0f, row_p0.y + 3.0f));

              const ImVec2 emb_p0 = ImGui::GetCursorScreenPos();
              const ImVec2 emb_p1(emb_p0.x + slot_sz, emb_p0.y + slot_sz);
              row_dl->AddRectFilled(emb_p0, emb_p1, IM_COL32(40, 32, 16, 220), 3.0f);
              row_dl->AddRect(emb_p0, emb_p1, IM_COL32(212, 175, 55, 230), 3.0f, 0, 1.5f);

              auto* extra_emb_tex = !orphan_skills.front()->icon_path.empty()
                                        ? ext_client::render::get_texture(orphan_skills.front()->icon_path)
                                        : nullptr;
              if (extra_emb_tex) {
                row_dl->AddImage(reinterpret_cast<ImTextureID>(extra_emb_tex),
                                 ImVec2(emb_p0.x + 2.0f, emb_p0.y + 2.0f),
                                 ImVec2(emb_p1.x - 2.0f, emb_p1.y - 2.0f));
              }

              ImGui::InvisibleButton("##orphan_emb", ImVec2(slot_sz, slot_sz));
              ImGui::SameLine(0.0f, 8.0f);

              const int orphan_slots = std::max(8, static_cast<int>(orphan_skills.size()));
              for (int slot_idx = 0; slot_idx < orphan_slots; ++slot_idx) {
                if (slot_idx < static_cast<int>(orphan_skills.size())) {
                  const auto* sk_ptr = orphan_skills[slot_idx];
                  char slot_str_id[32];
                  std::snprintf(slot_str_id, sizeof(slot_str_id), "##sk_orph_%u_%d", sk_ptr->id, slot_idx);
                  ext_client::render::render_skill_slot(slot_str_id, *sk_ptr, ImVec2(slot_sz, slot_sz), true);
                } else {
                  ext_client::render::render_empty_skill_slot(ImVec2(slot_sz, slot_sz));
                }
                if (slot_idx + 1 < orphan_slots) {
                  ImGui::SameLine(0.0f, gap);
                }
              }

              ImGui::SetCursorScreenPos(ImVec2(row_p0.x, row_p1.y + 4.0f));
              ImGui::PopID();
            }
          }

          ImGui::EndChild();
        } else {
          // Table View (View Mode 1) - Consolidates to highest learned level per chain
          if (ImGui::BeginTable("LearnedSkillsTable", 6, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 320.0f))) {
            ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 36.0f);
            ImGui::TableSetupColumn("Skill Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 65.0f);
            ImGui::TableSetupColumn("MP", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Cast / CD", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableHeadersRow();

            // Consolidate skills to highest learned level per series / chain
            std::vector<c_skill_manager::s_skill_details> table_skills;
            for (const auto& sk : detailed_skills) {
              const bool is_this_skill_job = sk.is_job_skill || sk.mastery_id == 1000 || sk.mastery_id == 277 ||
                                             sk.mastery_id == 264 || sk.mastery_id == 255 || sk.mastery_id == 115 ||
                                             ((sk.mastery_id > 0xFFFF) && ((sk.mastery_id >> 16) == 1000 || (sk.mastery_id >> 16) == 277 || (sk.mastery_id >> 16) == 264 || (sk.mastery_id >> 16) == 255 || (sk.mastery_id >> 16) == 115));

              if (is_job_active) {
                if (!is_this_skill_job) continue;
              } else if (s_active_mastery_id != 0) {
                const std::uint32_t sk_mid = (sk.mastery_id > 0xFFFF) ? (sk.mastery_id >> 16) : sk.mastery_id;
                if (is_this_skill_job || sk_mid != s_active_mastery_id) continue;
              }
              if (skill_category == 1 && sk.skill_type != 1) continue;
              if (skill_category == 2 && sk.skill_type != 2) continue;
              if (skill_category == 3 && sk.skill_type != 0) continue;
              if (!search_lower.empty()) {
                std::string name_l = sk.name;
                std::transform(name_l.begin(), name_l.end(), name_l.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (name_l.find(search_lower) == std::string::npos && sk.code_name.find(search_lower) == std::string::npos) continue;
              }

              auto it = std::find_if(table_skills.begin(), table_skills.end(), [&sk](const auto& item) {
                return (sk.series_id != 0 && item.series_id == sk.series_id) ||
                       (sk.branch_index == item.branch_index && sk.slot_index == item.slot_index);
              });
              if (it != table_skills.end()) {
                if (sk.level > it->level) {
                  *it = sk;
                }
              } else {
                table_skills.push_back(sk);
              }
            }

            std::sort(table_skills.begin(), table_skills.end(), [](const auto& a, const auto& b) {
              if (a.branch_index != b.branch_index) return a.branch_index < b.branch_index;
              if (a.slot_index != b.slot_index) return a.slot_index < b.slot_index;
              return a.level > b.level;
            });

            for (const auto& sk : table_skills) {
              ImGui::TableNextRow();

              // Col 0: Icon
              ImGui::TableSetColumnIndex(0);
              char slot_id_str[32];
              std::snprintf(slot_id_str, sizeof(slot_id_str), "##sk_tbl_%u", sk.id);
              ext_client::render::render_skill_slot(slot_id_str, sk, ImVec2(28.0f, 28.0f), true);

              // Col 1: Skill Name
              ImGui::TableSetColumnIndex(1);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
              ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.0f), "%s", sk.name.c_str());
              if (ImGui::IsItemHovered()) {
                ext_client::render::render_skill_tooltip(sk);
              }

              // Col 2: Type badge
              ImGui::TableSetColumnIndex(2);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
              if (sk.is_active) {
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Active");
              } else {
                ImGui::TextColored(ImVec4(0.75f, 0.50f, 1.0f, 1.0f), "Passive");
              }

              // Col 3: MP Cost
              ImGui::TableSetColumnIndex(3);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
              if (sk.mp_cost > 0) {
                ImGui::Text("%u", sk.mp_cost);
              } else {
                ImGui::TextDisabled("-");
              }

              // Col 4: Cast / CD
              ImGui::TableSetColumnIndex(4);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
              char time_buf[32];
              std::snprintf(time_buf, sizeof(time_buf), "%.1fs / %.1fs",
                            sk.cast_time_sec,
                            sk.cooldown_sec);
              ImGui::TextDisabled("%s", time_buf);

              // Col 5: Cooldown status
              ImGui::TableSetColumnIndex(5);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
              const int cd_ms = c_skill_manager::get_cooldown_remaining_ms(sk.id);
              if (cd_ms > 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.2f, 1.0f), "%.1fs", cd_ms / 1000.0f);
              } else {
                ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.35f, 1.0f), "READY");
              }
            }

            ImGui::EndTable();
          }
        }

        // =====================================================================
        // 5. NATIVE FOOTER (Skill Point + Mastery Level Total)
        // =====================================================================
        ImGui::Spacing();
        const ImVec2 foot_p0 = ImGui::GetCursorScreenPos();
        const float foot_w = ImGui::GetContentRegionAvail().x;
        const float foot_h = 28.0f;
        const ImVec2 foot_p1(foot_p0.x + foot_w, foot_p0.y + foot_h);
        ImDrawList* foot_dl = ImGui::GetWindowDrawList();

        foot_dl->AddRectFilled(foot_p0, foot_p1, IM_COL32(10, 14, 22, 230), 4.0f);
        foot_dl->AddRect(foot_p0, foot_p1, IM_COL32(32, 45, 65, 180), 4.0f);

        ImGui::SetCursorScreenPos(ImVec2(foot_p0.x + 10.0f, foot_p0.y + 5.0f));

        // Skill point label & value in Gold
        ImGui::TextColored(ImVec4(0.85f, 0.72f, 0.38f, 1.0f), "Skill point :");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.92f, 0.50f, 1.0f), "%s", format_thousands(player_sp).c_str());

        // Mastery level total label & value in Cyan (Aligned to right)
        ImGui::SameLine(foot_p1.x - 220.0f);
        ImGui::TextColored(ImVec4(0.40f, 0.70f, 0.95f, 1.0f), "Mastery level total :");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.82f, 0.94f, 1.0f, 1.0f), "%u / %u", total_mastery_lvl, max_mastery_cap);

        ImGui::SetCursorScreenPos(ImVec2(foot_p0.x, foot_p1.y + 4.0f));
        ImGui::EndTabItem();
      }

      // ---------------------------------------------------------------------
      // TAB 4: Target Player
      // ---------------------------------------------------------------------
      if (target_player && target_player != local_player) {
        const ImGuiTabItemFlags tab4_flags = (s_request_tab == 4) ? ImGuiTabItemFlags_SetSelected : 0;
        if (ImGui::BeginTabItem("Target Player", nullptr, tab4_flags)) {
          current_main_tab = 4;
        const wchar_t* raw_tname = target_player->user_name();
        const auto t_name = (raw_tname && ext_client::utils::memory::is_readable_ptr(raw_tname))
                              ? ext_client::utils::string::to_utf8(raw_tname)
                              : std::string{};
        const wchar_t* raw_gname = target_player->user_guild_name();
        const auto g_name = (raw_gname && ext_client::utils::memory::is_readable_ptr(raw_gname))
                              ? ext_client::utils::string::to_utf8(raw_gname)
                              : std::string{};
        const auto target_lbl = ext_client::sdk::ui::get_string_utf8(L"UIIT_CTL_PARTYMATCH_PSEARCH_OBJECTCOMBAT", "Target");
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s (Lv.%u)", t_name.empty() ? target_lbl.c_str() : t_name.c_str(), target_player->get_level());
        if (!g_name.empty()) {
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "<%s>", g_name.c_str());
        }
        if (target_player->is_in_job_mode()) {
          const auto j_name = target_player->job_name_loc();
          if (!j_name.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.4f, 1.0f), "[%s Lv.%u]", j_name.c_str(), target_player->job_level());
          }
        }
        if (target_player->is_in_pvp()) {
          const auto cape_name = target_player->pvp_cape_name_loc();
          if (!cape_name.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[%s]", cape_name.c_str());
          }
        }
        ImGui::Separator();

        const auto col_slot = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_ARMOR_POSITION", "Slot");
        const auto col_item = ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_ITEM_TYPE", "Item");
        const auto empty_str = "<" + ext_client::sdk::ui::get_string_utf8(L"UIIT_STT_SOCKET_EMPTY_SLOT", "Empty") + ">";

        if (ImGui::BeginTable("TargetEquipGrid", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
          ImGui::TableSetupColumn(col_slot.c_str(), ImGuiTableColumnFlags_WidthFixed, 80.0f);
          ImGui::TableSetupColumn(col_item.c_str(), ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableSetupColumn("Plus", ImGuiTableColumnFlags_WidthFixed, 50.0f);
          ImGui::TableHeadersRow();

          for (std::uint8_t vs = 0; vs < 9; ++vs) {
            const auto eq = target_player->get_visual_equip(vs);
            auto* ref = eq.ref_item_id ? cref_obj_item::get(eq.ref_item_id) : nullptr;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
            ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "%s", cic_user::visual_slot_name_loc(vs).c_str());

            ImGui::TableSetColumnIndex(1);
            char id_str[32];
            std::snprintf(id_str, sizeof(id_str), "##tgt_slot_%u", vs);

            if (ref) {
              const auto title = ref->name();
              const auto display_title = !title.empty() ? title : ext_client::utils::string::to_utf8(ref->code_name().c_str());
              auto data = ext_client::sdk::game::extract_tooltip_data_from_ref(ref, eq.plus_opt);
              if (data.title.empty()) data.title = display_title;

              ext_client::render::render_item_slot_from_data(id_str, data, ImVec2(24.0f, 24.0f), true);
              ImGui::SameLine(0.0f, 6.0f);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

              ImVec4 text_col = ref->is_sox() ? ImVec4(1.0f, 0.82f, 0.15f, 1.0f) :
                                ((eq.plus_opt > 0) ? ImVec4(1.0f, 0.85f, 0.25f, 1.0f) : ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
              ImGui::PushStyleColor(ImGuiCol_Text, text_col);
              char sel_id[36];
              std::snprintf(sel_id, sizeof(sel_id), "##tgt_sel_%u", vs);
              ImGui::Selectable((display_title + sel_id).c_str(), false, ImGuiSelectableFlags_None);
              ImGui::PopStyleColor();
              if (ImGui::IsItemHovered()) {
                ext_client::render::render_item_tooltip_from_data(data);
              }
            } else {
              ext_client::render::render_item_slot(id_str, nullptr, ImVec2(24.0f, 24.0f), false);
              ImGui::SameLine(0.0f, 6.0f);
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
              ImGui::TextDisabled("%s", empty_str.c_str());
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
            if (ref && eq.plus_opt > 0) {
              ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.25f, 1.0f), "+%u", eq.plus_opt);
            } else if (ref) {
              ImGui::TextDisabled("+0");
            } else {
              ImGui::TextDisabled("-");
            }
          }
          ImGui::EndTable();
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Note: Silkroad server network protocol only sends visual equipment and basic identity (Level, HP, Guild, Job) for other players. Combat stats (Attack/Defense/Balance) are computed locally by each client.");
        ImGui::EndTabItem();
      }
    }

    ImGui::EndTabBar();
    s_request_tab = -1;
  }
}

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
  }

  auto handle_menu_overlay(menu_draw_context& /*ctx*/) -> void {
    render_target_overlay();
    render_equipment_inspector();
  }

  auto handle_menu(menu_builder& ui) -> void {
    ui.section("Target HUD & Window Enhancements");

    auto& tw = ext_client::core::config::data().target_window;
    ui.checkbox("Enable Target Window Enhancements", &tw.enabled);
    ui.checkbox("Display HP Percent on Gauge", &tw.show_hp_percent);
    ui.checkbox("Display Exact 3D Distance in Meters", &tw.show_distance);
    ui.checkbox("Display Target-of-Target (ToT)", &tw.show_tot);
    ui.checkbox("Display Target Details Sub-Panel (Archetype/Level/Owner)", &tw.show_target_details);

    ui.spacing();
    ui.section("Equipment Inspector");
    ui.checkbox("Show Equipment Inspector Window", &tw.show_equipment_inspector);
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("target_hp", "Target HUD");

    ADD_EVENT(EVENT_ON_MENU, handle_menu);
    ADD_EVENT(EVENT_ON_SET_CHILD_PROCESS, handle_set_child_process);
    ADD_EVENT(EVENT_ON_CHAR_SELECT_ENTER, handle_char_select_enter);
    ADD_EVENT(EVENT_ON_PACKET, handle_packet);
    ADD_EVENT(EVENT_ON_MENU_OVERLAY, handle_menu_overlay);
  }

  PLUGIN_INIT(initialize);
} // namespace ext_client::plugins::target_window
