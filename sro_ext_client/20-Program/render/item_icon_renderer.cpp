#include "pch.hpp"
#include "render/item_icon_renderer.hpp"
#include "render/item_tooltip_renderer.hpp"
#include "sdk/game/cic_player.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/memory.hpp"

#include <d3d9.h>
#include <imgui.h>
#include <mutex>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cstdio>
#include <windows.h>

namespace ext_client::render {

  namespace {

    // Native Silkroad texture loader: sub_4E59C0 (takes const std::string* path, returns IDirect3DTexture9*)
    using pfn_load_texture = auto (__cdecl*)(const void* path_str) -> IDirect3DTexture9*;
    inline constexpr std::uintptr_t k_fn_load_texture = 0x004E59C0;

    auto call_engine_load_texture(const void* msvc_str_storage) -> IDirect3DTexture9* {
      if (!msvc_str_storage || !ext_client::utils::memory::is_valid_ptr(msvc_str_storage)) {
        return nullptr;
      }
      const auto pfn = reinterpret_cast<pfn_load_texture>(k_fn_load_texture);
      return pfn(msvc_str_storage);
    }

    auto is_valid_d3d_texture(IDirect3DTexture9* tex) -> bool {
      if (!tex || !ext_client::utils::memory::is_valid_ptr(tex)) return false;
      void* vtable = *reinterpret_cast<void**>(tex);
      if (!vtable || !ext_client::utils::memory::is_valid_ptr(vtable)) return false;

      IDirect3DTexture9* verified = nullptr;
      if (SUCCEEDED(tex->QueryInterface(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&verified)))) {
        if (verified) {
          verified->Release(); // QueryInterface increments refcount
          return true;
        }
      }
      return false;
    }

    auto normalize_texture_path(std::string path) -> std::string {
      for (char& c : path) {
        if (c == '/') c = '\\';
        else c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
      }
      return path;
    }

    std::unordered_map<std::string, IDirect3DTexture9*> s_texture_cache;
    std::mutex s_cache_mutex;

  } // namespace

  auto get_texture(const std::string& ddj_path) -> IDirect3DTexture9* {
    if (ddj_path.empty() || ddj_path == "icon\\xxx" || ddj_path == "xxx") {
      return nullptr;
    }

    const auto norm = normalize_texture_path(ddj_path);

    std::lock_guard lock(s_cache_mutex);
    const auto it = s_texture_cache.find(norm);
    if (it != s_texture_cache.end()) {
      return it->second;
    }

    // Allocate MSVC9 std::string through the game's allocator
    ext_client::msvc9::string msvc_str(norm.c_str());
    auto* tex = call_engine_load_texture(msvc_str.raw());
    if (tex && is_valid_d3d_texture(tex)) {
      s_texture_cache[norm] = tex;
      return tex;
    }

    // Cache null result to prevent repetitive failing lookups
    s_texture_cache[norm] = nullptr;
    return nullptr;
  }

  auto get_sox_edge_texture() -> IDirect3DTexture9* {
    return get_texture("icon\\item\\etc\\icon_edge_rare.ddj");
  }

  auto get_nasrun_edge_texture() -> IDirect3DTexture9* {
    return get_texture("icon\\item\\etc\\icon_edge_nasrun.ddj");
  }

  auto clear_texture_cache() -> void {
    std::lock_guard lock(s_cache_mutex);
    s_texture_cache.clear();
  }

