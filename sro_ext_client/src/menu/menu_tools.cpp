#include "ext_client.hpp"
#include "menu/menu_tools.hpp"
#include "menu/menu.hpp"

#include "utils/client_config.hpp"
#include "hooks/cnif_sro_ingame_start_hook.hpp"
#include "menu/interface_manager_runtime.hpp"
#include "hooks/net_hook.hpp"
#include "render/render_system.hpp"
#include "sdk/calarm_guide_mgr_wnd.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cif_manager.hpp"
#include "sdk/cif_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/vectorf.h"
#include "utils/string.hpp"
#include "utils/ui_res_catalog.hpp"

#include <imgui.h>

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

// ===========================================================================
// ui_browser
// ===========================================================================

namespace ext_client::menu::ui_browser {
  namespace {

    auto resolve_auto_root() -> ui_root {
      if (auto* root = cif_manager::resolve_root_from_hover()) {
        return root_from_widget(root);
      }
      return {nullptr, "none", false};
    }

    auto resolve_active_process_root() -> ui_root {
      if (auto* root = cif_manager::game_ui_root()) {
        return root_from_widget(root);
      }
      return {nullptr, "unavailable", false};
    }

    auto visit_recurse_child(cgwnd* child, void* user) -> void {
      auto* ctx = static_cast<tree_recurse_ctx*>(user);
      recurse_widget_tree(child, *ctx);
    }

  } // namespace

  auto root_from_widget(cgwnd* wnd) -> ui_root {
    if (!cif_manager::is_walkable_root(wnd)) {
      return {nullptr, "unavailable", false};
    }
    auto* top = cif_manager::topmost_ui_ancestor(wnd);
    if (!top) {
      return {nullptr, "unavailable", false};
    }
    return {top, cif_manager::ui_type_name(top), false};
  }

  auto resolve_ui_root(root_mode mode, const root_labels& labels) -> ui_root {
    if (mode == root_mode::auto_detect) {
      return resolve_auto_root();
    }

    switch (mode) {
      case root_mode::active_process:
        return resolve_active_process_root();
      case root_mode::cg_interface:
        if (auto* iface = cg_interface::get(); iface != nullptr && cg_interface::is_instance(iface)) {
          return root_from_widget(reinterpret_cast<cgwnd*>(iface));
        }
        break;
      case root_mode::ingame_res_map:
        return {nullptr, labels.ingame_res_map, true};
      case root_mode::iface_res_map:
        return {nullptr, labels.iface_res_map, true};
      default:
        break;
    }

    return {nullptr, "unavailable", false};
  }

  auto enumerate_browse_roots(root_mode mode) -> std::vector<cgwnd*> {
    std::vector<cgwnd*> roots;

    if (mode == root_mode::ingame_res_map) {
      for (const auto& entry : cif_manager::enumerate_ingame_res_map()) {
        if (entry.wnd != nullptr) {
          roots.push_back(entry.wnd);
        }
      }
      return roots;
    }

    if (mode == root_mode::iface_res_map) {
      for (const auto& entry : cif_manager::enumerate_iface_res_map()) {
        if (entry.wnd != nullptr) {
          roots.push_back(entry.wnd);
        }
      }
      return roots;
    }

    const auto root = resolve_ui_root(mode);
    if (root.wnd != nullptr) {
      roots.push_back(root.wnd);
    }
    return roots;
  }

  auto recurse_widget_tree(cgwnd* elem, tree_recurse_ctx& ctx) -> void {
    if (!elem || !cif_manager::is_live_widget(elem) || !ctx.seen) {
      return;
    }
    if (!ctx.seen->insert(elem).second) {
      return;
    }
    if (ctx.show_only_visible && !cgwnd::is_pickable(elem)) {
      return;
    }

    if (ctx.static_only && !cif_manager::is_static(elem)) {
      cif_manager::for_each_owned_child(elem, visit_recurse_child, &ctx);
      return;
    }

    const auto vftable = reinterpret_cast<std::uint32_t>(elem->vftable);
    const char* type_name = cif_manager::vftable_type_name(vftable);
    const int runtime_id = cif_manager::control_id(elem);
    const std::size_t child_count = cif_manager::count_owned_children(elem);

    char label[128]{};
    std::snprintf(label, sizeof(label), "%s [%d] [+%zu]", type_name, runtime_id, child_count);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (ctx.hooks.selected && ctx.hooks.selected(ctx.hooks.ctx) == elem) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (child_count == 0) {
      flags |= ImGuiTreeNodeFlags_Leaf;
    }

    ImGui::PushID(elem);
    const bool open = ImGui::TreeNodeEx("##node", flags, "%s", label);
    if (ImGui::IsItemClicked() && ctx.hooks.on_select) {
      ctx.hooks.on_select(ctx.hooks.ctx, elem, -1);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%dx%d @ (%d,%d)  %p", elem->rect_w(), elem->rect_h(), elem->rect_x(), elem->rect_y(), elem);
    }
    if (open) {
      cif_manager::for_each_owned_child(elem, visit_recurse_child, &ctx);
      ImGui::TreePop();
    }
    ImGui::PopID();
  }

