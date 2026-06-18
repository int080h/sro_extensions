#include "menu/interface_manager.hpp"

#include "config/client_config.hpp"
#include "hooks/interface_hide_hook.hpp"
#include "hooks/interface_manager_runtime.hpp"
#include "hooks/d3d_hook.hpp"
#include "menu/menu_ui.hpp"
#include "menu/ui_browser_common.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cif_manager.hpp"
#include "sdk/cif_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/string.hpp"

#include <imgui.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

namespace ext_client::menu::interface_manager {
  namespace {

    enum class visibility_filter : int {
      all = 0,
      shown = 1,
      hidden = 2,
    };

    using root_mode = ui_browser::root_mode;
    using ui_root = ui_browser::ui_root;

    struct manager_state {
      cgwnd* selected = nullptr;
      int selected_res_key = -1;
      bool selected_ingame_map = false;
      std::vector<cif_manager::widget_info> search_results;
      const char* root_label = "none";
      root_mode root = root_mode::auto_detect;
      bool search_stale = true;
      bool show_only_visible = false;
      bool hide_no_text = false;
      bool draw_outline = true;
      bool quick_hides_open = false;
      visibility_filter visibility = visibility_filter::all;
      char search_text[128]{};
      int find_res_id = 0;
    };

    auto& state() {
      static manager_state s;
      return s;
    }

    auto widget_prefs() -> decltype(ext_client::config::data().widgets)& {
      return ext_client::config::data().widgets;
    }

    auto is_safe_widget(const cgwnd* wnd) -> bool {
      if (!wnd || !ext_client::msvc9::is_readable_ptr(wnd, sizeof(void*) * 4)) {
        return false;
      }
      const auto* vftable = *reinterpret_cast<void* const*>(wnd);
      return ext_client::msvc9::is_readable_ptr(vftable, sizeof(void*) * 4);
    }

    auto resolve_ui_root() -> ui_root {
      return ui_browser::resolve_ui_root(state().root);
    }

    auto current_ingame_map() -> bool {
      const auto& s = state();
      if (s.root == root_mode::ingame_res_map) {
        return true;
      }
      if (s.root == root_mode::iface_res_map) {
        return false;
      }
      return cg_interface::is_ingame_hud_ready();
    }

    auto enumerate_browse_roots() -> std::vector<cgwnd*> {
      return ui_browser::enumerate_browse_roots(state().root);
    }

    auto select_widget(cgwnd* widget, int res_key = -1) -> void {
      auto& s = state();
      if (!is_safe_widget(widget)) {
        s.selected = nullptr;
        s.selected_res_key = -1;
        return;
      }
      s.selected = widget;
      if (res_key >= 0) {
        s.selected_res_key = res_key;
      } else {
        cif_manager::widget_info info{};
        info.widget = widget;
        info.control_id = cif_manager::control_id(widget);
        const auto roots = enumerate_browse_roots();
        cif_manager::enrich_widget_info(info, roots.empty() ? nullptr : roots.front());
        s.selected_res_key = info.lookup_res_key;
      }
      s.selected_ingame_map = current_ingame_map();
    }

    struct tree_user_ctx {
      manager_state* mgr = nullptr;
    };

    auto tree_selected(void* ctx) -> cgwnd* {
      const auto* user = static_cast<const tree_user_ctx*>(ctx);
      return user && user->mgr ? user->mgr->selected : nullptr;
    }

