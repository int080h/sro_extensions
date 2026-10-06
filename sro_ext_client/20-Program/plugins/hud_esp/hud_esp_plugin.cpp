#include "pch.hpp"
#include "plugins/hud_esp/hud_esp_plugin.hpp"

#include "core/config.hpp"
#include "core/event_bus.hpp"
#include "core/plugin_manager.hpp"
#include "render/menu_builder.hpp"
#include "sdk/game/ccompound_obj.hpp"
#include "sdk/game/ccontroler.hpp"
#include "sdk/game/centity_manager.hpp"
#include "sdk/game/ci_charactor.hpp"
#include "sdk/game/cic_user.hpp"
#include "sdk/game/cref_obj_item.hpp"
#include "sdk/game/crt_bone.hpp"
#include "sdk/game/crt_skeleton.hpp"
#include "render/render_overlay_3d.hpp"
#include "sdk/ui/cif_target_window.hpp"
#include "sdk/runtime/gfx_runtime.hpp"
#include "sdk/runtime/rtti.hpp"
#include "sdk/types/s_position.hpp"
#include "utils/memory.hpp"
#include "utils/offsets.hpp"
#include "utils/string.hpp"

#include <Windows.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using ext_client::render::menu::menu_builder;
using ext_client::utils::memory::is_game_ptr;
using ext_client::utils::memory::is_readable_ptr;
using namespace ext_client::core::event;

namespace ext_client::plugins::hud_esp {

namespace {

  auto draw_outlined_text(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text) -> void {
    if (!dl || !text) {
      return;
    }
    const ImU32 shadow = IM_COL32(0, 0, 0, 220);
    dl->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), shadow, text);
    dl->AddText(ImVec2(pos.x - 1.0f, pos.y + 1.0f), shadow, text);
    dl->AddText(ImVec2(pos.x + 1.0f, pos.y - 1.0f), shadow, text);
    dl->AddText(ImVec2(pos.x - 1.0f, pos.y - 1.0f), shadow, text);
    dl->AddText(pos, col, text);
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

    // 3. Fallback: Regional position at CICharactor+0x7C converted to continuous world coordinates
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

  auto get_rarity_color(const ci_charactor* ent) -> ImU32 {
    if (!ent || !is_game_ptr(ent)) {
      return IM_COL32(255, 255, 255, 255);
    }
    if (ent->is_player()) {
      return IM_COL32(100, 190, 255, 255); // Sky Blue
    }
    if (ent->is_pet()) {
      return IM_COL32(100, 240, 170, 255); // Mint Green / Companion
    }
    if (ent->is_npc()) {
      return IM_COL32(235, 210, 140, 240); // Warm Gold / Friendly NPC
    }
    if (ent->is_unique()) {
      return IM_COL32(255, 215, 0, 255); // Rich Gold
    }
    if (ent->get_rarity() == 5 || ent->get_rarity() == 6) { // Titan / Elite
      return IM_COL32(180, 100, 255, 255); // Violet Purple
    }
    if (ent->is_giant()) {
      return IM_COL32(255, 130, 40, 255); // Orange
    }
    if (ent->is_champion()) {
      return IM_COL32(255, 225, 70, 255); // Yellow
    }
    if (ent->is_party_mob()) {
      return IM_COL32(80, 205, 255, 255); // Cyan
    }
    return IM_COL32(240, 90, 90, 240); // Soft Crimson for hostile monsters
  }

  auto draw_mini_hp_bar(ImDrawList* dl, ImVec2 pos, float width, float height, float pct) -> void {
    if (!dl) {
      return;
    }
    const float clamped_pct = std::clamp(pct, 0.0f, 1.0f);
    const ImVec2 bg_min = pos;
    const ImVec2 bg_max(pos.x + width, pos.y + height);

    // Background
    dl->AddRectFilled(bg_min, bg_max, IM_COL32(15, 15, 15, 210), 2.0f);

    // Dynamic bar color: Green -> Yellow -> Red
    ImU32 bar_col;
    if (clamped_pct > 0.50f) {
      bar_col = IM_COL32(50, 220, 80, 230);
    } else if (clamped_pct > 0.25f) {
      bar_col = IM_COL32(240, 180, 40, 230);
    } else {
      bar_col = IM_COL32(230, 45, 45, 230);
    }

    // Filled portion
    if (clamped_pct > 0.0f) {
      const ImVec2 fill_max(pos.x + (width * clamped_pct), pos.y + height);
      dl->AddRectFilled(bg_min, fill_max, bar_col, 2.0f);
    }

    // Border
    dl->AddRect(bg_min, bg_max, IM_COL32(0, 0, 0, 240), 2.0f, 0, 1.0f);
  }

  auto draw_full_skeleton(ImDrawList* dl, crt_skeleton* skel, const D3DMATRIX* world_mat, ImU32 color, float thickness) -> void {
    if (!dl || !skel || !world_mat) {
      return;
    }
    skel->for_each_bone([&](crt_bone* b) {
      if (!b || b->is_dummy()) {
        return;
      }
      auto* p = b->parent();
      if (p && !p->is_dummy()) {
        const vector3f p1 = b->world_position(world_mat);
        const vector3f p2 = p->world_position(world_mat);
        ImVec2 s1{}, s2{};
        if (ext_client::render::overlay_3d::project(p1, s1) && ext_client::render::overlay_3d::project(p2, s2)) {
          dl->AddLine(s1, s2, color, thickness);
          dl->AddCircleFilled(s1, 1.8f, color);
        }
      }
    });
  }

