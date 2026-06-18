#include "ext_client.hpp"

#include "menu/widget_inspector.hpp"
#include "menu/menu_ui.hpp"
#include "menu/ui_browser_common.hpp"

#include "sdk/calarm_guide_mgr_wnd.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cif_manager.hpp"
#include "sdk/cif_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/sro_color.hpp"
#include "utils/ui_res_catalog.hpp"
#include "utils/string.hpp"

#include <imgui.h>

#include <Windows.h>

#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>

namespace ext_client::menu::widget_inspector {
  namespace {

    enum class visibility_filter : int {
      all = 0,
      shown = 1,
      hidden = 2,
    };

    using root_mode = ui_browser::root_mode;
    using ui_root = ui_browser::ui_root;

    struct inspector_state {
      cgwnd* selected = nullptr;
      wchar_t edit_text[256]{};
      char spawn_text[128] = "new label";
      int spawn_x = 5;
      int spawn_y = 1100;
      int spawn_w = 200;
      int spawn_h = 20;
      std::uint32_t spawn_color = 0xFFFFFFFF;
      std::vector<cif_manager::widget_info> search_results;
      const char* root_label = "none";
      root_mode root = root_mode::auto_detect;
      bool search_stale = true;
      bool show_only_visible = false;
      bool hide_no_text = false;
      visibility_filter visibility = visibility_filter::all;
      char search_text[128]{};
      int find_res_id = 0;
      bool draw_outline = false;
    };

    auto& state() {
      static inspector_state s;
      return s;
    }

    auto widget_prefs() -> decltype(ext_client::config::data().widgets)& {
      return ext_client::config::data().widgets;
    }

    auto is_safe_widget(const cgwnd* wnd) -> bool {
      return cif_manager::is_walkable_root(wnd);
    }

    auto resolve_ui_root() -> ui_root {
      ui_browser::root_labels labels{};
      labels.ingame_res_map = "IngameResMap@0x1420408";
      return ui_browser::resolve_ui_root(state().root, labels);
    }

    auto enumerate_browse_roots() -> std::vector<cgwnd*> {
      return ui_browser::enumerate_browse_roots(state().root);
    }

    auto select_widget(cgwnd* widget) -> void {
      auto& s = state();
      if (!is_safe_widget(widget)) {
        s.selected = nullptr;
        return;
      }

      s.selected = widget;
      s.edit_text[0] = L'\0';
      if (cif_manager::is_static(widget)) {
        cif_manager::read_static_text(cif_static::from(widget), s.edit_text, 256);
      }
    }

    struct tree_user_ctx {
      inspector_state* insp = nullptr;
    };

    auto tree_selected(void* ctx) -> cgwnd* {
      const auto* user = static_cast<const tree_user_ctx*>(ctx);
      return user && user->insp ? user->insp->selected : nullptr;
    }

    auto tree_on_select(void* ctx, cgwnd* wnd, int /*res_key*/) -> void {
      select_widget(wnd);
    }

    auto search_active() -> bool;

    auto widget_detail_suffix(const cif_manager::widget_info& info, char* dst, std::size_t dst_count) -> void {
      if (!dst || dst_count == 0) {
        return;
      }
      dst[0] = '\0';

      std::string text_utf8 = ::ext_client::utils::string::to_utf8(info.text);

      if (info.ddj_path[0] != '\0') {
        std::string_view base = ::ext_client::utils::string::basename(info.ddj_path);
        std::snprintf(dst, dst_count, " <%.*s>", static_cast<int>(base.size()), base.data());
        return;
      }
      if (!text_utf8.empty()) {
        std::snprintf(dst, dst_count, " <%s>", text_utf8.c_str());
        return;
      }
      std::strncpy(dst, " <no text>", dst_count - 1);
    }