#pragma comment(lib, "d3d9.lib")

  // ---------------------------------------------------------------------------
  // Rendering Helpers
  // ---------------------------------------------------------------------------

  auto render_item_icon(IDirect3DTexture9* icon_tex,
                        bool is_sox,
                        bool is_devil,
                        const ImVec2& size,
                        std::uint32_t seed) -> void {
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. Slot background & border
    dl->AddRectFilled(p0, p1, IM_COL32(18, 22, 30, 240), 2.0f);
    dl->AddRect(p0, p1, IM_COL32(55, 65, 85, 200), 2.0f);

    // 2. Base item icon
    if (icon_tex) {
      dl->AddImage(reinterpret_cast<ImTextureID>(icon_tex), p0, p1, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    }

    // 3. Rare / SoX animated edge overlay
    if (is_sox) {
      auto* sox_tex = get_sox_edge_texture();
      if (sox_tex) {
        ImVec2 uv0{}, uv1{};
        get_sox_uvs(calculate_sox_frame(seed), uv0, uv1);
        dl->AddImage(reinterpret_cast<ImTextureID>(sox_tex), p0, p1, uv0, uv1);
      }
    }

    // 4. Nasrun / Devil's Spirit animated edge overlay
    if (is_devil) {
      auto* nasrun_tex = get_nasrun_edge_texture();
      if (nasrun_tex) {
        ImVec2 uv0{}, uv1{};
        get_nasrun_uvs(calculate_nasrun_frame(seed), uv0, uv1);
        dl->AddImage(reinterpret_cast<ImTextureID>(nasrun_tex), p0, p1, uv0, uv1);
      }
    }

    ImGui::Dummy(size);
  }

  auto render_item_icon(const cref_obj_item* ref,
                        const ImVec2& size,
                        bool show_sox,
                        std::uint32_t seed) -> void {
    if (!ref || !ext_client::utils::memory::is_valid_ptr(ref)) {
      render_item_icon(nullptr, false, false, size, seed);
      return;
    }

    const auto path = ref->icon_path();
    auto* icon_tex = !path.empty() ? get_texture(path) : nullptr;
    const bool is_sox = show_sox && ref->is_sox();
    const bool is_devil = ref->is_devil_spirit();
    render_item_icon(icon_tex, is_sox, is_devil, size, seed != 0 ? seed : ref->ref_id());
  }

  auto render_item_slot_from_data(const char* str_id,
                                  const sdk::game::item_tooltip_data& data,
                                  const ImVec2& size,
                                  bool interactive) -> bool {
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. Draw Icon with SoX / Devil overlay (devil spirit overlay only animates when period is active)
    auto* icon_tex = !data.icon_path.empty() ? get_texture(data.icon_path) : nullptr;
    const bool animate_devil = data.is_devil_spirit && data.is_devil_spirit_active && !data.is_expired;
    render_item_icon(icon_tex, data.is_sox, animate_devil, size, data.opt_level);

    // 2. Opt level badge (e.g. "+7" at top-left)
    if (data.opt_level > 0) {
      char opt_buf[16];
      std::snprintf(opt_buf, sizeof(opt_buf), "+%u", data.opt_level);
      const ImVec2 opt_pos(p0.x + 2.0f, p0.y + 1.0f);
      // Drop shadow + golden text
      dl->AddText(ImVec2(opt_pos.x + 1.0f, opt_pos.y + 1.0f), IM_COL32(0, 0, 0, 220), opt_buf);
      dl->AddText(opt_pos, IM_COL32(255, 220, 60, 255), opt_buf);
    }

    // 3. Stack count badge (e.g. "50" at bottom-right)
    if (data.stack_count > 1) {
      char cnt_buf[16];
      std::snprintf(cnt_buf, sizeof(cnt_buf), "%u", data.stack_count);
      const ImVec2 text_sz = ImGui::CalcTextSize(cnt_buf);
      const ImVec2 cnt_pos(p1.x - text_sz.x - 2.0f, p1.y - text_sz.y - 1.0f);
      dl->AddText(ImVec2(cnt_pos.x + 1.0f, cnt_pos.y + 1.0f), IM_COL32(0, 0, 0, 220), cnt_buf);
      dl->AddText(cnt_pos, IM_COL32(255, 255, 255, 255), cnt_buf);
    }

    // 4. Interactive button for click / hover
    bool clicked = false;
    if (interactive) {
      ImGui::SetCursorScreenPos(p0);
      clicked = ImGui::InvisibleButton(str_id, size);
      if (ImGui::IsItemHovered()) {
        ext_client::render::render_item_tooltip_from_data(data);
      }
    }

    return clicked;
  }

  auto render_item_slot(const char* str_id,
                        const cso_item* item,
                        const ImVec2& size,
                        bool interactive) -> bool {
    if (!item || !item->is_valid()) {
      const ImVec2 p0 = ImGui::GetCursorScreenPos();
      const ImVec2 p1(p0.x + size.x, p0.y + size.y);
      ImDrawList* dl = ImGui::GetWindowDrawList();
      dl->AddRectFilled(p0, p1, IM_COL32(18, 22, 30, 160), 2.0f);
      dl->AddRect(p0, p1, IM_COL32(45, 50, 65, 120), 2.0f);
      ImGui::Dummy(size);
      return false;
    }

    const auto data = ext_client::sdk::game::extract_tooltip_data(item);
    return render_item_slot_from_data(str_id, data, size, interactive);
  }

  auto render_skill_tooltip(const ::c_skill_manager::s_skill_details& skill) -> void {
    ImGui::BeginTooltip();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.32f, 0.48f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.08f, 0.14f, 0.96f));

    const float wrap_width = 320.0f;
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);

    // 1. Skill Title & Level
    if (skill.level > 0) {
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s Lv %u", skill.name.c_str(), skill.level);
    } else {
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", skill.name.c_str());
    }

    ImGui::Spacing();

    // 2. Active / Passive type
    const char* type_str = skill.is_active ? "Active" : "Passive";
    ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.92f, 1.0f), "%s", type_str);

    // 3. Consumed MP / HP
    if (skill.mp_cost > 0) {
      ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.92f, 1.0f), "Consumed MP: %u", skill.mp_cost);
    } else if (skill.mp_ratio_cost > 0) {
      ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.92f, 1.0f), "Consumed MP: %u%%", skill.mp_ratio_cost);
    }

    if (skill.hp_cost > 0) {
      ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.92f, 1.0f), "Consumed HP: %u", skill.hp_cost);
    } else if (skill.hp_ratio_cost > 0) {
      ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.92f, 1.0f), "Consumed HP: %u%%", skill.hp_ratio_cost);
    }

    // 4. Used weapon
    if (!skill.req_weapon.empty()) {
      ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.92f, 1.0f), "Used weapon: %s", skill.req_weapon.c_str());
    }

    // 5. Description
    if (!skill.description.empty()) {
      ImGui::TextColored(ImVec4(0.86f, 0.88f, 0.92f, 1.0f), "%s", skill.description.c_str());
    }

    // 6. Skill Effects (Engine-formatted or parsed)
    // Game displays combat effects in soft wheat/gold color
    const ImVec4 effect_col(0.90f, 0.82f, 0.58f, 1.0f);

    bool has_effects = false;
    if (!skill.effects_text.empty()) {
      ImGui::Spacing();
      const std::string& eff = skill.effects_text;
      std::size_t start = 0;
      while (start < eff.size()) {
        std::size_t end = eff.find_first_of("\r\n", start);
        if (end == std::string::npos) end = eff.size();
        if (end > start) {
          std::string line = eff.substr(start, end - start);
          while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(0, 1);
          while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
          if (!line.empty()) {
            ImGui::TextColored(effect_col, "%s", line.c_str());
            has_effects = true;
          }
        }
        start = eff.find_first_not_of("\r\n", end);
      }
    }

    // Fallback if effects_text was empty but combat stats are present
    if (!has_effects && (skill.phy_atk_max > 0 || skill.mag_atk_max > 0 || skill.range_meters > 0.0f)) {
      ImGui::Spacing();
      if (skill.phy_atk_max > 0) {
        if (skill.atk_ratio > 0.0f) {
          ImGui::TextColored(effect_col, "Phy. atk. pwr %d~%d (%d%%)",
                             skill.phy_atk_min, skill.phy_atk_max, static_cast<int>(skill.atk_ratio * 100.0f));
        } else {
          ImGui::TextColored(effect_col, "Phy. atk. pwr %d~%d", skill.phy_atk_min, skill.phy_atk_max);
        }
      }
      if (skill.mag_atk_max > 0) {
        if (skill.atk_ratio > 0.0f) {
          ImGui::TextColored(effect_col, "Mag. atk. pwr %d~%d (%d%%)",
                             skill.mag_atk_min, skill.mag_atk_max, static_cast<int>(skill.atk_ratio * 100.0f));
        } else {
          ImGui::TextColored(effect_col, "Mag. atk. pwr %d~%d", skill.mag_atk_min, skill.mag_atk_max);
        }
      }
      if (skill.range_meters > 0.0f) {
        ImGui::TextColored(effect_col, "Front range Radius %.1fm", skill.range_meters);
      }
      if (skill.cast_time_sec > 0.0f) {
        ImGui::TextColored(effect_col, "Casting time %.1f Second", skill.cast_time_sec);
      }
      if (skill.cooldown_sec > 0.0f) {
        ImGui::TextColored(effect_col, "Cool time %.1f Second", skill.cooldown_sec);
      }
    }

    // 7. Next level condition section
    if (skill.has_next_level) {
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.92f, 1.0f), "* Next level condition");

      const ImVec4 red_col(0.92f, 0.22f, 0.22f, 1.0f);
      const ImVec4 normal_col(0.78f, 0.82f, 0.88f, 1.0f);

      auto* player = cic_player::local();
      const std::uint8_t player_mastery_lvl = ::c_skill_manager::get_player_mastery_level(skill.mastery_id);
      const std::uint32_t player_sp = player ? player->sp() : 0;
      const std::uint16_t player_str = player ? player->strength() : 0;
      const std::uint16_t player_int = player ? player->intelligence() : 0;

      // 7A. Mastery level condition
      if (skill.next_req_mastery_level > 0) {
        const bool met = (player_mastery_lvl >= skill.next_req_mastery_level);
        const std::string mname = !skill.next_mastery_name.empty() ? skill.next_mastery_name :
                                  (!skill.mastery_name.empty() ? skill.mastery_name : "Mastery");
        ImGui::TextColored(met ? normal_col : red_col, "Mastery level : %s [Lv %u]",
                           mname.c_str(), skill.next_req_mastery_level);
      }

      // 7B. Prerequisite Skills
      int pr_idx = 1;
      for (const auto& pr : skill.next_prereqs) {
        const bool met = ::c_skill_manager::is_skill_learned(pr.skill_id);
        ImGui::TextColored(met ? normal_col : red_col, "Required skill %d : %s Lv %u",
                           pr_idx++, pr.name.c_str(), pr.req_level);
      }

      // 7C. STR requirement
      if (skill.next_req_str > 0) {
        const bool met = (player_str >= skill.next_req_str);
        ImGui::TextColored(met ? normal_col : red_col, "Required STR : %u", skill.next_req_str);
      }

      // 7D. INT requirement
      if (skill.next_req_int > 0) {
        const bool met = (player_int >= skill.next_req_int);
        ImGui::TextColored(met ? normal_col : red_col, "Required INT : %u", skill.next_req_int);
      }

      // 7E. SP requirement
      if (skill.next_req_sp > 0) {
        const bool met = (player_sp >= skill.next_req_sp);
        ImGui::TextColored(met ? normal_col : red_col, "Required skill point : %u", skill.next_req_sp);
      }
    } else if (skill.is_max_level) {
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.50f, 1.0f), "[Max Level Reached]");
    }

    ImGui::PopTextWrapPos();

    // Dev footer
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Skill ID: %u | %s", skill.id, skill.code_name.c_str());

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    ImGui::EndTooltip();
  }

  auto render_empty_skill_slot(const ImVec2& size) -> void {
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    auto* nothing_tex = get_texture("interface\\skill\\skl_mastery_nothing.ddj");
    if (nothing_tex) {
      dl->AddImage(reinterpret_cast<ImTextureID>(nothing_tex), p0, p1);
    } else {
      // Silkroad empty slot: dark metallic recessed frame with center square
      dl->AddRectFilled(p0, p1, IM_COL32(14, 17, 24, 230), 3.0f);
      dl->AddRect(p0, p1, IM_COL32(40, 50, 68, 180), 3.0f, 0, 1.5f);

      // Inner recessed center box
      const float pad_x = size.x * 0.28f;
      const float pad_y = size.y * 0.28f;
      const ImVec2 c0(p0.x + pad_x, p0.y + pad_y);
      const ImVec2 c1(p1.x - pad_x, p1.y - pad_y);
      dl->AddRectFilled(c0, c1, IM_COL32(22, 28, 38, 160), 2.0f);
      dl->AddRect(c0, c1, IM_COL32(48, 60, 80, 140), 2.0f);
    }

    ImGui::Dummy(size);
  }

  auto render_skill_slot(const char* str_id,
                         const ::c_skill_manager::s_skill_details& skill,
                         const ImVec2& size,
                         bool interactive) -> bool {
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. Slot background & border
    dl->AddRectFilled(p0, p1, IM_COL32(16, 20, 28, 220), 4.0f);
    dl->AddRect(p0, p1, IM_COL32(50, 65, 88, 180), 4.0f);

    // 2. Icon image
    auto* icon_tex = !skill.icon_path.empty() ? get_texture(skill.icon_path) : nullptr;
    if (icon_tex) {
      dl->AddImage(reinterpret_cast<ImTextureID>(icon_tex),
                   ImVec2(p0.x + 2.0f, p0.y + 2.0f),
                   ImVec2(p1.x - 2.0f, p1.y - 2.0f));
    } else {
      const ImU32 cat_bg = (skill.skill_type == 0) ? IM_COL32(75, 45, 110, 180) :
                           ((skill.skill_type == 1) ? IM_COL32(120, 40, 40, 180) :
                           ((skill.skill_type == 2) ? IM_COL32(30, 80, 120, 180) : IM_COL32(50, 70, 90, 180)));
      dl->AddRectFilled(ImVec2(p0.x + 2.0f, p0.y + 2.0f), ImVec2(p1.x - 2.0f, p1.y - 2.0f), cat_bg, 3.0f);
      const char* p_letter = (skill.skill_type == 0) ? "P" : ((skill.skill_type == 1) ? "A" : ((skill.skill_type == 2) ? "B" : "S"));
      const ImVec2 t_sz = ImGui::CalcTextSize(p_letter);
      dl->AddText(ImVec2(p0.x + (size.x - t_sz.x) * 0.5f, p0.y + (size.y - t_sz.y) * 0.5f), IM_COL32(230, 230, 230, 220), p_letter);
    }

    // 3. Category pip in top-left
    const ImU32 pip_col = (skill.skill_type == 0) ? IM_COL32(180, 90, 240, 220) :
                          ((skill.skill_type == 1) ? IM_COL32(235, 70, 70, 220) :
                          ((skill.skill_type == 2) ? IM_COL32(60, 180, 245, 220) : IM_COL32(245, 160, 40, 220)));
    dl->AddCircleFilled(ImVec2(p0.x + 5.0f, p0.y + 5.0f), 2.5f, pip_col);

    // 4. Level / Rank badge in bottom-right
    if (skill.level > 0) {
      char lvl_buf[16];
      std::snprintf(lvl_buf, sizeof(lvl_buf), "%u", skill.level);
      const ImVec2 text_sz = ImGui::CalcTextSize(lvl_buf);
      const ImVec2 lvl_pos(p1.x - text_sz.x - 2.0f, p1.y - text_sz.y - 1.0f);
      dl->AddRectFilled(ImVec2(lvl_pos.x - 2.0f, lvl_pos.y), ImVec2(p1.x, p1.y), IM_COL32(10, 14, 20, 200), 2.0f);
      dl->AddText(lvl_pos, IM_COL32(240, 240, 240, 255), lvl_buf);
    }

    // 5. Cooldown overlay
    const int cd_ms = ::c_skill_manager::get_cooldown_remaining_ms(skill.id);
    if (cd_ms > 0) {
      dl->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 175), 4.0f);
      char cd_buf[16];
      std::snprintf(cd_buf, sizeof(cd_buf), "%.1fs", cd_ms / 1000.0f);
      const ImVec2 cd_sz = ImGui::CalcTextSize(cd_buf);
      dl->AddText(ImVec2(p0.x + (size.x - cd_sz.x) * 0.5f, p0.y + (size.y - cd_sz.y) * 0.5f),
                  IM_COL32(255, 230, 80, 255), cd_buf);
    }

    // 6. Interactive button & hover
    bool clicked = false;
    if (interactive) {
      ImGui::SetCursorScreenPos(p0);
      clicked = ImGui::InvisibleButton(str_id, size);
      if (ImGui::IsItemHovered()) {
        dl->AddRect(p0, p1, IM_COL32(255, 215, 60, 240), 4.0f, 0, 1.5f);
        render_skill_tooltip(skill);
      }
    } else {
      ImGui::Dummy(size);
    }

    return clicked;
  }

} // namespace ext_client::render