    auto tree_on_select(void* ctx, cgwnd* wnd, int res_key) -> void {
      select_widget(wnd, res_key);
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

      std::string text_utf8 = ext_client::utils::string::to_utf8(info.text);
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

      const int probe_max_id = 0;
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

    auto widget_detail_suffix(const cif_manager::widget_info& info, char* dst, std::size_t dst_count) -> void {
      if (!dst || dst_count == 0) {
        return;
      }
      dst[0] = '\0';
      std::string text_utf8 = ext_client::utils::string::to_utf8(info.text);
      if (info.ddj_path[0] != '\0') {
        std::string_view base = ext_client::utils::string::basename(info.ddj_path);
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
        std::snprintf(label, sizeof(label), "%*s[%s]%s%s", info.depth * 2, "", info.type_name, info.visible ? "" : " [hidden]", detail);
      }

      ImGui::PushID(info.widget);
      const bool selected = s.selected == info.widget;
      if (ImGui::Selectable(label, selected) && is_safe_widget(info.widget)) {
        select_widget(info.widget, info.lookup_res_key);
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("id=%d  %dx%d @ (%d,%d)  %p", info.control_id, info.rect_w, info.rect_h, info.rect_x, info.rect_y, info.widget);
      }
      ImGui::PopID();
    }

    auto draw_toolbar() -> void {
      auto& s = state();
      const auto root = resolve_ui_root();
      s.root_label = root.label;

      ImGui::TextDisabled("Root");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(190.0f);
      int root_mode_i = static_cast<int>(s.root);
      if (ImGui::Combo("##iface_root", &root_mode_i, "Auto\0Active process\0CGInterface\0Ingame res map\0IFace res map\0")) {
        s.root = static_cast<root_mode>(root_mode_i);
        s.search_stale = true;
      }
      ImGui::SameLine();
      ImGui::TextDisabled("%s", root.res_map_view ? s.root_label : (root.wnd ? s.root_label : "(n/a)"));

      ImGui::SameLine(0.0f, 18.0f);
      ImGui::Checkbox("outline on hover", &s.draw_outline);

      ImGui::SameLine(0.0f, 18.0f);
      ImGui::Checkbox("visible only##iface", &s.show_only_visible);
      ImGui::SameLine();
      if (ImGui::Checkbox("static only##iface", &widget_prefs().static_only)) {
        ext_client::menu::ui::setting_changed(false);
        s.search_stale = true;
      }
    }

    auto draw_search_row() -> void {
      auto& s = state();

      ImGui::SetNextItemWidth(-1.0f);
      if (ImGui::InputTextWithHint("##iface_search", "Search (flat scan) — empty = live tree", s.search_text, sizeof(s.search_text))) {
        s.search_stale = true;
      }

      ImGui::SetNextItemWidth(90.0f);
      if (ImGui::InputInt("res id##iface", &s.find_res_id, 0, 0)) {
        s.search_stale = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear##iface_res")) {
        s.find_res_id = 0;
        s.search_stale = true;
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100.0f);
      int visibility = static_cast<int>(s.visibility);
      if (ImGui::Combo("show##iface", &visibility, "all\0shown\0hidden\0")) {
        s.visibility = static_cast<visibility_filter>(visibility);
        s.search_stale = true;
      }
      ImGui::SameLine();
      if (ImGui::Checkbox("hide <no text>##iface", &s.hide_no_text)) {
        s.search_stale = true;
      }

      if (search_active()) {
        ImGui::SameLine();
        if (ImGui::Button("Scan##iface")) {
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
        ImGui::TextDisabled("Root not available — try Auto, CGInterface, or a res map.");
        return;
      }

      ui_browser::recurse_widget_tree(root.wnd, ctx);
    }

    auto lookup_info_for(cgwnd* widget) -> cif_manager::widget_info {
      return ui_browser::lookup_info_for(widget, state().search_results, state().root);
    }

    auto draw_hover_panel() -> void {
      ImGui::TextDisabled("Under mouse (g_CurrentIfUnderCursor)");
      ImGui::Separator();

      cgwnd* target = cif_manager::resolve_hovered_widget(false);
      if (!target) {
        ImGui::TextWrapped("Move the mouse over game UI. Uses game tick hover; click Select hovered to sync pick.");
        return;
      }

      const cgwnd_bounds bounds = target->get_bounds();
      const auto vftable = reinterpret_cast<std::uint32_t>(target->vftable);
      const auto lookup = lookup_info_for(target);

      if (ImGui::BeginTable("iface_hover", 2, ImGuiTableFlags_SizingStretchProp)) {
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

        if (lookup.lookup_res_key >= 0) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("Res key");
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%d%s", lookup.lookup_res_key, lookup.lookup_res_inferred ? " (inferred)" : "");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Bounds");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d, %d  %dx%d", bounds.x, bounds.y, bounds.w, bounds.h);

        if (lookup.res_name[0] != '\0') {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("Res name");
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(lookup.res_name);
        }

        ImGui::EndTable();
      }

      if (ImGui::Button("Select hovered##iface")) {
        if (auto* picked = cif_manager::resolve_hovered_widget(true)) {
          const auto picked_lookup = lookup_info_for(picked);
          select_widget(picked, picked_lookup.lookup_res_key);
        }
      }
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

      if (ImGui::BeginTable("iface_sel", 2, ImGuiTableFlags_SizingStretchProp)) {
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
        ImGui::TextDisabled("Visible");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(s.selected->visible() ? "yes" : "no");

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
          ImGui::Text("%d%s", lookup.lookup_res_key, lookup.lookup_res_inferred ? " (inferred)" : "");
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

        if (lookup.text[0] != L'\0') {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("Text");
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(ext_client::utils::string::to_utf8(lookup.text).c_str());
        }

        ImGui::EndTable();
      }

      if (ImGui::Button("Hide##iface")) {
        cgwnd::set_visible(s.selected, false);
      }
      ImGui::SameLine();
      if (ImGui::Button("Show##iface")) {
        cgwnd::set_visible(s.selected, true);
      }

      const int res_key = s.selected_res_key >= 0 ? s.selected_res_key : lookup.lookup_res_key;
      if (res_key > 0) {
        const bool ingame = s.selected_ingame_map;
        const bool saved = ext_client::hooks::interface_manager::is_hidden_widget(res_key, ingame);
        if (!saved) {
          if (ImGui::Button("Save hide rule##iface")) {
            char label[64]{};
            if (lookup.res_name[0] != '\0') {
              std::snprintf(label, sizeof(label), "%s", lookup.res_name);
            } else {
              std::snprintf(label, sizeof(label), "0x%X", res_key);
            }
            ext_client::hooks::interface_manager::add_hidden_widget(res_key, ingame, label);
          }
        } else {
          if (ImGui::Button("Remove hide rule##iface")) {
            ext_client::hooks::interface_manager::remove_hidden_widget(res_key, ingame);
          }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s 0x%X", ingame ? "ingame" : "iface", res_key);
      }
    }

    auto draw_saved_hides() -> void {
      if (!ImGui::CollapsingHeader("Saved hide rules")) {
        return;
      }

      const auto& mgr = ext_client::config::data().interface_manager;
      if (mgr.hidden_count == 0) {
        ImGui::TextDisabled("No saved rules.");
        return;
      }

      for (int i = 0; i < mgr.hidden_count; ++i) {
        const auto& rule = mgr.hidden[i];
        if (rule.res_key == 0) {
          continue;
        }
        ImGui::PushID(i);
        ImGui::Text("%s 0x%X  %s", rule.ingame_map ? "ingame" : "iface", rule.res_key, rule.label);
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove##iface_rule")) {
          ext_client::hooks::interface_manager::remove_hidden_widget(rule.res_key, rule.ingame_map);
        }
        ImGui::PopID();
      }
    }

    auto draw_res_maps_panel() -> void {
      if (!ImGui::CollapsingHeader("Res maps (quick pick)")) {
        return;
      }

      auto draw_map = [](const char* title, const std::vector<cif_manager::res_map_entry>& entries) {
        if (!ImGui::TreeNode(title)) {
          return;
        }
        if (ImGui::BeginTable("iface_res_map", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
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
              select_widget(entry.wnd, entry.key);
            }
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::Text("%p", entry.wnd);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", cif_manager::unique_id(entry.wnd));
          }
          ImGui::EndTable();
        }
        ImGui::TreePop();
      };

      draw_map("Ingame res map", cif_manager::enumerate_ingame_res_map());
      draw_map("IFace res map", cif_manager::enumerate_iface_res_map());
    }

  } // namespace