    auto draw_widget_row(const cif_manager::widget_info& info) -> void {
      auto& s = state();

      char detail[192]{};
      widget_detail_suffix(info, detail, sizeof(detail));

      // Keep the visible label short and stable — runtime id / rect belong in the tooltip only.
      char label[256]{};
      const char* res_tag = info.lookup_res_inferred ? "res~=" : "res=";
      if (info.res_name[0] != '\0') {
        std::snprintf(label,
                      sizeof(label),
                      "%*s[%s] %s %s%d%s%s",
                      info.depth * 2,
                      "",
                      info.type_name,
                      info.res_name,
                      res_tag,
                      info.lookup_res_key,
                      info.visible ? "" : " [hidden]",
                      detail);
      } else if (info.lookup_res_key >= 0) {
        std::snprintf(label,
                      sizeof(label),
                      "%*s[%s] %s%d%s%s",
                      info.depth * 2,
                      "",
                      info.type_name,
                      res_tag,
                      info.lookup_res_key,
                      info.visible ? "" : " [hidden]",
                      detail);
      } else {
        std::snprintf(label, sizeof(label), "%*s[%s]%s%s", info.depth * 2, "", info.type_name, info.visible ? " [hidden]" : "", detail);
      }

      // Stable per-widget ID — never derive ImGui IDs from snprintf labels (truncation breaks focus).
      ImGui::PushID(info.widget);
      const bool selected = s.selected == info.widget;
      if (ImGui::Selectable(label, selected) && is_safe_widget(info.widget)) {
        select_widget(info.widget);
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("id=%d  %dx%d @ (%d,%d)  %p", info.control_id, info.rect_w, info.rect_h, info.rect_x, info.rect_y, info.widget);
      }
      ImGui::PopID();
    }

    auto resolve_alarm_mgr() -> calarm_guide_mgr_wnd* {
      if (auto* mgr = calarm_guide_mgr_wnd::current()) {
        return mgr;
      }
      return calarm_guide_mgr_wnd::resolve();
    }

    auto draw_alarm_slots_debug(calarm_guide_mgr_wnd* mgr) -> void {
      if (!mgr || !calarm_guide_mgr_wnd::is_instance(mgr)) {
        return;
      }

      ImGui::TextDisabled("alarm mgr %p id=%d", mgr, mgr->m_control_id);
      for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
        calarm_guide_mgr_wnd::slot_debug_info slot{};
        calarm_guide_mgr_wnd::read_slot_debug(mgr, i, slot);
        ImGui::TextDisabled(
          "%zu: type=%d active=%d status=%u %p", i, slot.entry_type, slot.entry_active ? 1 : 0, slot.alarm_status, slot.slot_widget);
        if (slot.yellow_path[0] != '\0') {
          ImGui::TextDisabled("  y: %s", slot.yellow_path);
        }
        if (slot.red_path[0] != '\0') {
          ImGui::TextDisabled("  r: %s", slot.red_path);
        }
        if (slot.ref_icon_path[0] != '\0') {
          ImGui::TextDisabled("  ref: %s", slot.ref_icon_path);
        }
        if (slot.tex_path[0] != '\0') {
          ImGui::TextDisabled("  tex: %s", slot.tex_path);
        }
        if (slot.effect_widget) {
          ImGui::TextDisabled("  fx: %p", slot.effect_widget);
        }
      }
    }

    auto widget_has_text(const cif_manager::widget_info& info) -> bool {
      if (!info.text[0]) {
        return false;
      }
      for (std::size_t i = 0; info.text[i] != L'\0'; ++i) {
        if (info.text[i] > L' ') {
          return true;
        }
      }
      return false;
    }

    auto widget_matches_filters(const cif_manager::widget_info& info) -> bool {
      auto& s = state();
      if (s.hide_no_text && !widget_has_text(info)) {
        return false;
      }
      if (s.visibility == visibility_filter::shown && !info.visible) {
        return false;
      }
      if (s.visibility == visibility_filter::hidden && info.visible) {
        return false;
      }
      if (s.find_res_id > 0 && info.lookup_res_key != s.find_res_id) {
        return false;
      }

      if (s.search_text[0] == '\0') {
        return true;
      }

      std::string text_utf8 = ::ext_client::utils::string::to_utf8(info.text);
      char haystack[960]{};
      std::snprintf(haystack,
                    sizeof(haystack),
                    "%s %s res=%d id=%d %p %s %s",
                    info.type_name,
                    info.res_name,
                    info.lookup_res_key,
                    info.control_id,
                    info.widget,
                    text_utf8.c_str(),
                    info.ddj_path);

      char needle[sizeof(s.search_text)]{};
      std::strncpy(needle, s.search_text, sizeof(needle) - 1);
      for (char* p = needle; *p; ++p) {
        if (*p >= 'A' && *p <= 'Z') {
          *p = static_cast<char>(*p - 'A' + 'a');
        }
      }
      for (char* p = haystack; *p; ++p) {
        if (*p >= 'A' && *p <= 'Z') {
          *p = static_cast<char>(*p - 'A' + 'a');
        }
      }
      return std::strstr(haystack, needle) != nullptr;
    }