  auto draw_biped_spine(ImDrawList* dl, crt_skeleton* skel, const D3DMATRIX* world_mat, ImU32 color, float thickness) -> void {
    if (!dl || !skel || !world_mat) {
      return;
    }

    struct biped_rig {
      crt_bone* head = nullptr;
      crt_bone* neck = nullptr;
      crt_bone* spine1 = nullptr;
      crt_bone* spine = nullptr;
      crt_bone* pelvis = nullptr;

      crt_bone* l_clavicle = nullptr;
      crt_bone* l_upperarm = nullptr;
      crt_bone* l_forearm = nullptr;
      crt_bone* l_hand = nullptr;

      crt_bone* r_clavicle = nullptr;
      crt_bone* r_upperarm = nullptr;
      crt_bone* r_forearm = nullptr;
      crt_bone* r_hand = nullptr;

      crt_bone* l_thigh = nullptr;
      crt_bone* l_calf = nullptr;
      crt_bone* l_foot = nullptr;

      crt_bone* r_thigh = nullptr;
      crt_bone* r_calf = nullptr;
      crt_bone* r_foot = nullptr;
    } rig{};

    // Single pass to collect all relevant joints with 0 repeated linear scans
    skel->for_each_bone([&](crt_bone* b) {
      if (!b || b->is_dummy()) {
        return;
      }

      const char* name = b->name();
      if (!name || !name[0]) {
        return;
      }

      const char* p = name;
      if (_strnicmp(p, "Bip01 ", 6) == 0 || _strnicmp(p, "Bip02 ", 6) == 0) {
        p += 6;
      }

      if (_stricmp(p, "Head") == 0) rig.head = b;
      else if (_stricmp(p, "Neck") == 0) rig.neck = b;
      else if (_stricmp(p, "Spine1") == 0) rig.spine1 = b;
      else if (_stricmp(p, "Spine") == 0) rig.spine = b;
      else if (_stricmp(p, "Pelvis") == 0) rig.pelvis = b;
      else if (_stricmp(p, "L Clavicle") == 0) rig.l_clavicle = b;
      else if (_stricmp(p, "L UpperArm") == 0) rig.l_upperarm = b;
      else if (_stricmp(p, "L Forearm") == 0) rig.l_forearm = b;
      else if (_stricmp(p, "L Hand") == 0) rig.l_hand = b;
      else if (_stricmp(p, "R Clavicle") == 0) rig.r_clavicle = b;
      else if (_stricmp(p, "R UpperArm") == 0) rig.r_upperarm = b;
      else if (_stricmp(p, "R Forearm") == 0) rig.r_forearm = b;
      else if (_stricmp(p, "R Hand") == 0) rig.r_hand = b;
      else if (_stricmp(p, "L Thigh") == 0) rig.l_thigh = b;
      else if (_stricmp(p, "L Calf") == 0) rig.l_calf = b;
      else if (_stricmp(p, "L Foot") == 0) rig.l_foot = b;
      else if (_stricmp(p, "R Thigh") == 0) rig.r_thigh = b;
      else if (_stricmp(p, "R Calf") == 0) rig.r_calf = b;
      else if (_stricmp(p, "R Foot") == 0) rig.r_foot = b;
    });

    const auto draw_seg = [&](crt_bone* a, crt_bone* b) {
      if (!a || !b) {
        return;
      }
      const vector3f p1 = a->world_position(world_mat);
      const vector3f p2 = b->world_position(world_mat);
      ImVec2 s1{}, s2{};
      if (ext_client::render::overlay_3d::project(p1, s1) && ext_client::render::overlay_3d::project(p2, s2)) {
        dl->AddLine(s1, s2, color, thickness);
        dl->AddCircleFilled(s1, 2.0f, color);
        dl->AddCircleFilled(s2, 2.0f, color);
      }
    };

    // If human biped structure detected, draw the clean essential rig
    if (rig.pelvis || rig.spine || rig.head) {
      // Main Spine
      draw_seg(rig.head, rig.neck);
      draw_seg(rig.neck, rig.spine1);
      draw_seg(rig.spine1, rig.spine);
      draw_seg(rig.spine, rig.pelvis);

      // Left Arm
      draw_seg(rig.spine1, rig.l_clavicle);
      draw_seg(rig.l_clavicle, rig.l_upperarm);
      draw_seg(rig.l_upperarm, rig.l_forearm);
      draw_seg(rig.l_forearm, rig.l_hand);

      // Right Arm
      draw_seg(rig.spine1, rig.r_clavicle);
      draw_seg(rig.r_clavicle, rig.r_upperarm);
      draw_seg(rig.r_upperarm, rig.r_forearm);
      draw_seg(rig.r_forearm, rig.r_hand);

      // Left Leg
      draw_seg(rig.pelvis, rig.l_thigh);
      draw_seg(rig.l_thigh, rig.l_calf);
      draw_seg(rig.l_calf, rig.l_foot);

      // Right Leg
      draw_seg(rig.pelvis, rig.r_thigh);
      draw_seg(rig.r_thigh, rig.r_calf);
      draw_seg(rig.r_calf, rig.r_foot);
    } else {
      // Non-biped creatures (monsters, quadrupeds, mounts):
      // Draw parent-child joint connections skipping dummy sockets
      draw_full_skeleton(dl, skel, world_mat, color, thickness);
    }
  }