  auto draw_res_map_roots_tree(const std::vector<cif_manager::res_map_entry>& entries, tree_recurse_ctx& ctx) -> void {
    if (entries.empty()) {
      ImGui::TextDisabled("Res map empty or unreadable.");
      return;
    }

    for (const auto& entry : entries) {
      if (!entry.wnd || !cif_manager::is_live_widget(entry.wnd)) {
        continue;
      }

      const auto vftable = reinterpret_cast<std::uint32_t>(entry.wnd->vftable);
      const char* type_name = cif_manager::vftable_type_name(vftable);
      const std::size_t child_count = cif_manager::count_owned_children(entry.wnd);

      char label[128]{};
      std::snprintf(label, sizeof(label), "0x%X %s [%d] [+%zu]", entry.key, type_name, cif_manager::control_id(entry.wnd), child_count);

      ImGui::PushID(entry.wnd);
      ImGui::PushID(entry.key);
      const bool open = ImGui::TreeNodeEx("##map_root", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow, "%s", label);
      if (ImGui::IsItemClicked() && ctx.hooks.on_select) {
        ctx.hooks.on_select(ctx.hooks.ctx, entry.wnd, entry.key);
      }
      if (open) {
        recurse_widget_tree(entry.wnd, ctx);
        ImGui::TreePop();
      }
      ImGui::PopID();
      ImGui::PopID();
    }
  }

  auto lookup_info_for(cgwnd* widget,
                       const std::vector<cif_manager::widget_info>& search_results,
                       root_mode mode) -> cif_manager::widget_info {
    for (const auto& item : search_results) {
      if (item.widget == widget) {
        return item;
      }
    }

    cif_manager::widget_info info{};
    info.widget = widget;
    info.control_id = cif_manager::control_id(widget);
    const auto roots = enumerate_browse_roots(mode);
    cif_manager::enrich_widget_info(info, roots.empty() ? nullptr : roots.front());
    return info;
  }

  auto draw_widget_glow(cgwnd* widget, ImU32 color, float thickness) -> void {
    if (!widget) {
      return;
    }

    const cgwnd_bounds bounds = widget->get_bounds();
    if (bounds.w <= 0 || bounds.h <= 0) {
      return;
    }

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    dl->PushClipRect(viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y), true);
    const ImVec2 tl(static_cast<float>(bounds.x), static_cast<float>(bounds.y));
    const ImVec2 br(static_cast<float>(bounds.x + bounds.w), static_cast<float>(bounds.y + bounds.h));
    const ImU32 glow = (color & 0x00FFFFFFu) | 0x40000000u;
    dl->AddRect(tl, br, glow, 0.0f, 0, thickness + 2.0f);
    dl->AddRect(tl, br, color, 0.0f, 0, thickness);
    dl->PopClipRect();
  }

} // namespace ext_client::menu::ui_browser

// ===========================================================================
// interface_manager
// ===========================================================================

namespace ext_client::menu {
  namespace {

    enum class visibility_filter : int {
      all = 0,
      shown = 1,
      hidden = 2,
    };

    using root_mode = ui_browser::root_mode;
    using ui_root = ui_browser::ui_root;

  } // namespace

  struct interface_manager::state {
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

  auto interface_manager::get() -> interface_manager& {
    static interface_manager instance;
    return instance;
  }

  auto interface_manager::s() -> state& {
    static state s;
    return s;
  }

  namespace {

    auto& mgr_state() {
      return interface_manager::get().s();
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
      return ui_browser::resolve_ui_root(mgr_state().root);
    }

    auto current_ingame_map() -> bool {
      const auto& s = mgr_state();
      if (s.root == root_mode::ingame_res_map) {
        return true;
      }
      if (s.root == root_mode::iface_res_map) {
        return false;
      }
      return cg_interface::is_ingame_hud_ready();
    }

    auto enumerate_browse_roots() -> std::vector<cgwnd*> {
      return ui_browser::enumerate_browse_roots(mgr_state().root);
    }

    auto select_widget(cgwnd* widget, int res_key = -1) -> void {
      auto& s = mgr_state();
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
      interface_manager::state* mgr = nullptr;
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
      auto& s = mgr_state();
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
      const auto& s = mgr_state();
      return s.search_text[0] != '\0' || s.find_res_id > 0;
    }

    auto run_search_scan() -> void {
      auto& s = mgr_state();
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
      auto& s = mgr_state();
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
      auto& s = mgr_state();
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
      auto& s = mgr_state();

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
      auto& s = mgr_state();
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
      return ui_browser::lookup_info_for(widget, mgr_state().search_results, mgr_state().root);
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
      auto& s = mgr_state();
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
        const bool saved = ext_client::menu::interface_manager_runtime::is_hidden_widget(res_key, ingame);
        if (!saved) {
          if (ImGui::Button("Save hide rule##iface")) {
            char label[64]{};
            if (lookup.res_name[0] != '\0') {
              std::snprintf(label, sizeof(label), "%s", lookup.res_name);
            } else {
              std::snprintf(label, sizeof(label), "0x%X", res_key);
            }
            ext_client::menu::interface_manager_runtime::add_hidden_widget(res_key, ingame, label);
          }
        } else {
          if (ImGui::Button("Remove hide rule##iface")) {
            ext_client::menu::interface_manager_runtime::remove_hidden_widget(res_key, ingame);
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
          ext_client::menu::interface_manager_runtime::remove_hidden_widget(rule.res_key, rule.ingame_map);
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

  auto interface_manager::draw_quick_hides() -> void {
    auto& s = mgr_state();
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
    ext_client::menu::ui::checkbox_setting("Apply on startup", &iface.apply_on_startup);

    if (ext_client::menu::ui::apply_button("Apply quick hides##iface")) {
      ext_client::hooks::cnif_sro_ingame_start_hook::apply_from_control();
    }
  }

  auto interface_manager::draw() -> void {
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

  auto interface_manager::draw_overlay() -> void {
    if (!mgr_state().draw_outline) {
      return;
    }
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
      return;
    }
    if (auto* target = cif_manager::resolve_hovered_widget(false)) {
      ui_browser::draw_widget_glow(target, IM_COL32(255, 200, 64, 255), 2.0f);
    }
  }

} // namespace ext_client::menu

// ===========================================================================
// widget_inspector
// ===========================================================================

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