  auto draw_quick_hides() -> void {
    auto& s = state();
    s.quick_hides_open = ImGui::CollapsingHeader("Quick hides (promo / alarms)", s.quick_hides_open ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    if (!s.quick_hides_open) {
      return;
    }

    auto& iface = ext_client::config::data().interface_hide;
    ext_client::menu::ui::checkbox_setting("Facebook alarm", &iface.hide_facebook);
    ext_client::menu::ui::checkbox_setting("Magic lamp alarm", &iface.hide_magic_lamp);
    ext_client::menu::ui::checkbox_setting("Daily login alarm", &iface.hide_daily_login);
    ext_client::menu::ui::checkbox_setting("Web item alarm", &iface.hide_web_item_alarm);
    ext_client::menu::ui::checkbox_setting("Macro guide alarm", &iface.hide_macro_guide);
    ext_client::menu::ui::checkbox_setting("Survey button", &iface.hide_survey);
    ext_client::menu::ui::checkbox_setting("Apply on startup", &iface.apply_on_startup);

    if (ext_client::menu::ui::apply_button("Apply quick hides##iface")) {
      ext_client::hooks::interface_hide::apply_from_control();
    }
  }

  auto draw() -> void {
    draw_quick_hides();
    ImGui::Spacing();
    draw_toolbar();
    ImGui::Spacing();
    draw_search_row();
    ImGui::Spacing();

    const float footer_reserve = 220.0f;
    const float tree_h = ext_client::menu::ui::avail_height(footer_reserve, 120.0f);
    ImGui::BeginChild("iface_tree_host", ImVec2(0.0f, tree_h), ImGuiChildFlags_Border);
    draw_widget_browser();
    ImGui::EndChild();

    if (ImGui::BeginTable("iface_footer", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
      ImGui::TableSetupColumn("hover", ImGuiTableColumnFlags_WidthStretch, 0.42f);
      ImGui::TableSetupColumn("selection", ImGuiTableColumnFlags_WidthStretch, 0.58f);
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      if (ImGui::BeginChild("iface_hover", ImVec2(0.0f, 200.0f), ImGuiChildFlags_Border)) {
        draw_hover_panel();
        ImGui::EndChild();
      }

      ImGui::TableSetColumnIndex(1);
      if (ImGui::BeginChild("iface_selection", ImVec2(0.0f, 200.0f), ImGuiChildFlags_Border)) {
        draw_selection_panel();
        draw_saved_hides();
        ImGui::EndChild();
      }

      ImGui::EndTable();
    }

    draw_res_maps_panel();
  }

  auto draw_overlay() -> void {
    if (!state().draw_outline) {
      return;
    }
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
      return;
    }
    if (auto* target = cif_manager::resolve_hovered_widget(false)) {
      ui_browser::draw_widget_glow(target, IM_COL32(255, 200, 64, 255), 2.0f);
    }
  }

} // namespace ext_client::menu::interface_manager