  auto render_hud_overlay() -> void {
    auto& cfg = ext_client::core::config::data().hud_esp;
    if (!cfg.enabled) {
      return;
    }

    // Local player must exist and be spawned into the 3D world
    auto* local_player = cic_user::get_local_player();
    if (!local_player || !is_game_ptr(local_player)) {
      return;
    }

    // Guard against non-world screens (Title, VersionCheck, CharacterSelect, Creation, Restart, Quit)
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
        return;
      }
    }

    vector3f local_pos{};
    if (!get_entity_world_pos(local_player, local_pos)) {
      return;
    }

    auto* draw_list = ImGui::GetForegroundDrawList();
    if (!draw_list) {
      return;
    }

    const auto& io = ImGui::GetIO();
    const float anim_progress = static_cast<float>((GetTickCount() % 2400)) / 2400.0f;

    // Optional local player ground ring
    if (cfg.show_local_player_ring) {
      ext_client::render::overlay_3d::draw_ground_target_indicator(
        draw_list,
        local_pos,
        22.0f,
        IM_COL32(0, 220, 255, 230),
        anim_progress
      );
    }

    // Track any detected uniques for boss alert banner and directional radar
    std::string unique_alert_name;
    float unique_alert_dist = 9999.0f;
    vector3f unique_alert_pos{};
    ci_charactor* unique_alert_ent = nullptr;
    int entities_rendered = 0;

    // -----------------------------------------------------------------------
    // 1. Target Visuals (Indicator Ring & Snapline)
    // -----------------------------------------------------------------------
    ci_charactor* target_ent = nullptr;
    auto* tw = cif_target_window::active();
    if (tw && is_game_ptr(tw) && cif_target_window::is_live_target_panel(tw)) {
      const auto target_id = tw->target_slot_id();
      if (target_id > 0) {
        target_ent = centity_manager::resolve_by_uid_or_slot(target_id);
      }
    }

    if (target_ent && is_readable_ptr(target_ent) && target_ent != local_player) {
      vector3f target_pos{};
      if (get_entity_world_pos(target_ent, target_pos)) {
        const ImU32 target_col = get_rarity_color(target_ent);

        // Ground indicator
        if (cfg.show_target_indicator) {
          ext_client::render::overlay_3d::draw_ground_target_indicator(
            draw_list,
            target_pos,
            cfg.target_ring_radius,
            target_col,
            anim_progress
          );
        }

        // Snapline from screen bottom
        if (cfg.show_target_snapline) {
          ImVec2 screen_feet{};
          if (ext_client::render::overlay_3d::project(target_pos, screen_feet)) {
            const ImVec2 screen_bottom(io.DisplaySize.x * 0.5f, io.DisplaySize.y);
            const ImU32 line_col = (target_col & 0x00FFFFFF) | (0xB0 << 24);
            draw_list->AddLine(screen_bottom, screen_feet, line_col, 1.8f);
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 2. World Entity Iteration & Overhead ESP
    // -----------------------------------------------------------------------
    centity_manager::for_each_in_world([&](ci_charactor* ent) {
      if (ent == local_player && !cfg.show_self_esp) {
        return;
      }

      vector3f ent_pos{};
      if (!get_entity_world_pos(ent, ent_pos)) {
        return;
      }

      // 3D Distance in meters (Silkroad standard: 10 world units = 1 meter)
      const float dx = ent_pos.x - local_pos.x;
      const float dy = ent_pos.y - local_pos.y;
      const float dz = ent_pos.z - local_pos.z;
      const float dist_units = std::sqrt(dx * dx + dy * dy + dz * dz);
      const float dist_meters = dist_units * 0.1f;

      if (dist_meters > static_cast<float>(cfg.max_distance_meters)) {
        return;
      }

      const bool is_player = ent->is_player();
      const bool is_npc = ent->is_npc();
      const bool is_pet = ent->is_pet();
      const bool is_monster = ent->is_monster();
      const bool is_unique = ent->is_unique();
      const bool is_giant = ent->is_giant();
      const bool is_champion = ent->is_champion();
      const auto rarity = ent->get_rarity();

      // Check unique alert (monsters only)
      if (is_monster && is_unique && (cfg.show_unique_alert || cfg.show_boss_radar || cfg.show_radar_compass_widget)) {
        if (dist_meters < unique_alert_dist) {
          unique_alert_dist = dist_meters;
          unique_alert_pos = ent_pos;
          unique_alert_ent = ent;
          const auto* raw_name = ent->get_display_name();
          if (raw_name && is_readable_ptr(raw_name)) {
            unique_alert_name = ext_client::utils::string::to_utf8(raw_name);
          } else {
            unique_alert_name = "Unique Boss";
          }
        }
      }

      // Filter settings
      if (is_player) {
        if (!cfg.show_player_esp) {
          return;
        }
      } else if (is_pet) {
        if (!cfg.show_pet_esp) {
          return;
        }
      } else if (is_npc) {
        if (!cfg.show_npc_esp) {
          return;
        }
      } else if (is_monster) {
        if (!cfg.show_monster_esp) {
          return;
        }
        // Rarity filter for monsters: 0=all, 1=champion+, 2=giant+, 3=unique only
        if (cfg.min_rarity == 1 && rarity == 0 && !ent->is_party_mob()) {
          return;
        }
        if (cfg.min_rarity == 2 && !is_giant && !is_unique && rarity < 4) {
          return;
        }
        if (cfg.min_rarity == 3 && !is_unique) {
          return;
        }
      } else {
        if (!cfg.show_monster_esp) {
          return;
        }
      }

      // Calculate overhead billboard position via exact model height & anchor
      vector3f overhead_pos = ent->get_overhead_3d_position();
      if (overhead_pos.x == 0.0f && overhead_pos.y == 0.0f && overhead_pos.z == 0.0f) {
        const float head_elevation = is_unique ? 36.0f : (is_giant ? 30.0f : 18.0f);
        overhead_pos = vector3f(ent_pos.x, ent_pos.y + head_elevation, ent_pos.z);
      }

      ImVec2 screen_head{};
      if (!ext_client::render::overlay_3d::project(overhead_pos, screen_head)) {
        return;
      }

      ++entities_rendered;

      // Entity name & formatting
      std::string name_utf8;
      const auto* raw_name = ent->get_display_name();
      if (raw_name && is_readable_ptr(raw_name) && raw_name[0] != L'\0') {
        name_utf8 = ext_client::utils::string::to_utf8(raw_name);
      }
      if (name_utf8.empty()) {
        if (is_player) {
          name_utf8 = "Player";
        } else if (is_pet) {
          name_utf8 = "Pet";
        } else if (is_npc) {
          name_utf8 = ent->is_guard() ? "Town Guard" : "NPC";
        } else {
          name_utf8 = "Monster";
        }
      } else if (is_pet) {
        const auto scroll_pos = name_utf8.find(" Summon Scroll");
        if (scroll_pos != std::string::npos) {
          name_utf8.erase(scroll_pos);
        }
      }

      const std::uint32_t level = ent->get_level();
      char tag_buf[128]{};
      if (ent == local_player) {
        if (level > 0) {
          std::snprintf(tag_buf, sizeof(tag_buf), "[Lv.%u] %s (Self)", level, name_utf8.c_str());
        } else {
          std::snprintf(tag_buf, sizeof(tag_buf), "%s (Self)", name_utf8.c_str());
        }
      } else {
        if (level > 0) {
          std::snprintf(tag_buf, sizeof(tag_buf), "[Lv.%u] %s (%.0fm)", level, name_utf8.c_str(), dist_meters);
        } else {
          std::snprintf(tag_buf, sizeof(tag_buf), "%s (%.0fm)", name_utf8.c_str(), dist_meters);
        }
      }

      const float base_offset_y = cfg.overhead_offset_y + (is_player ? 14.0f : 0.0f);
      const ImVec2 text_size = ImGui::CalcTextSize(tag_buf);
      const ImVec2 tag_pos(screen_head.x - (text_size.x * 0.5f), screen_head.y - base_offset_y - text_size.y);
      const ImU32 entity_col = get_rarity_color(ent);

      draw_outlined_text(draw_list, tag_pos, entity_col, tag_buf);

      // Identity / Rarity badge label
      if (is_player) {
        const auto* user = static_cast<const cic_user*>(ent);
        const auto* raw_guild = user->user_guild_name();
        if (raw_guild && is_readable_ptr(raw_guild) && raw_guild[0] != L'\0') {
          const auto guild_utf8 = ext_client::utils::string::to_utf8(raw_guild);
          if (!guild_utf8.empty()) {
            char guild_buf[96]{};
            std::snprintf(guild_buf, sizeof(guild_buf), "<%s>", guild_utf8.c_str());
            const ImVec2 g_size = ImGui::CalcTextSize(guild_buf);
            const ImVec2 g_pos(screen_head.x - (g_size.x * 0.5f), tag_pos.y - g_size.y - 1.0f);
            draw_outlined_text(draw_list, g_pos, IM_COL32(140, 220, 255, 230), guild_buf);
          }
        }
      } else if (is_pet) {
        char badge_buf[64]{};
        const auto* arch = ent->pet_archetype_name();
        const auto owner_uid = ent->get_owner_unique_id();
        auto* owner_ent = owner_uid > 0 ? centity_manager::resolve_by_uid_or_slot(owner_uid) : nullptr;
        const auto* owner_name = owner_ent ? owner_ent->get_display_name() : nullptr;

        if (arch && arch[0] != '\0') {
          if (owner_name && ext_client::utils::memory::is_valid_ptr(owner_name) && owner_name[0] != L'\0') {
            std::snprintf(badge_buf, sizeof(badge_buf), "<%s Pet of %s>", arch, ext_client::utils::string::to_utf8(owner_name).c_str());
          } else {
            std::snprintf(badge_buf, sizeof(badge_buf), "<%s Pet>", arch);
          }
        } else {
          if (owner_name && ext_client::utils::memory::is_valid_ptr(owner_name) && owner_name[0] != L'\0') {
            std::snprintf(badge_buf, sizeof(badge_buf), "<Pet of %s>", ext_client::utils::string::to_utf8(owner_name).c_str());
          } else {
            std::snprintf(badge_buf, sizeof(badge_buf), "<Pet>");
          }
        }
        const ImVec2 badge_size = ImGui::CalcTextSize(badge_buf);
        const ImVec2 badge_pos(screen_head.x - (badge_size.x * 0.5f), tag_pos.y - badge_size.y - 1.0f);
        draw_outlined_text(draw_list, badge_pos, entity_col, badge_buf);
      } else if (is_npc) {
        char badge_buf[32]{};
        std::snprintf(badge_buf, sizeof(badge_buf), ent->is_guard() ? "<Guard>" : "<NPC>");
        const ImVec2 badge_size = ImGui::CalcTextSize(badge_buf);
        const ImVec2 badge_pos(screen_head.x - (badge_size.x * 0.5f), tag_pos.y - badge_size.y - 1.0f);
        draw_outlined_text(draw_list, badge_pos, entity_col, badge_buf);
      } else if (is_monster && (rarity > 0 || ent->is_party_mob())) {
        const char* r_name = ent->rarity_name();
        if (r_name && r_name[0] != '\0') {
          char badge_buf[32]{};
          std::snprintf(badge_buf, sizeof(badge_buf), "<%s>", r_name);
          const ImVec2 badge_size = ImGui::CalcTextSize(badge_buf);
          const ImVec2 badge_pos(screen_head.x - (badge_size.x * 0.5f), tag_pos.y - badge_size.y - 1.0f);
          draw_outlined_text(draw_list, badge_pos, entity_col, badge_buf);
        }
      }

        // Mini HP Bar: placed cleanly directly below the ESP text tag so it never collides with in-game native name/bar
        if (cfg.show_monster_hp_bar && (is_monster || is_pet)) {
          const auto max_hp = ent->get_max_hp();
          const auto hp = ent->get_hp();
          if (max_hp > 0) {
            const float hp_pct = static_cast<float>(hp) / static_cast<float>(max_hp);
            constexpr float k_bar_width = 44.0f;
            constexpr float k_bar_height = 4.0f;
            const ImVec2 bar_pos(screen_head.x - (k_bar_width * 0.5f), tag_pos.y + text_size.y + 2.0f);
            draw_mini_hp_bar(draw_list, bar_pos, k_bar_width, k_bar_height, hp_pct);
          }
        }

        // -------------------------------------------------------------------
        // 3D Meshbox & Skeleton Wireframe
        // -------------------------------------------------------------------
        auto* cobj = ent->get_compound_obj();
        if (cobj && is_game_ptr(cobj)) {
          const auto* world_mat = cobj->world_matrix();
          if (world_mat && is_game_ptr(world_mat)) {
            // 1. 3D Mesh Box (Oriented Bounding Box)
            if (cfg.show_mesh_boxes) {
              vector3f min_pt{}, max_pt{};
              if (cobj->bounding_box(min_pt, max_pt)) {
                const ImU32 box_col = (entity_col & 0x00FFFFFF) | (0xB0 << 24);
                ext_client::render::overlay_3d::draw_mesh_box(
                  draw_list, world_mat, min_pt, max_pt, box_col, cfg.mesh_box_thickness);
              }
            }

            // 2. 3D Skeleton / Spine Wireframe
            if (cfg.show_skeleton) {
              auto* skel = cobj->skeleton();
              if (skel && is_game_ptr(skel)) {
                const ImU32 skel_col = (entity_col & 0x00FFFFFF) | (0xE0 << 24);
                if (cfg.show_spine_only) {
                  draw_biped_spine(draw_list, skel, world_mat, skel_col, cfg.skeleton_thickness);
                } else {
                  draw_full_skeleton(draw_list, skel, world_mat, skel_col, cfg.skeleton_thickness);
                }
              }
            }
          }
        }
      });

    // -----------------------------------------------------------------------
    // 2B. Ground Dropped Items (ESP)
    // -----------------------------------------------------------------------
    if (cfg.show_item_drop_esp) {
      centity_manager::for_each_all_entities([&](ci_charactor* obj) {
        if (!obj || !is_readable_ptr(obj)) {
          return;
        }
        const auto vt = *reinterpret_cast<const std::uintptr_t*>(obj);
        if (vt != 0x0104258Cu && vt != 0x0104257Cu) {
          return; // not a CIItem
        }

        const auto* pos_ptr = &ext_client::off::field_at<float>(obj, 0x90);
        if (!is_readable_ptr(pos_ptr) || !is_readable_ptr(pos_ptr + 2)) {
          return;
        }
        vector3f item_pos(pos_ptr[0], pos_ptr[1], pos_ptr[2]);
        if (!position_is_usable(item_pos)) {
          return;
        }

        const float dx = item_pos.x - local_pos.x;
        const float dy = item_pos.y - local_pos.y;
        const float dz = item_pos.z - local_pos.z;
        const float dist_m = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.1f;
        if (dist_m > static_cast<float>(cfg.max_distance_meters)) {
          return;
        }

        // Lift floating tag slightly above ground for visibility
        vector3f tag_world_pos(item_pos.x, item_pos.y + 4.0f, item_pos.z);
        ImVec2 screen_pos{};
        if (!ext_client::render::overlay_3d::project(tag_world_pos, screen_pos)) {
          return;
        }

        const auto ref_id = ext_client::off::field_at<std::uint32_t>(obj, 0x2D4);
        auto* ref = cref_obj_item::get(ref_id);
        if (!ref) {
          return;
        }

        std::string title = ref->name();
        if (title.empty()) {
          title = ext_client::utils::string::to_utf8(ref->code_name().c_str());
        }
        if (title.empty()) {
          return;
        }

        const bool is_sox = ref->is_sox();
        const bool is_gold = (ref->type_id2() == 3 && ref->type_id3() == 1);
        const bool is_elixir = (ref->code_name().find(L"_ARCHEMY_POTION") != std::wstring::npos ||
                                ref->code_name().find(L"_ARCHEMY_REINFORCE") != std::wstring::npos);
        const bool is_stone = (ref->code_name().find(L"_ARCHEMY_MAGICSTONE") != std::wstring::npos ||
                               ref->code_name().find(L"_ARCHEMY_ATTRSTONE") != std::wstring::npos);

        ImU32 item_col = IM_COL32(230, 230, 230, 230);
        char item_buf[128]{};
        if (is_sox) {
          item_col = IM_COL32(255, 215, 30, 255); // Rich Gold
          std::snprintf(item_buf, sizeof(item_buf), "* [SoX] %s (%.0fm)", title.c_str(), dist_m);
        } else if (is_elixir) {
          item_col = IM_COL32(80, 245, 150, 240); // Mint Green
          std::snprintf(item_buf, sizeof(item_buf), "+ %s (%.0fm)", title.c_str(), dist_m);
        } else if (is_stone) {
          item_col = IM_COL32(210, 140, 255, 240); // Violet
          std::snprintf(item_buf, sizeof(item_buf), "* %s (%.0fm)", title.c_str(), dist_m);
        } else if (is_gold) {
          item_col = IM_COL32(255, 235, 90, 240); // Gold Yellow
          std::snprintf(item_buf, sizeof(item_buf), "%s (%.0fm)", title.c_str(), dist_m);
        } else {
          std::snprintf(item_buf, sizeof(item_buf), "%s (%.0fm)", title.c_str(), dist_m);
        }

        const ImVec2 text_sz = ImGui::CalcTextSize(item_buf);
        const ImVec2 draw_pos(screen_pos.x - text_sz.x * 0.5f, screen_pos.y - text_sz.y * 0.5f);
        draw_outlined_text(draw_list, draw_pos, item_col, item_buf);
      });
    }

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------
    // 3. Unique Boss Alert Banner & Directional Radar (Top Center & Border)
    // -----------------------------------------------------------------------
    if ((cfg.show_unique_alert || cfg.show_boss_radar || cfg.show_radar_compass_widget) && !unique_alert_name.empty()) {
      const float pulse = 0.75f + 0.25f * std::sin(anim_progress * 6.283185f);

      // Top banner
      if (cfg.show_unique_alert) {
        char alert_msg[128]{};
        std::snprintf(alert_msg, sizeof(alert_msg), " UNIQUE ENCOUNTER: [%s] (%.0fm) ",
                      unique_alert_name.c_str(), unique_alert_dist);

        const ImVec2 msg_size = ImGui::CalcTextSize(alert_msg);
        constexpr float pad_x = 18.0f;
        constexpr float pad_y = 7.0f;
        const float banner_w = msg_size.x + (pad_x * 2.0f);
        const float banner_h = msg_size.y + (pad_y * 2.0f);
        const float banner_x = (io.DisplaySize.x - banner_w) * 0.5f;
        constexpr float banner_y = 35.0f;

        const ImVec2 b_min(banner_x, banner_y);
        const ImVec2 b_max(banner_x + banner_w, banner_y + banner_h);

        const ImU32 bg_col = IM_COL32(20, 15, 10, static_cast<int>(230 * pulse));
        const ImU32 border_col = IM_COL32(255, 200, 30, static_cast<int>(255 * pulse));

        draw_list->AddRectFilled(b_min, b_max, bg_col, 6.0f);
        draw_list->AddRect(b_min, b_max, border_col, 6.0f, 0, 2.0f);

        const ImVec2 text_pos(banner_x + pad_x, banner_y + pad_y);
        draw_outlined_text(draw_list, text_pos, IM_COL32(255, 225, 60, 255), alert_msg);
      }

      // Directional Radar Compass & Screen-Edge Pointer
      if (cfg.show_boss_radar && position_is_usable(unique_alert_pos)) {
        ImVec2 screen_pos{};
        float depth = 0.0f;
        const bool in_front = ext_client::render::overlay_3d::project_with_depth(unique_alert_pos, screen_pos, depth);

        constexpr float margin_x = 55.0f;
        constexpr float margin_y = 55.0f;
        const ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

        const bool on_screen = in_front &&
          screen_pos.x >= margin_x && screen_pos.x <= (io.DisplaySize.x - margin_x) &&
          screen_pos.y >= margin_y && screen_pos.y <= (io.DisplaySize.y - margin_y);

        if (on_screen) {
          // On-screen: Draw prominent golden overhead diamond beacon
          const ImVec2 beacon_center(screen_pos.x, screen_pos.y - 28.0f);
          constexpr float diamond_size = 9.0f;
          const ImVec2 d_top(beacon_center.x, beacon_center.y - diamond_size);
          const ImVec2 d_bottom(beacon_center.x, beacon_center.y + diamond_size);
          const ImVec2 d_left(beacon_center.x - diamond_size, beacon_center.y);
          const ImVec2 d_right(beacon_center.x + diamond_size, beacon_center.y);

          draw_list->AddQuadFilled(d_top, d_right, d_bottom, d_left, IM_COL32(255, 215, 30, static_cast<int>(240 * pulse)));
          draw_list->AddQuad(d_top, d_right, d_bottom, d_left, IM_COL32(255, 255, 180, 255), 2.0f);

          char on_screen_tag[96]{};
          std::snprintf(on_screen_tag, sizeof(on_screen_tag), "[%s - %.0fm]", unique_alert_name.c_str(), unique_alert_dist);
          const ImVec2 t_sz = ImGui::CalcTextSize(on_screen_tag);
          const ImVec2 t_pos(beacon_center.x - t_sz.x * 0.5f, beacon_center.y - diamond_size - t_sz.y - 3.0f);
          draw_outlined_text(draw_list, t_pos, IM_COL32(255, 230, 70, 255), on_screen_tag);
        } else {
          // Off-screen or behind camera: Screen-edge clamped compass arrow
          ImVec2 dir{};
          if (!in_front) {
            dir.x = -(screen_pos.x - center.x);
            dir.y = -(screen_pos.y - center.y);
          } else {
            dir.x = screen_pos.x - center.x;
            dir.y = screen_pos.y - center.y;
          }

          float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
          if (len < 0.001f) {
            dir = ImVec2(0.0f, -1.0f);
            len = 1.0f;
          }
          dir.x /= len;
          dir.y /= len;

          // Ray-box intersection to clamp to screen border
          float t_min = 1e9f;
          if (dir.x > 0.0f) {
            t_min = std::min(t_min, (io.DisplaySize.x - margin_x - center.x) / dir.x);
          } else if (dir.x < 0.0f) {
            t_min = std::min(t_min, (margin_x - center.x) / dir.x);
          }
          if (dir.y > 0.0f) {
            t_min = std::min(t_min, (io.DisplaySize.y - margin_y - center.y) / dir.y);
          } else if (dir.y < 0.0f) {
            t_min = std::min(t_min, (margin_y - center.y) / dir.y);
          }

          const ImVec2 edge_pos(center.x + dir.x * t_min, center.y + dir.y * t_min);
          const ImVec2 perp(-dir.y, dir.x);

          constexpr float arrow_len = 16.0f;
          constexpr float arrow_half_w = 11.0f;

          const ImVec2 tip(edge_pos.x + dir.x * arrow_len, edge_pos.y + dir.y * arrow_len);
          const ImVec2 base_left(edge_pos.x - dir.x * (arrow_len * 0.5f) + perp.x * arrow_half_w,
                                 edge_pos.y - dir.y * (arrow_len * 0.5f) + perp.y * arrow_half_w);
          const ImVec2 base_right(edge_pos.x - dir.x * (arrow_len * 0.5f) - perp.x * arrow_half_w,
                                  edge_pos.y - dir.y * (arrow_len * 0.5f) - perp.y * arrow_half_w);

          const ImU32 arrow_fill = IM_COL32(255, 195, 30, static_cast<int>(240 * pulse));
          const ImU32 arrow_border = IM_COL32(255, 245, 160, 255);

          draw_list->AddTriangleFilled(tip, base_left, base_right, arrow_fill);
          draw_list->AddTriangle(tip, base_left, base_right, arrow_border, 2.0f);

          // Indicator badge inward from arrow
          char edge_tag[96]{};
          std::snprintf(edge_tag, sizeof(edge_tag), "★ %s (%.0fm)", unique_alert_name.c_str(), unique_alert_dist);
          const ImVec2 badge_sz = ImGui::CalcTextSize(edge_tag);
          const ImVec2 badge_pos(edge_pos.x - dir.x * (badge_sz.x * 0.5f + 25.0f) - badge_sz.x * 0.5f,
                                 edge_pos.y - dir.y * 30.0f - badge_sz.y * 0.5f);

          const ImVec2 bg_min(badge_pos.x - 5.0f, badge_pos.y - 3.0f);
          const ImVec2 bg_max(badge_pos.x + badge_sz.x + 5.0f, badge_pos.y + badge_sz.y + 3.0f);
          draw_list->AddRectFilled(bg_min, bg_max, IM_COL32(18, 14, 10, 215), 4.0f);
          draw_list->AddRect(bg_min, bg_max, IM_COL32(255, 200, 30, 220), 4.0f, 0, 1.2f);
          draw_outlined_text(draw_list, badge_pos, IM_COL32(255, 225, 70, 255), edge_tag);
        }
      }

      // Circular Mini-Radar Compass Widget (Upper Right)
      if (cfg.show_radar_compass_widget && position_is_usable(unique_alert_pos)) {
        const ImVec2 radar_center(io.DisplaySize.x - 85.0f, 165.0f);
        constexpr float radar_radius = 48.0f;
        constexpr float max_radar_range_m = 150.0f;

        draw_list->AddCircleFilled(radar_center, radar_radius, IM_COL32(12, 16, 24, 215), 32);
        draw_list->AddCircle(radar_center, radar_radius, IM_COL32(70, 90, 130, 220), 32, 1.5f);
        draw_list->AddCircle(radar_center, radar_radius * 0.66f, IM_COL32(50, 70, 100, 140), 24, 1.0f);
        draw_list->AddCircle(radar_center, radar_radius * 0.33f, IM_COL32(50, 70, 100, 140), 24, 1.0f);

        // Center player dot
        draw_list->AddCircleFilled(radar_center, 3.5f, IM_COL32(255, 255, 255, 255));

        // Camera heading vector
        const vector3f cam_fwd = ext_client::render::overlay_3d::get_camera_forward();
        const float cam_yaw = std::atan2(cam_fwd.x, cam_fwd.z);

        // North indicator on radar perimeter
        const float north_angle = -cam_yaw - 1.5707963f;
        const ImVec2 n_pos(radar_center.x + std::cos(north_angle) * (radar_radius - 8.0f),
                           radar_center.y + std::sin(north_angle) * (radar_radius - 8.0f));
        draw_outlined_text(draw_list, ImVec2(n_pos.x - 3.0f, n_pos.y - 5.0f), IM_COL32(200, 220, 255, 220), "N");

        // Boss blip on radar
        const float rel_x = (unique_alert_pos.x - local_pos.x) * 0.1f; // in meters
        const float rel_z = (unique_alert_pos.z - local_pos.z) * 0.1f;
        const float dist_m = std::sqrt(rel_x * rel_x + rel_z * rel_z);
        const float clamped_dist = std::min(dist_m, max_radar_range_m);
        const float blip_r = (clamped_dist / max_radar_range_m) * radar_radius;

        const float world_angle = std::atan2(rel_x, rel_z);
        const float radar_angle = (world_angle - cam_yaw) - 1.5707963f;

        const ImVec2 blip_pos(radar_center.x + std::cos(radar_angle) * blip_r,
                              radar_center.y + std::sin(radar_angle) * blip_r);

        draw_list->AddCircleFilled(blip_pos, 5.0f, IM_COL32(255, 40, 40, static_cast<int>(255 * pulse)));
        draw_list->AddCircle(blip_pos, 7.0f, IM_COL32(255, 215, 30, 255), 12, 1.5f);
      }
    }

    // -----------------------------------------------------------------------
    // 4. On-Screen Telemetry HUD (Optional)
    // -----------------------------------------------------------------------
    if (cfg.show_telemetry_hud) {
      D3DMATRIX vp{};
      const bool matrix_ok = ext_client::render::overlay_3d::get_view_projection_matrix(vp);
      const auto cam_pos = ext_client::render::overlay_3d::get_camera_position();

      char hud_buf[256]{};
      std::snprintf(hud_buf, sizeof(hud_buf),
        "[3D HUD Telemetry]\nState: %s\nPlayer: (%.1f, %.1f, %.1f)\nCamera: (%.1f, %.1f, %.1f)\nMatrix: %s | Entities Drawn: %d",
        proc_name ? proc_name : "In-World",
        local_pos.x, local_pos.y, local_pos.z,
        cam_pos.x, cam_pos.y, cam_pos.z,
        matrix_ok ? "OK" : "NONE",
        entities_rendered
      );

      const ImVec2 hud_pos(15.0f, io.DisplaySize.y - 100.0f);
      const ImVec2 t_size = ImGui::CalcTextSize(hud_buf);
      const ImVec2 box_min(hud_pos.x - 6.0f, hud_pos.y - 4.0f);
      const ImVec2 box_max(hud_pos.x + t_size.x + 6.0f, hud_pos.y + t_size.y + 4.0f);

      draw_list->AddRectFilled(box_min, box_max, IM_COL32(10, 15, 25, 210), 4.0f);
      draw_list->AddRect(box_min, box_max, IM_COL32(70, 180, 240, 220), 4.0f);
      draw_outlined_text(draw_list, hud_pos, IM_COL32(230, 245, 255, 255), hud_buf);
    }
  }

  auto handle_menu(menu_builder& ui) -> void {
    auto& cfg = ext_client::core::config::data().hud_esp;

    ui.section("3D World HUD & ESP System");
    ui.checkbox("Enable 3D HUD & ESP", &cfg.enabled);

    if (!cfg.enabled) {
      ui.text_disabled("Enable 3D HUD & ESP to view in-game overlays.");
      return;
    }

    ui.spacing();
    ui.section("World Overlays & Radar (Player QoL)");
    ui.checkbox("Show 3D Ground Target Ring", &cfg.show_target_indicator);
    if (cfg.show_target_indicator) {
      ui.slider_float("Target Ring Radius", &cfg.target_ring_radius, 8.0f, 35.0f);
    }
    ui.checkbox("Show Dropped Items ESP (SoX, Elixirs, Stones, Gold)", &cfg.show_item_drop_esp);
    ui.checkbox("Show Unique Boss Alert Banner", &cfg.show_unique_alert);
    ui.checkbox("Show Unique Boss Directional Radar & Compass Arrow", &cfg.show_boss_radar);
    ui.checkbox("Show Mini-Radar Compass Widget", &cfg.show_radar_compass_widget);
    ui.text_wrapped("Alerts when a Unique monster spawns or enters visual range, pointing an off-screen compass arrow straight to the boss.");

    ui.spacing();
    ui.section("Surrounding Entities (ESP)");
    ui.checkbox("Show Player ESP (Overhead Level, Name & Guild)", &cfg.show_player_esp);
    if (cfg.show_player_esp) {
      ui.checkbox("Show Local Player Overhead Tag", &cfg.show_self_esp);
    }
    ui.checkbox("Show Monster ESP", &cfg.show_monster_esp);
    if (cfg.show_monster_esp) {
      ui.checkbox("Show Monster Mini HP Bar", &cfg.show_monster_hp_bar);
      const char* rarity_items[] = {
        "All Creatures (No Filter)",
        "Champions & Above",
        "Giants & Above",
        "Uniques Only"
      };
      ui.combo("Rarity Filter", &cfg.min_rarity, rarity_items, 4);
    }
    ui.checkbox("Show Pet ESP", &cfg.show_pet_esp);
    ui.checkbox("Show NPC / Guard ESP", &cfg.show_npc_esp);
    ui.slider_float("Overhead Text Offset (Y)", &cfg.overhead_offset_y, 10.0f, 90.0f);
    ui.slider_int("Max Distance (Meters)", &cfg.max_distance_meters, 15, 180);

    ui.spacing();
    ui.section("Developer Diagnostics (Debug)");
    ui.checkbox("Show Target Snapline (Screen Bottom)", &cfg.show_target_snapline);
    ui.checkbox("Show 3D Mesh Boxes (OBB)", &cfg.show_mesh_boxes);
    if (cfg.show_mesh_boxes) {
      ui.slider_float("Mesh Box Line Thickness", &cfg.mesh_box_thickness, 0.8f, 3.0f);
    }
    ui.checkbox("Show Skeleton Bones Wireframe", &cfg.show_skeleton);
    if (cfg.show_skeleton) {
      ui.checkbox("Spine Line Only (High Performance)", &cfg.show_spine_only);
      ui.slider_float("Skeleton Line Thickness", &cfg.skeleton_thickness, 0.8f, 3.5f);
    }
    ui.checkbox("Show Local Player Ground Ring", &cfg.show_local_player_ring);
    ui.checkbox("Show On-Screen Telemetry HUD", &cfg.show_telemetry_hud);

    auto* local_player = cic_user::get_local_player();
    const char* proc_name = ccontroler::active_child_process_name();
    D3DMATRIX vp{};
    const bool matrix_ok = ext_client::render::overlay_3d::get_view_projection_matrix(vp);
    const auto cam_pos = ext_client::render::overlay_3d::get_camera_position();
    const std::size_t ent_count = centity_manager::entity_count();

    ui.text("Active Process: %s", proc_name ? proc_name : "(None)");
    ui.text("World State: %s", local_player ? "In-World (Active)" : "Non-World / Lobby");
    if (local_player) {
      vector3f pos{};
      if (get_entity_world_pos(local_player, pos)) {
        ui.text("Player Pos: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
      }
    }
    ui.text("3D Camera State: %s | Eye: (%.1f, %.1f, %.1f)",
            matrix_ok ? "Valid (Ready)" : "Not Detected",
            cam_pos.x, cam_pos.y, cam_pos.z);
    ui.text("Entities In Registry: %u", static_cast<unsigned>(ent_count));
  }

  auto handle_menu_overlay(menu_draw_context& /*ctx*/) -> void {
    render_hud_overlay();
  }

} // namespace

auto initialize() -> void {
  REGISTER_PLUGIN("hud_esp", "3D HUD & ESP");

  ADD_EVENT(EVENT_ON_MENU, handle_menu);
  ADD_EVENT(EVENT_ON_MENU_OVERLAY, handle_menu_overlay);
}

PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::hud_esp