    auto search_active() -> bool {
      const auto& s = state();
      return s.search_text[0] != '\0' || s.find_res_id > 0;
    }

    auto run_search_scan() -> void {
      auto& s = state();
      auto& cfg = widget_prefs();

      s.search_results.clear();
      s.root_label = resolve_ui_root().label;

      if (!search_active()) {
        s.search_stale = false;
        return;
      }

      const auto roots = enumerate_browse_roots();
      if (roots.empty()) {
        s.search_stale = false;
        return;
      }

      const int probe_max_id = cfg.deep_scan ? cfg.probe_max_id : 0;
      std::vector<cif_manager::widget_info> all;
      for (auto* root_wnd : roots) {
        const auto chunk = cfg.static_only ? cif_manager::walk_static_texts(root_wnd, cif_manager::walk_unlimited_depth, probe_max_id)
                                           : cif_manager::walk(root_wnd, cif_manager::walk_unlimited_depth, probe_max_id);
        all.insert(all.end(), chunk.begin(), chunk.end());
      }

      s.search_results.reserve(all.size());
      for (const auto& item : all) {
        if (widget_matches_filters(item)) {
          s.search_results.push_back(item);
        }
      }
      s.search_stale = false;
    }

    auto draw_toolbar_row() -> void {
      auto& s = state();
      auto& cfg = widget_prefs();

      const auto root = resolve_ui_root();
      s.root_label = root.label;

      ImGui::TextDisabled("Root");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(190.0f);
      int root_mode_i = static_cast<int>(s.root);
      if (ImGui::Combo("##root_pick", &root_mode_i, "Auto\0Active process\0CGInterface\0Ingame res map\0IFace res map\0")) {
        s.root = static_cast<root_mode>(root_mode_i);
        s.search_stale = true;
      }
      ImGui::SameLine();
      if (root.res_map_view) {
        ImGui::TextDisabled("%s", s.root_label);
      } else {
        ImGui::TextDisabled("%s %s", s.root_label, root.wnd ? "" : "(n/a)");
      }

      ImGui::SameLine(0.0f, 18.0f);
      ImGui::Checkbox("outline on hover", &s.draw_outline);

      ImGui::SameLine(0.0f, 18.0f);
      if (ImGui::Checkbox("visible only##wid", &s.show_only_visible)) {}
      ImGui::SameLine();
      if (ImGui::Checkbox("static only##wid", &cfg.static_only)) {
        ext_client::menu::ui::setting_changed(false);
        s.search_stale = true;
      }
      ImGui::SameLine();
      if (ImGui::Checkbox("deep scan##wid", &cfg.deep_scan)) {
        ext_client::menu::ui::setting_changed(false);
        s.search_stale = true;
      }

      if (cfg.deep_scan) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(72.0f);
        if (ImGui::InputInt("probe##wid", &cfg.probe_max_id, 0, 0)) {
          if (cfg.probe_max_id < 1) {
            cfg.probe_max_id = 1;
          } else if (cfg.probe_max_id > 512) {
            cfg.probe_max_id = 512;
          }
          ext_client::menu::ui::setting_changed(false);
          s.search_stale = true;
        }
      }
    }

    auto draw_search_row() -> void {
      auto& s = state();

      ImGui::SetNextItemWidth(-1.0f);
      if (ImGui::InputTextWithHint(
            "##widget_search", "Search (flat scan) — leave empty for live tree", s.search_text, sizeof(s.search_text))) {
        s.search_stale = true;
      }

      ImGui::SetNextItemWidth(90.0f);
      if (ImGui::InputInt("res id##wid", &s.find_res_id, 0, 0)) {
        s.search_stale = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear##widres")) {
        s.find_res_id = 0;
        s.search_stale = true;
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100.0f);
      int visibility = static_cast<int>(s.visibility);
      if (ImGui::Combo("show##wid", &visibility, "all\0shown\0hidden\0")) {
        s.visibility = static_cast<visibility_filter>(visibility);
        s.search_stale = true;
      }
      ImGui::SameLine();
      if (ImGui::Checkbox("hide <no text>##wid", &s.hide_no_text)) {
        s.search_stale = true;
      }

      if (search_active()) {
        ImGui::SameLine();
        if (ImGui::Button("Scan##wid")) {
          run_search_scan();
        }
        if (s.search_stale) {
          ImGui::SameLine();
          ImGui::TextDisabled("(stale)");
        } else {
          ImGui::SameLine();
          ImGui::TextDisabled("%zu hits", s.search_results.size());
        }
      }
    }

    auto draw_hover_pick_panel() -> void {
      ImGui::TextDisabled("Hover (g_CurrentIfUnderCursor)");
      ImGui::Separator();

      cgwnd* target = cif_manager::resolve_hovered_widget(false);
      if (!target) {
        ImGui::TextWrapped("Move the mouse over a game widget. Click Select hovered to sync pick.");
        return;
      }

      const cgwnd_bounds bounds = target->get_bounds();
      const auto vftable = reinterpret_cast<std::uint32_t>(target->vftable);

      if (ImGui::BeginTable("hover_props", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Class");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(cif_manager::vftable_type_name(vftable));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Address");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%p", target);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("UniqueID");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", cif_manager::unique_id(target));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Runtime id");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", cif_manager::control_id(target));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Bounds");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d, %d  %dx%d", bounds.x, bounds.y, bounds.w, bounds.h);

        ImGui::EndTable();
      }

      if (ImGui::Button("Select hovered##wid")) {
        if (auto* picked = cif_manager::resolve_hovered_widget(true)) {
          select_widget(picked);
        }
      }
    }

    auto draw_widget_browser() -> void {
      auto& s = state();
      auto& cfg = widget_prefs();

      if (cfg.auto_refresh && search_active()) {
        run_search_scan();
      }

      if (search_active()) {
        if (s.search_stale) {
          ImGui::TextDisabled("Search active — click Scan to run a flat walk.");
        } else {
          ImGuiListClipper clipper;
          clipper.Begin(static_cast<int>(s.search_results.size()));
          while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
              draw_widget_row(s.search_results[static_cast<std::size_t>(row)]);
            }
          }
        }
        return;
      }

      const auto root = resolve_ui_root();
      s.root_label = root.label;

      std::unordered_set<cgwnd*> seen;
      tree_user_ctx user_ctx{&s};
      ui_browser::tree_recurse_ctx ctx{};
      ctx.static_only = cfg.static_only;
      ctx.show_only_visible = s.show_only_visible;
      ctx.seen = &seen;
      ctx.hooks = {tree_selected, tree_on_select, &user_ctx};

      if (s.root == root_mode::ingame_res_map) {
        ui_browser::draw_res_map_roots_tree(cif_manager::enumerate_ingame_res_map(), ctx);
        return;
      }
      if (s.root == root_mode::iface_res_map) {
        ui_browser::draw_res_map_roots_tree(cif_manager::enumerate_iface_res_map(), ctx);
        return;
      }

      if (!root.wnd) {
        if (s.root == root_mode::active_process) {
          ImGui::TextDisabled("CGFXMainFrame UI root unavailable — try Auto or hover a widget.");
        } else {
          ImGui::TextDisabled("Root not available — hover a widget and use Select hovered.");
        }
        return;
      }

      ui_browser::recurse_widget_tree(root.wnd, ctx);
    }

    auto lookup_info_for(cgwnd* widget) -> cif_manager::widget_info {
      return ui_browser::lookup_info_for(widget, state().search_results, state().root);
    }

    auto draw_selection_panel() -> void {
      auto& s = state();
      ImGui::TextDisabled("Selection");
      ImGui::Separator();

      if (!s.selected || !is_safe_widget(s.selected)) {
        s.selected = nullptr;
        ImGui::TextWrapped("Click a widget in the tree or use Select hovered.");
        return;
      }

      const auto lookup = lookup_info_for(s.selected);
      const auto vftable = reinterpret_cast<std::uint32_t>(s.selected->vftable);

      if (ImGui::BeginTable("sel_props", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Class");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(cif_manager::vftable_type_name(vftable));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Address");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%p", s.selected);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Bounds");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%dx%d @ (%d,%d)", s.selected->rect_w(), s.selected->rect_h(), s.selected->rect_x(), s.selected->rect_y());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Runtime id");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", cif_manager::control_id(s.selected));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("UniqueID");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", cif_manager::unique_id(s.selected));

        if (lookup.lookup_res_key >= 0) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("Res key");
          ImGui::TableSetColumnIndex(1);
          if (lookup.lookup_res_inferred) {
            ImGui::Text("%d (inferred)", lookup.lookup_res_key);
          } else {
            ImGui::Text("%d", lookup.lookup_res_key);
          }
        }

        if (lookup.res_name[0] != '\0') {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("Res name");
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(lookup.res_name);
        }

        if (lookup.ddj_path[0] != '\0') {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("Texture");
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(lookup.ddj_path);
        }

        ImGui::EndTable();
      }

      if (cif_manager::is_static(s.selected)) {
        char edit_utf8[320]{};
        std::string text_utf8 = ::ext_client::utils::string::to_utf8(s.edit_text);
        std::strncpy(edit_utf8, text_utf8.c_str(), sizeof(edit_utf8) - 1);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("##edit_text", edit_utf8, sizeof(edit_utf8))) {
          MultiByteToWideChar(CP_UTF8, 0, edit_utf8, -1, s.edit_text, 256);
        }

        ImVec4 color = ImGui::ColorConvertU32ToFloat4(ext_client::utils::color::to_imgui(s.spawn_color));
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::ColorEdit4("color##sel", &color.x, ImGuiColorEditFlags_NoInputs)) {
          s.spawn_color = ext_client::utils::color::from_imgui(ImGui::ColorConvertFloat4ToU32(color));
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply text")) {
          cif_manager::apply_static_label(cif_static::from(s.selected), s.edit_text, s.spawn_color);
        }
      }

      if (ImGui::Button("Hide##sel")) {
        s.selected->set_visible(false);
      }
      ImGui::SameLine();
      if (ImGui::Button("Show##sel")) {
        s.selected->set_visible(true);
      }
      ImGui::SameLine();
      if (ImGui::Button("Destroy##sel")) {
        const auto doomed = s.selected;
        s.selected = nullptr;
        cif_manager::destroy_widget(doomed);
      }
    }

    auto draw_spawn_panel() -> void {
      auto& s = state();
      const auto root = resolve_ui_root();

      if (!ImGui::CollapsingHeader("Spawn label")) {
        return;
      }

      ImGui::SetNextItemWidth(-1.0f);
      ImGui::InputText("##spawn_text", s.spawn_text, sizeof(s.spawn_text));

      ImGui::SetNextItemWidth(52.0f);
      ImGui::InputInt("x##sp", &s.spawn_x);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(52.0f);
      ImGui::InputInt("y##sp", &s.spawn_y);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(52.0f);
      ImGui::InputInt("w##sp", &s.spawn_w);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(52.0f);
      ImGui::InputInt("h##sp", &s.spawn_h);

      ImVec4 spawn_color = ImGui::ColorConvertU32ToFloat4(ext_client::utils::color::to_imgui(s.spawn_color));
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::ColorEdit4("color##sp", &spawn_color.x, ImGuiColorEditFlags_NoInputs)) {
        s.spawn_color = ext_client::utils::color::from_imgui(ImGui::ColorConvertFloat4ToU32(spawn_color));
      }
      ImGui::SameLine();
      if (ImGui::Button("Spawn") && root.wnd) {
        wchar_t wide_spawn[128]{};
        MultiByteToWideChar(CP_UTF8, 0, s.spawn_text, -1, wide_spawn, 128);

        cif_manager::spawn_label_options opts{};
        opts.rect = {5, s.spawn_y, s.spawn_w, s.spawn_h};
        opts.rect.x = s.spawn_x;
        opts.rect.type = 5;
        opts.text = wide_spawn;
        opts.text_color = s.spawn_color;
        opts.visible = true;

        if (auto* spawned = cif_manager::spawn_static_label(root.wnd, opts)) {
          select_widget(spawned);
        }
      }
    }

    auto draw_res_map_row(const char* title, const std::vector<cif_manager::res_map_entry>& entries) -> void {
      if (!ImGui::TreeNode(title)) {
        return;
      }

      if (entries.empty()) {
        ImGui::TextDisabled("empty / unreadable");
        ImGui::TreePop();
        return;
      }

      if (ImGui::BeginTable("res_map", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("key");
        ImGui::TableSetupColumn("class");
        ImGui::TableSetupColumn("addr");
        ImGui::TableSetupColumn("uid");
        ImGui::TableHeadersRow();

        for (const auto& entry : entries) {
          if (!entry.wnd) {
            continue;
          }
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text("0x%X", entry.key);
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(cif_manager::vftable_type_name(reinterpret_cast<std::uint32_t>(entry.wnd->vftable)));
          ImGui::TableSetColumnIndex(2);
          if (ImGui::Selectable("##pick", false, ImGuiSelectableFlags_SpanAllColumns)) {
            select_widget(entry.wnd);
          }
          ImGui::SameLine(0.0f, 0.0f);
          ImGui::Text("%p", entry.wnd);
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%d", cif_manager::unique_id(entry.wnd));
        }
        ImGui::EndTable();
      }

      ImGui::TreePop();
    }

    auto draw_res_maps_panel() -> void {
      if (!ImGui::CollapsingHeader("Res maps (read-only)")) {
        return;
      }

      ImGui::TextDisabled("Map keys are not UniqueIDs. Safe cap: 200 entries per map.");
      draw_res_map_row("Ingame NIF @ 0x1420408 (sub_4016F0)", cif_manager::enumerate_ingame_res_map());
      draw_res_map_row("CGInterface +0x374 (sub_9CF790)", cif_manager::enumerate_iface_res_map());
    }

    auto draw_alarm_debug_panel() -> void {
      if (!widget_prefs().show_alarm_debug) {
        return;
      }
      if (!cg_interface::is_ready()) {
        return;
      }
      if (!ImGui::CollapsingHeader("Alarm strip debug", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      if (auto* mgr = resolve_alarm_mgr()) {
        draw_alarm_slots_debug(mgr);
      } else {
        ImGui::TextDisabled("alarm mgr not found");
      }
    }

  } // namespace

  auto draw_inspector_footer() -> void {
    constexpr ImGuiWindowFlags footer_flags = ImGuiWindowFlags_NoNav;
    const float footer_h = 200.0f;
    if (!ImGui::BeginChild("widget_footer", ImVec2(0.0f, footer_h), ImGuiChildFlags_Border, footer_flags)) {
      ImGui::EndChild();
      return;
    }

    if (ImGui::BeginTable("widget_footer_cols", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
      ImGui::TableSetupColumn("hover", ImGuiTableColumnFlags_WidthStretch, 0.42f);
      ImGui::TableSetupColumn("selection", ImGuiTableColumnFlags_WidthStretch, 0.58f);
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      draw_hover_pick_panel();

      ImGui::TableSetColumnIndex(1);
      draw_selection_panel();

      ImGui::EndTable();
    }

    ImGui::EndChild();
  }

  auto draw() -> void {
    draw_toolbar_row();
    ImGui::Spacing();
    draw_search_row();
    ImGui::Spacing();

    const float footer_reserve = 212.0f;
    const float tree_h = ext_client::menu::ui::avail_height(footer_reserve, 120.0f);
    ImGui::BeginChild("widget_tree_host", ImVec2(0.0f, tree_h), ImGuiChildFlags_Border);
    draw_widget_browser();
    ImGui::EndChild();

    draw_inspector_footer();

    if (ImGui::CollapsingHeader("Tools")) {
      draw_res_maps_panel();
      draw_spawn_panel();
      draw_alarm_debug_panel();
    }
  }

  auto draw_overlay() -> void {
    if (!state().draw_outline) {
      return;
    }
    if (auto* target = cif_manager::resolve_hovered_widget(false)) {
      ui_browser::draw_widget_glow(target, IM_COL32(255, 64, 64, 255), 2.0f);
    }
  }

} // namespace ext_client::menu::widget_inspector
