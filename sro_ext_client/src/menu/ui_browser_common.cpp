#include "menu/ui_browser_common.hpp"

#include "sdk/cg_interface.hpp"
#include "sdk/cgwnd.hpp"

#include <cstdio>

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
