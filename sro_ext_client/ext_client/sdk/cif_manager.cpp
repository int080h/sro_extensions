#include "cif_manager.hpp"

#include "calarm_guide_mgr_wnd.hpp"
#include "cg_interface.hpp"
#include "cif_decorated_static.hpp"
#include "cif_static.hpp"
#include "cps_character_select.hpp"
#include "cps_outer_interface.hpp"
#include "cps_title.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/ui_res_catalog.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cif_manager = ext_client::cif_manager;

namespace {

  using ext_client::offsets::as_fn;

  auto try_read_msvc_rtti_class_name(std::uint32_t vftable, char* dst, std::size_t dst_count) -> bool;

  auto is_safe_widget(const cgwnd* wnd) -> bool {
    if (!ext_client::msvc9::is_readable_ptr(wnd, ext_client::offsets::cgwnd::fields::child_list)) {
      return false;
    }
    const auto vftable = wnd->vftable;
    return ext_client::msvc9::is_readable_ptr(vftable, sizeof(void*) * 8);
  }

  auto is_cps_outer_process(const void* obj) -> bool {
    if (!obj || !ext_client::msvc9::is_readable_ptr(obj, sizeof(std::uint32_t))) {
      return false;
    }
    std::uint32_t vft = 0;
    if (!ext_client::msvc9::try_read_u32(obj, &vft)) {
      return false;
    }
    char name[64]{};
    if (!try_read_msvc_rtti_class_name(vft, name, sizeof(name))) {
      return false;
    }
    return std::strncmp(name, "CPS", 3) == 0;
  }

  auto has_outer_res_map(const cgwnd* root) -> bool {
    if (!root || !is_cps_outer_process(root)) {
      return false;
    }
    const auto* map = reinterpret_cast<const std::uint8_t*>(root) + ext_client::offsets::cps_silkroad::fields::res_ui_root;
    return ext_client::msvc9::is_readable_ptr(map, ext_client::msvc9::ui_res_map_size);
  }

  auto has_parsed_res_catalog(const cgwnd* root) -> bool {
    if (!root) {
      return false;
    }
    const auto vftable = reinterpret_cast<std::uint32_t>(root->vftable);
    return vftable == ext_client::offsets::cps_title::vtable::address ||
           vftable == ext_client::offsets::cps_character_select::vtable::address;
  }

  auto catalog_screen_for_root(const cgwnd* root) -> ext_client::utils::ui_res_catalog::screen;

  auto child_list_sentinel(const cgwnd* wnd) -> const void* {
    if (!wnd) {
      return nullptr;
    }
    using ext_client::offsets::field_at;
    if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const std::uint8_t*>(wnd) + ext_client::offsets::cgwnd::fields::child_list,
                                            sizeof(void*))) {
      return nullptr;
    }
    const auto* sentinel = field_at<const void*>(wnd, ext_client::offsets::cgwnd::fields::child_list);
    return ext_client::msvc9::is_readable_ptr(sentinel, ext_client::msvc9::child_list_sentinel_size) ? sentinel : nullptr;
  }

  auto embedded_res_map_ptr(cps_outer_interface* outer) -> void* {
    if (!outer) {
      return nullptr;
    }
    return reinterpret_cast<std::uint8_t*>(outer) + ext_client::offsets::cps_silkroad::fields::res_ui_root;
  }

  auto cg_interface_res_map_ptr(const cgwnd* root) -> const void* {
    if (!cg_interface::is_instance(root)) {
      return nullptr;
    }
    return reinterpret_cast<const cg_interface*>(root)->ui_res_map();
  }

  auto if_wnd_res_map_ptr(const cgwnd* wnd) -> const void* {
    if (!wnd || cg_interface::is_instance(wnd)) {
      return nullptr;
    }

    const auto vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
    if (vftable == ext_client::offsets::cif_static::vtable::address || vftable == ext_client::offsets::cif_static::vtable::secondary ||
        vftable == ext_client::offsets::cif_button::vtable::address || vftable == ext_client::offsets::cif_edit::vtable::address ||
        vftable == ext_client::offsets::cif_decorated_static::vtable::address ||
        vftable == ext_client::offsets::cif_decorated_static::vtable::secondary ||
        vftable == ext_client::offsets::cif_facebook_link_alram::vtable::address ||
        vftable == ext_client::offsets::cif_facebook_link_alram::vtable::secondary ||
        vftable == ext_client::offsets::cif_magic_lamp_alram::vtable::address ||
        vftable == ext_client::offsets::cif_magic_lamp_alram::vtable::secondary ||
        vftable == ext_client::offsets::cif_daily_login_alram::vtable::address ||
        vftable == ext_client::offsets::cif_daily_login_alram::vtable::secondary) {
      return nullptr;
    }

    if (!ext_client::msvc9::is_readable_ptr(wnd, ext_client::offsets::cif_wnd::fields::ui_res_map + ext_client::msvc9::ui_res_map_size)) {
      return nullptr;
    }

    const auto* map = reinterpret_cast<const std::uint8_t*>(wnd) + ext_client::offsets::cif_wnd::fields::ui_res_map;
    return ext_client::msvc9::is_readable_ptr(map, ext_client::msvc9::ui_res_map_size) ? map : nullptr;
  }

  auto walk_if_wnd_res_map_children(const void* map_obj,
                                    cgwnd* parent,
                                    int depth,
                                    int max_depth,
                                    std::vector<cif_manager::widget_info>& out,
                                    std::unordered_set<cgwnd*>& seen) -> void;

  auto walk_child_list_children(const void* sentinel,
                                cgwnd* parent,
                                int depth,
                                int max_depth,
                                std::vector<cif_manager::widget_info>& out,
                                std::unordered_set<cgwnd*>& seen) -> void;

  auto walk_res_map_children(const void* map_obj,
                             cgwnd* parent,
                             int depth,
                             int max_depth,
                             std::vector<cif_manager::widget_info>& out,
                             std::unordered_set<cgwnd*>& seen) -> void;

  auto walk_each_child_list_children(const void* sentinel,
                                     cgwnd* parent,
                                     int depth,
                                     int max_depth,
                                     std::unordered_set<cgwnd*>& seen,
                                     cif_manager::walk_visitor_fn visit,
                                     void* ctx) -> void;

  auto walk_each_res_map_children(const void* map_obj,
                                  cgwnd* parent,
                                  int depth,
                                  int max_depth,
                                  std::unordered_set<cgwnd*>& seen,
                                  cif_manager::walk_visitor_fn visit,
                                  void* ctx) -> void;

  auto
  walk_child_subtrees(cgwnd* child, int depth, int max_depth, std::vector<cif_manager::widget_info>& out, std::unordered_set<cgwnd*>& seen)
    -> void;

  // 0 = no cap. Walks are bounded by depth + a seen-set (each live widget once).
  constexpr std::size_t k_default_walk_widget_budget = 0;
  thread_local std::size_t g_walk_widget_budget = k_default_walk_widget_budget;

  auto walk_each_visit_unique(cgwnd* wnd, std::unordered_set<cgwnd*>& seen, cif_manager::walk_visitor_fn visit, void* ctx) -> void {
    if (!wnd || !visit || !cif_manager::is_live_widget(wnd)) {
      return;
    }
    if (!seen.insert(wnd).second) {
      return;
    }
    visit(wnd, ctx);
  }

  auto push_widget(std::vector<cif_manager::widget_info>& out,
                   cgwnd* wnd,
                   int depth,
                   int max_depth,
                   std::unordered_set<cgwnd*>& seen,
                   int res_map_key = -1) -> void {
    if (g_walk_widget_budget > 0 && out.size() >= g_walk_widget_budget) {
      return;
    }
    if (!is_safe_widget(wnd) || !seen.insert(wnd).second) {
      return;
    }

    cif_manager::widget_info info{};
    info.widget = wnd;
    info.depth = depth;
    info.control_id = cif_manager::control_id(wnd);
    info.res_map_key = res_map_key;
    info.rect_x = wnd->rect_x();
    info.rect_y = wnd->rect_y();
    info.rect_w = wnd->rect_w();
    info.rect_h = wnd->rect_h();
    info.visible = cif_manager::is_visible(wnd);
    info.vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
    info.kind = cif_manager::vftable_kind(info.vftable);

    const char* type_name = cif_manager::vftable_type_name(info.vftable);
    std::strncpy(info.type_name, type_name, sizeof(info.type_name) - 1);

    if (cif_manager::is_static(wnd)) {
      cif_manager::read_static_text(cif_static::from(wnd), info.text, 256);
    }

    out.push_back(info);

    if (depth < max_depth && calarm_guide_mgr_wnd::is_instance(wnd)) {
      auto* mgr = calarm_guide_mgr_wnd::from(wnd);
      for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
        if (auto* slot = calarm_guide_mgr_wnd::loose_slot_widget(mgr, i); is_safe_widget(slot)) {
          push_widget(out, slot, depth + 1, max_depth, seen);
          walk_child_subtrees(slot, depth + 1, max_depth, out, seen);
        }
      }
      for (std::size_t i = 0; i < 3; ++i) {
        if (auto* effect = calarm_guide_mgr_wnd::loose_effect_widget(mgr, i); is_safe_widget(effect)) {
          push_widget(out, effect, depth + 1, max_depth, seen);
          walk_child_subtrees(effect, depth + 1, max_depth, out, seen);
        }
      }
    }
  }

  auto
  walk_child_subtrees(cgwnd* child, int depth, int max_depth, std::vector<cif_manager::widget_info>& out, std::unordered_set<cgwnd*>& seen)
    -> void {
    walk_child_list_children(child_list_sentinel(child), child, depth + 1, max_depth, out, seen);
    if (auto* nested_res = cg_interface_res_map_ptr(child)) {
      walk_res_map_children(nested_res, child, depth + 1, max_depth, out, seen);
    }
    if (auto* if_map = if_wnd_res_map_ptr(child)) {
      walk_if_wnd_res_map_children(if_map, child, depth + 1, max_depth, out, seen);
    }
  }

  auto walk_child_list_children(const void* sentinel,
                                cgwnd* parent,
                                int depth,
                                int max_depth,
                                std::vector<cif_manager::widget_info>& out,
                                std::unordered_set<cgwnd*>& seen) -> void {
    if (!ext_client::msvc9::is_game_ptr(sentinel) || depth > max_depth) {
      return;
    }

    ext_client::msvc9::child_list_ref::from_sentinel(sentinel).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!cif_manager::is_live_widget(child) || child == parent) {
        return;
      }
      push_widget(out, child, depth, max_depth, seen);
      walk_child_subtrees(child, depth, max_depth, out, seen);
    });
  }

  auto walk_if_wnd_res_map_children(const void* map_obj,
                                    cgwnd* parent,
                                    int depth,
                                    int max_depth,
                                    std::vector<cif_manager::widget_info>& out,
                                    std::unordered_set<cgwnd*>& seen) -> void {
    if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size) || depth > max_depth) {
      return;
    }

    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t key, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!ext_client::msvc9::is_game_ptr(child) || child == parent) {
        return;
      }
      push_widget(out, child, depth, max_depth, seen, static_cast<int>(key));
      walk_child_subtrees(child, depth, max_depth, out, seen);
    });
  }

  auto walk_res_map_children(const void* map_obj,
                             cgwnd* parent,
                             int depth,
                             int max_depth,
                             std::vector<cif_manager::widget_info>& out,
                             std::unordered_set<cgwnd*>& seen) -> void {
    if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size) || depth > max_depth) {
      return;
    }

    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t key, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!ext_client::msvc9::is_game_ptr(child) || child == parent) {
        return;
      }
      push_widget(out, child, depth, max_depth, seen, static_cast<int>(key));
      walk_child_subtrees(child, depth, max_depth, out, seen);
    });
  }

  auto walk_each_if_wnd_res_map_children(const void* map_obj,
                                         cgwnd* parent,
                                         int depth,
                                         int max_depth,
                                         std::unordered_set<cgwnd*>& seen,
                                         cif_manager::walk_visitor_fn visit,
                                         void* ctx) -> void;

  auto append_discovered_child(
    cgwnd* child, int depth, int max_depth, std::vector<cif_manager::widget_info>& out, std::unordered_set<cgwnd*>& seen) -> void {
    if (!is_safe_widget(child)) {
      return;
    }
    const auto before = seen.size();
    push_widget(out, child, depth, max_depth, seen);
    if (seen.size() != before) {
      walk_child_subtrees(child, depth, max_depth, out, seen);
    }
  }

  auto append_cginterface_extras(cgwnd* root,
                                 int max_depth,
                                 int cginterface_probe_max_id,
                                 std::vector<cif_manager::widget_info>& out,
                                 std::unordered_set<cgwnd*>& seen) -> void {
    if (!cg_interface::is_instance(root)) {
      return;
    }

    auto* iface = reinterpret_cast<cg_interface*>(root);
    const int extras_depth = max_depth < 6 ? max_depth : 6;

    if (auto* mgr = iface->alarm_guide_mgr()) {
      append_discovered_child(reinterpret_cast<cgwnd*>(mgr), 1, extras_depth, out, seen);
    }

    if (cginterface_probe_max_id > 0) {
      for (int control_id = 1; control_id <= cginterface_probe_max_id; ++control_id) {
        if (g_walk_widget_budget > 0 && out.size() >= g_walk_widget_budget) {
          break;
        }
        if (auto* child = iface->get_ui_child(control_id, true)) {
          append_discovered_child(reinterpret_cast<cgwnd*>(child), 1, extras_depth, out, seen);
        }
      }
    } else {
      for (std::size_t i = 0; i < cg_interface::known_child_id_count(); ++i) {
        if (g_walk_widget_budget > 0 && out.size() >= g_walk_widget_budget) {
          break;
        }
        const int control_id = cg_interface::known_child_id(i);
        if (control_id <= 0) {
          continue;
        }
        if (auto* child = iface->get_ui_child(control_id, true)) {
          append_discovered_child(reinterpret_cast<cgwnd*>(child), 1, extras_depth, out, seen);
        }
      }
    }

    if (auto* popup = iface->alarm_guide_mgr_popup()) {
      append_discovered_child(reinterpret_cast<cgwnd*>(popup), 1, extras_depth, out, seen);
    }
  }

  auto walk_each_child_subtrees(
    cgwnd* child, int depth, int max_depth, std::unordered_set<cgwnd*>& seen, cif_manager::walk_visitor_fn visit, void* ctx) -> void {
    walk_each_child_list_children(child_list_sentinel(child), child, depth + 1, max_depth, seen, visit, ctx);
    if (auto* nested_res = cg_interface_res_map_ptr(child)) {
      walk_each_res_map_children(nested_res, child, depth + 1, max_depth, seen, visit, ctx);
    }
    if (auto* if_map = if_wnd_res_map_ptr(child)) {
      walk_each_if_wnd_res_map_children(if_map, child, depth + 1, max_depth, seen, visit, ctx);
    }
  }

  auto walk_each_child_list_children(const void* sentinel,
                                     cgwnd* parent,
                                     int depth,
                                     int max_depth,
                                     std::unordered_set<cgwnd*>& seen,
                                     cif_manager::walk_visitor_fn visit,
                                     void* ctx) -> void {
    if (!ext_client::msvc9::is_game_ptr(sentinel) || depth > max_depth || !visit) {
      return;
    }

    ext_client::msvc9::child_list_ref::from_sentinel(sentinel).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!cif_manager::is_live_widget(child) || child == parent) {
        return;
      }
      walk_each_visit_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, depth, max_depth, seen, visit, ctx);
    });
  }

  auto walk_each_if_wnd_res_map_children(const void* map_obj,
                                         cgwnd* parent,
                                         int depth,
                                         int max_depth,
                                         std::unordered_set<cgwnd*>& seen,
                                         cif_manager::walk_visitor_fn visit,
                                         void* ctx) -> void {
    if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size) || depth > max_depth || !visit) {
      return;
    }

    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!ext_client::msvc9::is_game_ptr(child) || child == parent) {
        return;
      }
      walk_each_visit_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, depth, max_depth, seen, visit, ctx);
    });
  }

  auto walk_each_res_map_children(const void* map_obj,
                                  cgwnd* parent,
                                  int depth,
                                  int max_depth,
                                  std::unordered_set<cgwnd*>& seen,
                                  cif_manager::walk_visitor_fn visit,
                                  void* ctx) -> void {
    if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size) || depth > max_depth || !visit) {
      return;
    }

    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!ext_client::msvc9::is_game_ptr(child) || child == parent) {
        return;
      }
      walk_each_visit_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, depth, max_depth, seen, visit, ctx);
    });
  }

  auto walk_each_gwnd_recursive(
    cgwnd* root, int depth, int max_depth, std::unordered_set<cgwnd*>& seen, cif_manager::walk_visitor_fn visit, void* ctx) -> void {
    if (!ext_client::msvc9::is_game_ptr(root) || depth > max_depth || !visit) {
      return;
    }

    walk_each_child_list_children(child_list_sentinel(root), root, depth, max_depth, seen, visit, ctx);
  }

  auto
  walk_gwnd_recursive(cgwnd* root, int depth, int max_depth, std::vector<cif_manager::widget_info>& out, std::unordered_set<cgwnd*>& seen)
    -> void {
    if (!root || depth > max_depth) {
      return;
    }

    walk_child_list_children(child_list_sentinel(root), root, depth, max_depth, out, seen);
  }

  std::unordered_map<std::uint32_t, std::string> g_rtti_name_cache;

  auto try_read_msvc_rtti_class_name(std::uint32_t vftable, char* dst, std::size_t dst_count) -> bool {
    if (!dst || dst_count < 2 || vftable < 0x10000u) {
      return false;
    }

    const auto* vt = reinterpret_cast<const void* const*>(static_cast<std::uintptr_t>(vftable));
    if (!ext_client::msvc9::is_readable_ptr(vt - 1, sizeof(void*))) {
      return false;
    }

    const void* col = vt[-1];
    if (!col || !ext_client::msvc9::is_readable_ptr(col, 16)) {
      return false;
    }

    const auto* col_bytes = reinterpret_cast<const std::uint8_t*>(col);
    if (!ext_client::msvc9::is_readable_ptr(col_bytes + 12, sizeof(void*))) {
      return false;
    }
    const void* type_desc = *reinterpret_cast<const void* const*>(col_bytes + 12);
    if (!type_desc || !ext_client::msvc9::is_readable_ptr(type_desc, 12)) {
      return false;
    }

    const char* mangled = reinterpret_cast<const char*>(reinterpret_cast<const std::uint8_t*>(type_desc) + 8);
    if (!ext_client::msvc9::is_readable_ptr(mangled, 5)) {
      return false;
    }

    const char* class_start = nullptr;
    if (std::strncmp(mangled, ".?AV", 4) == 0) {
      class_start = mangled + 4;
    } else if (std::strncmp(mangled, ".?AU", 4) == 0) {
      class_start = mangled + 4;
    } else {
      return false;
    }

    const char* class_end = std::strstr(class_start, "@@");
    const std::size_t len = class_end != nullptr ? static_cast<std::size_t>(class_end - class_start) : std::strlen(class_start);
    if (len == 0 || len + 1 > dst_count) {
      return false;
    }

    std::memcpy(dst, class_start, len);
    dst[len] = '\0';
    return true;
  }

} // namespace

namespace ext_client::cif_manager {

  auto kind_name(widget_kind kind) -> const char* {
    switch (kind) {
      case widget_kind::cif_static:
        return "CIFStatic";
      case widget_kind::cif_facebook_link_alarm:
        return "CIFFacebookLinkAlram";
      case widget_kind::cif_magic_lamp_alarm:
        return "CIFMagicLampAlram";
      case widget_kind::cif_daily_login_alarm:
        return "CIFDailyLoginAlram";
      case widget_kind::cif_button:
        return "CIFButton";
      case widget_kind::cif_edit:
        return "CIFEdit";
      case widget_kind::cif_decorated_static:
        return "CIFDecoratedStatic";
      case widget_kind::cif_wnd:
        return "CIFWnd";
      case widget_kind::calarm_guide_mgr:
        return "CAlramGuideMgrWnd";
      case widget_kind::cg_interface:
        return "CGInterface";
      case widget_kind::cps_title:
        return "CPSTitle";
      case widget_kind::cps_version_check:
        return "CPSVersionCheck";
      case widget_kind::cps_character_select:
        return "CPSCharacterSelect";
      case widget_kind::cgwnd:
        return "CGWnd";
      default:
        return "unknown";
    }
  }

  auto vftable_kind(std::uint32_t vftable) -> widget_kind {
    switch (vftable) {
      case ext_client::offsets::cif_static::vtable::address:
        return widget_kind::cif_static;
      case ext_client::offsets::cif_facebook_link_alram::vtable::address:
        return widget_kind::cif_facebook_link_alarm;
      case ext_client::offsets::cif_magic_lamp_alram::vtable::address:
        return widget_kind::cif_magic_lamp_alarm;
      case ext_client::offsets::cif_daily_login_alram::vtable::address:
        return widget_kind::cif_daily_login_alarm;
      case ext_client::offsets::cif_decorated_static::vtable::address:
      case ext_client::offsets::cif_decorated_static::vtable::secondary:
        return widget_kind::cif_decorated_static;
      case ext_client::offsets::cif_wnd::vtable::address:
        return widget_kind::cif_wnd;
      case ext_client::offsets::cif_button::vtable::address:
      case ext_client::offsets::cnif_button::vtable::address:
      case ext_client::offsets::cnif_button::vtable::secondary:
        return widget_kind::cif_button;
      case ext_client::offsets::cnif_sro_ingame_start::vtable::address:
      case ext_client::offsets::cnif_sro_ingame_start::vtable::secondary:
        return widget_kind::cgwnd;
      case ext_client::offsets::cnif_sro_ingame_info::vtable::address:
      case ext_client::offsets::cnif_sro_ingame_info::vtable::secondary:
        return widget_kind::cgwnd;
      case ext_client::offsets::cif_edit::vtable::address:
        return widget_kind::cif_edit;
      case ext_client::offsets::calarm_guide_mgr_wnd::vtable::address:
        return widget_kind::calarm_guide_mgr;
      case ext_client::offsets::cg_interface::vtable::address:
      case ext_client::offsets::cg_interface::vtable::secondary:
        return widget_kind::cg_interface;
      case ext_client::offsets::cps_title::vtable::address:
        return widget_kind::cps_title;
      case ext_client::offsets::cps_version_check::vtable::address:
        return widget_kind::cps_version_check;
      case ext_client::offsets::cps_character_select::vtable::address:
        return widget_kind::cps_character_select;
      case ext_client::offsets::cgwnd::vtable::cgwnd:
        return widget_kind::cgwnd;
      default:
        return widget_kind::unknown;
    }
  }

  auto vftable_type_name(std::uint32_t vftable) -> const char* {
    if (const auto kind = vftable_kind(vftable); kind != widget_kind::unknown) {
      return kind_name(kind);
    }

    if (auto found = g_rtti_name_cache.find(vftable); found != g_rtti_name_cache.end()) {
      return found->second.c_str();
    }

    char parsed[64]{};
    if (!try_read_msvc_rtti_class_name(vftable, parsed, sizeof(parsed))) {
      return "unknown";
    }

    auto [it, _] = g_rtti_name_cache.emplace(vftable, parsed);
    return it->second.c_str();
  }

  auto is_plain_static(const cgwnd* wnd) -> bool {
    if (!wnd) {
      return false;
    }
    const auto vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
    return vftable == ext_client::offsets::cif_static::vtable::address || vftable == ext_client::offsets::cif_static::vtable::secondary;
  }

  auto is_static(const cgwnd* wnd) -> bool {
    if (!wnd) {
      return false;
    }
    if (is_plain_static(wnd)) {
      return true;
    }
    const auto vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
    return vftable == ext_client::offsets::cif_decorated_static::vtable::address ||
           vftable == ext_client::offsets::cif_decorated_static::vtable::secondary;
  }

  auto static_text_object(cgwnd* wnd) -> cif_static* {
    if (!wnd) {
      return nullptr;
    }

    const auto vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
    if (vftable == ext_client::offsets::cif_decorated_static::vtable::address ||
        vftable == ext_client::offsets::cif_decorated_static::vtable::secondary) {
      return reinterpret_cast<cif_static*>(reinterpret_cast<std::uint8_t*>(wnd) + ext_client::offsets::cif_static::subobject_offset);
    }

    return cif_static::from(wnd);
  }

  auto as_static_if(cgwnd* wnd) -> cif_static* {
    if (!wnd) {
      return nullptr;
    }
    if (is_plain_static(wnd) || is_static(wnd)) {
      return static_text_object(wnd);
    }
    if (!is_live_widget(wnd)) {
      return nullptr;
    }

    const auto vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
    const char* type_name = vftable_type_name(vftable);
    if (std::strstr(type_name, "IFStatic") == nullptr) {
      return nullptr;
    }

    return static_text_object(wnd);
  }

  auto is_same_res_type(const void* res_descriptor, const void* expected) -> bool {
    // Resource type nodes (dword_1179970, etc.) are globals — compare addresses only.
    return res_descriptor != nullptr && res_descriptor == expected;
  }

  auto control_id(const cgwnd* wnd) -> int {
    if (!wnd) {
      return 0;
    }
    return wnd->control_id();
  }

  auto unique_id(const cgwnd* wnd) -> int {
    if (!wnd) {
      return 0;
    }
    return wnd->unique_id();
  }

  auto is_visible(const cgwnd* wnd) -> bool {
    if (!wnd) {
      return false;
    }
    return wnd->visible();
  }

  auto ui_type_name(const void* obj) -> const char* {
    if (!obj) {
      return "none";
    }
    std::uint32_t vft = 0;
    if (!ext_client::msvc9::try_read_u32(obj, &vft)) {
      return "unknown";
    }
    return vftable_type_name(vft);
  }

  auto browsable_ui_root(void* obj) -> cgwnd* {
    if (!obj || !ext_client::msvc9::is_readable_ptr(obj, sizeof(void*) * 4)) {
      return nullptr;
    }

    auto* wnd = reinterpret_cast<cgwnd*>(obj);
    if (cg_interface::is_instance(obj) && is_live_widget(wnd)) {
      return wnd;
    }
    if (has_outer_res_map(wnd)) {
      return wnd;
    }
    if (is_live_widget(wnd)) {
      return wnd;
    }
    return nullptr;
  }

  auto is_live_widget(const cgwnd* wnd) -> bool {
    if (!is_safe_widget(wnd)) {
      return false;
    }

    std::uint32_t vft = 0;
    if (!ext_client::msvc9::try_read_u32(wnd, &vft)) {
      return false;
    }
    if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(vft)), sizeof(void*) * 4)) {
      return false;
    }
    if (vftable_kind(vft) != widget_kind::unknown) {
      return true;
    }
    if (has_outer_res_map(wnd)) {
      return true;
    }
    char scratch[4]{};
    return try_read_msvc_rtti_class_name(vft, scratch, sizeof(scratch));
  }

  auto read_static_text(const cif_static* label, wchar_t* dst, std::size_t dst_count) -> bool {
    if (!label || !dst || dst_count == 0) {
      return false;
    }

    const auto try_read = [&](const cif_static* candidate) -> bool {
      if (!candidate) {
        return false;
      }
      const auto* text_obj = reinterpret_cast<const std::uint8_t*>(candidate) + ext_client::offsets::cif_static::fields::text_wstring;
      return ext_client::msvc9::wstring_ref::from(text_obj).copy_to(dst, dst_count);
    };

    if (try_read(label)) {
      return true;
    }

    const auto* outer = reinterpret_cast<const cgwnd*>(label);
    if (!is_live_widget(outer)) {
      return false;
    }
    return try_read(as_static_if(const_cast<cgwnd*>(outer)));
  }

  auto find_ui_child(cps_outer_interface* outer, int control_id, bool add_base_key) -> cgwnd* {
    if (!outer) {
      return nullptr;
    }
    return static_cast<cgwnd*>(outer->get_ui_child(control_id, add_base_key));
  }

  auto find_ui_child(cgwnd* outer_as_gwnd, int control_id, bool add_base_key) -> cgwnd* {
    return find_ui_child(reinterpret_cast<cps_outer_interface*>(outer_as_gwnd), control_id, add_base_key);
  }

  auto ingame_res_map_ptr() -> void* {
    return reinterpret_cast<void*>(ext_client::offsets::ingame_ui_map::globals::address);
  }

  auto find_ingame_res_raw(int res_key) -> void* {
    auto* map = ingame_res_map_ptr();
    if (!ext_client::msvc9::is_readable_ptr(map, ext_client::msvc9::ui_res_map_size)) {
      return nullptr;
    }

    using find_fn = int(__thiscall*)(void*, int);
    const auto fn = as_fn<find_fn>(ext_client::offsets::ingame_ui_map::functions::find);
    const auto raw = fn(map, res_key);
    if (raw == 0) {
      return nullptr;
    }

    auto* wnd = reinterpret_cast<void*>(raw);
    return ext_client::msvc9::is_game_ptr(wnd) ? wnd : nullptr;
  }

  auto find_ingame_res(int res_key) -> cgwnd* {
    auto* raw = find_ingame_res_raw(res_key);
    if (raw == nullptr) {
      return nullptr;
    }

    auto* wnd = reinterpret_cast<cgwnd*>(raw);
    return is_live_widget(wnd) ? wnd : nullptr;
  }

  auto diagnose_ingame_res(int res_key) -> ingame_res_lookup {
    ingame_res_lookup out{};
    out.res_key = res_key;

    const auto* map = ingame_res_map_ptr();
    out.map_readable = ext_client::msvc9::is_readable_ptr(map, ext_client::msvc9::ui_res_map_size);
    if (!out.map_readable) {
      return out;
    }

    out.raw = find_ingame_res_raw(res_key);
    out.found = out.raw != nullptr;
    if (out.raw == nullptr) {
      return out;
    }

    if (!ext_client::msvc9::try_read_u32(out.raw, &out.vftable)) {
      return out;
    }

    auto* wnd = reinterpret_cast<cgwnd*>(out.raw);
    out.live = is_live_widget(wnd);
    out.wnd = out.live ? wnd : nullptr;
    return out;
  }

  auto enumerate_res_map(const void* map_obj, int max_entries) -> std::vector<res_map_entry> {
    std::vector<res_map_entry> out;
    if (!map_obj || max_entries <= 0 || !ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size)) {
      return out;
    }

    out.reserve(static_cast<std::size_t>(max_entries));
    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t key, void* value) {
      if (static_cast<int>(out.size()) >= max_entries) {
        return;
      }
      auto* wnd = reinterpret_cast<cgwnd*>(value);
      if (!is_live_widget(wnd)) {
        return;
      }
      out.push_back({static_cast<int>(key), wnd});
    });

    std::sort(out.begin(), out.end(), [](const res_map_entry& a, const res_map_entry& b) { return a.key < b.key; });
    return out;
  }

  auto enumerate_ingame_res_map(int max_entries) -> std::vector<res_map_entry> {
    return enumerate_res_map(ingame_res_map_ptr(), max_entries);
  }

  auto enumerate_iface_res_map(int max_entries) -> std::vector<res_map_entry> {
    if (!cg_interface::is_ready()) {
      return {};
    }
    return enumerate_res_map(cg_interface::get()->ui_res_map(), max_entries);
  }

  auto walk_ingame_res_roots(walk_visitor_fn visit, void* ctx, int child_depth) -> void {
    if (!visit) {
      return;
    }

    const auto* map = ingame_res_map_ptr();
    if (!ext_client::msvc9::is_readable_ptr(map, ext_client::msvc9::ui_res_map_size)) {
      return;
    }

    ext_client::msvc9::map_ref::from(map).for_each([&](std::uint32_t /*key*/, void* value) {
      auto* wnd = reinterpret_cast<cgwnd*>(value);
      if (!is_live_widget(wnd)) {
        return;
      }
      visit(wnd, ctx);
      walk_each(wnd, child_depth, visit, ctx);
    });
  }

  auto is_title_list_button_widget(const cgwnd* wnd) -> bool {
    if (!cif_manager::is_live_widget(wnd)) {
      return false;
    }

    const auto kind = cif_manager::vftable_kind(reinterpret_cast<std::uint32_t>(wnd->vftable));
    if (kind != cif_manager::widget_kind::cif_button && kind != cif_manager::widget_kind::cif_decorated_static) {
      return false;
    }

    const int w = wnd->rect_w();
    const int h = wnd->rect_h();
    return w > 0 && h > 0 && w <= 200 && h <= 80;
  }

  auto is_channel_row_list_button(const cgwnd* wnd, const cgwnd* channel_combo) -> bool {
    if (!is_title_list_button_widget(wnd)) {
      return false;
    }
    if (!channel_combo || !cif_manager::is_live_widget(channel_combo)) {
      return true;
    }

    const int row_y = channel_combo->rect_y();
    const int row_right = channel_combo->rect_x() + channel_combo->rect_w();
    const int y = wnd->rect_y();
    const int x = wnd->rect_x();
    return std::abs(y - row_y) <= 12 && x >= row_right - 24;
  }

  auto pick_channel_row_list_button(cgwnd* const* buttons, int count) -> cgwnd* {
    cgwnd* channel_btn = nullptr;
    int channel_y = INT_MAX;

    for (int i = 0; i < count; ++i) {
      if (!cif_manager::is_live_widget(buttons[i])) {
        continue;
      }
      for (int j = i + 1; j < count; ++j) {
        if (!cif_manager::is_live_widget(buttons[j])) {
          continue;
        }

        const int xi = buttons[i]->rect_x();
        const int xj = buttons[j]->rect_x();
        const int yi = buttons[i]->rect_y();
        const int yj = buttons[j]->rect_y();
        if (std::abs(xi - xj) > 8) {
          continue;
        }

        auto* upper = yi <= yj ? buttons[i] : buttons[j];
        const int upper_y = yi <= yj ? yi : yj;
        if (upper_y < channel_y) {
          channel_y = upper_y;
          channel_btn = upper;
        }
      }
    }

    if (channel_btn) {
      return channel_btn;
    }

    for (int i = 0; i < count; ++i) {
      if (!cif_manager::is_live_widget(buttons[i])) {
        continue;
      }
      const int y = buttons[i]->rect_y();
      if (y < channel_y) {
        channel_y = y;
        channel_btn = buttons[i];
      }
    }

    return cif_manager::is_live_widget(channel_btn) ? channel_btn : nullptr;
  }

  struct title_list_button_collect_ctx {
    cgwnd* buttons[8]{};
    int count = 0;
  };

  auto collect_title_list_button(cgwnd* wnd, void* ctx) -> void {
    auto* collect = static_cast<title_list_button_collect_ctx*>(ctx);
    if (!collect || collect->count >= 8) {
      return;
    }
    if (!is_title_list_button_widget(wnd)) {
      return;
    }
    collect->buttons[collect->count++] = wnd;
  }

  auto scan_title_channel_list_button(cps_outer_interface* title, const cgwnd* channel_combo) -> cgwnd* {
    if (!title) {
      return nullptr;
    }

    auto* root = reinterpret_cast<cgwnd*>(title);
    if (!is_live_widget(root)) {
      return nullptr;
    }

    title_list_button_collect_ctx collect{};
    walk_each(root, 12, collect_title_list_button, &collect);
    if (collect.count == 0) {
      return nullptr;
    }

    if (channel_combo && is_live_widget(channel_combo)) {
      cgwnd* best = nullptr;
      int best_dx = INT_MAX;
      const int row_right = channel_combo->rect_x() + channel_combo->rect_w();

      for (int i = 0; i < collect.count; ++i) {
        auto* wnd = collect.buttons[i];
        if (!is_channel_row_list_button(wnd, channel_combo)) {
          continue;
        }
        const int dx = wnd->rect_x() - row_right;
        if (dx < best_dx) {
          best_dx = dx;
          best = wnd;
        }
      }

      if (best) {
        return best;
      }
    }

    return pick_channel_row_list_button(collect.buttons, collect.count);
  }

  auto find_title_channel_list_button(cps_outer_interface* title) -> cgwnd* {
    if (!title) {
      return nullptr;
    }

    if (auto* widget = find_ui_child(title, ext_client::offsets::cps_title::ui_ids::channel_list_button, false)) {
      if (is_live_widget(widget) && is_title_list_button_widget(widget)) {
        return widget;
      }
    }

    auto* channel_combo = find_ui_child(title, ext_client::offsets::cps_title::ui_ids::channel_combo, false);
    return scan_title_channel_list_button(title, channel_combo);
  }

  auto find_title_channel_list_button(cgwnd* title_as_gwnd) -> cgwnd* {
    return find_title_channel_list_button(reinterpret_cast<cps_outer_interface*>(title_as_gwnd));
  }

  auto res_map_key_in(const void* map_obj, const cgwnd* widget) -> int {
    if (!map_obj || !widget || !ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size)) {
      return -1;
    }

    int found = -1;
    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t key, void* value) {
      if (value == widget) {
        found = static_cast<int>(key);
      }
    });
    return found;
  }

  auto descendant_depth(const cgwnd* root, const cgwnd* target, int depth, int max_depth) -> int {
    if (!root || !target || depth > max_depth) {
      return -1;
    }
    if (root == target) {
      return depth;
    }

    const auto* sentinel = child_list_sentinel(root);
    if (!sentinel) {
      return -1;
    }

    int best = -1;
    ext_client::msvc9::child_list_ref::from_sentinel(sentinel).for_each([&](std::uint32_t, void* value) {
      if (best >= 0) {
        return;
      }
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!is_live_widget(child) || child == root) {
        return;
      }
      if (const int found = descendant_depth(child, target, depth + 1, max_depth); found >= 0) {
        best = found;
      }
    });
    return best;
  }

  auto res_map_key_containing(const void* map_obj, const cgwnd* widget) -> int {
    if (!map_obj || !widget || !ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size)) {
      return -1;
    }

    int best_key = -1;
    int best_depth = INT_MAX;
    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t key, void* value) {
      auto* root = reinterpret_cast<cgwnd*>(value);
      if (!is_live_widget(root)) {
        return;
      }
      const int depth = descendant_depth(root, widget, 0, 32);
      if (depth < 0 || depth >= best_depth) {
        return;
      }
      best_depth = depth;
      best_key = static_cast<int>(key);
    });
    return best_key;
  }

  auto res_map_key_for(cps_outer_interface* outer, const cgwnd* widget) -> int {
    if (!outer || !widget) {
      return -1;
    }
    return res_map_key_in(embedded_res_map_ptr(outer), widget);
  }

  auto find_res_map_key(const cgwnd* widget) -> int {
    if (!widget) {
      return -1;
    }

    auto try_maps = [&](const void* map_obj) -> int {
      if (const int direct = res_map_key_in(map_obj, widget); direct >= 0) {
        return direct;
      }
      return res_map_key_containing(map_obj, widget);
    };

    if (auto* title = cps_title::current()) {
      if (const int key = try_maps(embedded_res_map_ptr(title)); key >= 0) {
        return key;
      }
    }
    if (auto* character_select = cps_character_select::current()) {
      if (const int key = try_maps(embedded_res_map_ptr(character_select)); key >= 0) {
        return key;
      }
    }
    if (cg_interface::is_ready()) {
      if (const int key = try_maps(cg_interface::get()->ui_res_map()); key >= 0) {
        return key;
      }
    }

    for (auto* parent = widget->parent(); parent != nullptr; parent = parent->parent()) {
      if (const int key = try_maps(if_wnd_res_map_ptr(parent)); key >= 0) {
        return key;
      }
    }

    return -1;
  }

  auto read_string_field(const void* object, std::size_t byte_offset, char* dst, std::size_t dst_count) -> bool {
    if (!object || !dst || dst_count == 0) {
      return false;
    }
    dst[0] = '\0';
    const auto* field = reinterpret_cast<const std::uint8_t*>(object) + byte_offset;
    if (!ext_client::msvc9::is_readable_ptr(field, ext_client::msvc9::string_object_size)) {
      return false;
    }
    return ext_client::msvc9::string_ref::from(field).copy_to(dst, dst_count);
  }

  auto read_widget_ddj_path(const cgwnd* wnd, char* dst, std::size_t dst_count) -> bool {
    if (!wnd || !dst || dst_count == 0) {
      return false;
    }
    dst[0] = '\0';

    if (cif_decorated_static::is_instance(wnd)) {
      if (read_string_field(wnd, ext_client::offsets::cif_decorated_static::fields::texture_path_alt, dst, dst_count) && dst[0] != '\0') {
        return true;
      }
      return read_string_field(wnd, ext_client::offsets::cif_decorated_static::fields::texture_path, dst, dst_count) && dst[0] != '\0';
    }

    if (!is_static(wnd)) {
      return false;
    }

    if (read_string_field(wnd, ext_client::offsets::cif_static::fields::image_texture_path, dst, dst_count) && dst[0] != '\0') {
      return true;
    }

    return false;
  }

  auto catalog_screen_for_root(const cgwnd* root) -> ext_client::utils::ui_res_catalog::screen {
    if (!root) {
      return ext_client::utils::ui_res_catalog::screen::pstitle;
    }
    const auto vftable = reinterpret_cast<std::uint32_t>(root->vftable);
    if (vftable == ext_client::offsets::cps_character_select::vtable::address) {
      return ext_client::utils::ui_res_catalog::screen::character_select;
    }
    return ext_client::utils::ui_res_catalog::screen::pstitle;
  }

  auto ensure_catalog_for_walk(const cgwnd* root) -> void {
    const auto screen = catalog_screen_for_root(root);
    if (has_parsed_res_catalog(root)) {
      auto* outer = const_cast<cps_outer_interface*>(reinterpret_cast<const cps_outer_interface*>(root));
      ext_client::utils::ui_res_catalog::sync_from_game(embedded_res_map_ptr(outer), screen);
      return;
    }
    ext_client::utils::ui_res_catalog::ensure_loaded(screen);
  }

  auto enrich_widget_info(widget_info& info, const cgwnd* walk_root) -> void {
    if (!info.widget) {
      return;
    }

    info.lookup_res_inferred = false;
    read_widget_ddj_path(info.widget, info.ddj_path, sizeof(info.ddj_path));
    info.lookup_res_key = info.res_map_key >= 0 ? info.res_map_key : find_res_map_key(info.widget);

    const auto screen = catalog_screen_for_root(walk_root);
    ext_client::utils::ui_res_catalog::ensure_loaded(screen);

    if (info.lookup_res_key < 0 && info.ddj_path[0] != '\0') {
      if (const auto* by_ddj = ext_client::utils::ui_res_catalog::find_by_ddj(screen, info.ddj_path)) {
        info.lookup_res_key = by_ddj->id;
        info.lookup_res_inferred = true;
        std::strncpy(info.res_name, by_ddj->name, sizeof(info.res_name) - 1);
      }
    }

    if (info.lookup_res_key < 0) {
      return;
    }

    if (info.res_name[0] == '\0') {
      if (const auto* entry = ext_client::utils::ui_res_catalog::find(screen, info.lookup_res_key)) {
        std::strncpy(info.res_name, entry->name, sizeof(info.res_name) - 1);
        if (info.ddj_path[0] == '\0' && entry->ddj[0] != '\0') {
          std::strncpy(info.ddj_path, entry->ddj, sizeof(info.ddj_path) - 1);
        }
      }
    }
  }

  auto for_each_owned_child(cgwnd* parent, child_visitor_fn visit, void* ctx) -> void {
    if (!parent || !visit) {
      return;
    }

    if (const auto* sentinel = child_list_sentinel(parent)) {
      ext_client::msvc9::child_list_ref::from_sentinel(sentinel).for_each([&](std::uint32_t, void* value) {
        auto* child = reinterpret_cast<cgwnd*>(value);
        if (!is_live_widget(child) || child == parent) {
          return;
        }
        visit(child, ctx);
      });
    }

    if (has_outer_res_map(parent)) {
      const auto* map = embedded_res_map_ptr(reinterpret_cast<cps_outer_interface*>(parent));
      if (ext_client::msvc9::is_readable_ptr(map, ext_client::msvc9::ui_res_map_size)) {
        ext_client::msvc9::map_ref::from(map).for_each([&](std::uint32_t, void* value) {
          auto* child = reinterpret_cast<cgwnd*>(value);
          if (!is_live_widget(child) || child == parent) {
            return;
          }
          visit(child, ctx);
        });
      }
    }

    if (const auto* res_map = cg_interface_res_map_ptr(parent)) {
      ext_client::msvc9::map_ref::from(res_map).for_each([&](std::uint32_t, void* value) {
        auto* child = reinterpret_cast<cgwnd*>(value);
        if (!is_live_widget(child) || child == parent) {
          return;
        }
        visit(child, ctx);
      });
    }

    if (const auto* if_map = if_wnd_res_map_ptr(parent)) {
      ext_client::msvc9::map_ref::from(if_map).for_each([&](std::uint32_t, void* value) {
        auto* child = reinterpret_cast<cgwnd*>(value);
        if (!is_live_widget(child) || child == parent) {
          return;
        }
        visit(child, ctx);
      });
    }

    if (calarm_guide_mgr_wnd::is_instance(parent)) {
      auto* mgr = calarm_guide_mgr_wnd::from(parent);
      for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
        if (auto* slot = calarm_guide_mgr_wnd::loose_slot_widget(mgr, i); is_live_widget(slot)) {
          visit(slot, ctx);
        }
      }
      for (std::size_t i = 0; i < 3; ++i) {
        if (auto* effect = calarm_guide_mgr_wnd::loose_effect_widget(mgr, i); is_live_widget(effect)) {
          visit(effect, ctx);
        }
      }
    }
  }

  auto count_owned_children(const cgwnd* parent) -> std::size_t {
    struct count_ctx {
      std::size_t n = 0;
    } ctx;

    for_each_owned_child(const_cast<cgwnd*>(parent), [](cgwnd* /*child*/, void* user) { ++static_cast<count_ctx*>(user)->n; }, &ctx);
    return ctx.n;
  }

  auto walk_each(cgwnd* root, int max_depth, walk_visitor_fn visit, void* ctx) -> void {
    if (!ext_client::msvc9::is_game_ptr(root) || !visit) {
      return;
    }

    std::unordered_set<cgwnd*> seen;
    walk_each_visit_unique(root, seen, visit, ctx);
    walk_each_gwnd_recursive(root, 1, max_depth, seen, visit, ctx);

    if (has_outer_res_map(root)) {
      auto* outer = reinterpret_cast<cps_outer_interface*>(root);
      walk_each_res_map_children(embedded_res_map_ptr(outer), root, 1, max_depth, seen, visit, ctx);
    }

    if (auto* res_map = cg_interface_res_map_ptr(root)) {
      walk_each_res_map_children(res_map, root, 1, max_depth, seen, visit, ctx);
    }

    if (auto* if_map = if_wnd_res_map_ptr(root)) {
      walk_each_if_wnd_res_map_children(if_map, root, 1, max_depth, seen, visit, ctx);
    }
  }

  auto walk(cgwnd* root, int max_depth, int cginterface_probe_max_id) -> std::vector<widget_info> {
    std::vector<widget_info> out;
    if (!is_safe_widget(root)) {
      return out;
    }

    g_walk_widget_budget = k_default_walk_widget_budget;
    std::unordered_set<cgwnd*> seen;
    push_widget(out, root, 0, max_depth, seen);

    walk_gwnd_recursive(root, 1, max_depth, out, seen);

    if (has_outer_res_map(root)) {
      auto* outer = reinterpret_cast<cps_outer_interface*>(root);
      walk_res_map_children(embedded_res_map_ptr(outer), root, 1, max_depth, out, seen);
    }

    if (auto* res_map = cg_interface_res_map_ptr(root)) {
      walk_res_map_children(res_map, root, 1, max_depth, out, seen);
    }

    if (auto* if_map = if_wnd_res_map_ptr(root)) {
      walk_if_wnd_res_map_children(if_map, root, 1, max_depth, out, seen);
    }

    append_cginterface_extras(root, max_depth, cginterface_probe_max_id, out, seen);

    ensure_catalog_for_walk(root);
    for (auto& item : out) {
      enrich_widget_info(item, root);
    }

    g_walk_widget_budget = k_default_walk_widget_budget;
    return out;
  }

  auto walk_static_texts(cgwnd* root, int max_depth, int cginterface_probe_max_id) -> std::vector<widget_info> {
    const auto all = walk(root, max_depth, cginterface_probe_max_id);
    std::vector<widget_info> out;
    out.reserve(all.size());
    for (const auto& item : all) {
      if (is_static(item.widget)) {
        out.push_back(item);
      }
    }
    return out;
  }

  auto spawn_static_label(cgwnd* parent, const spawn_label_options& options) -> cif_static* {
    if (!parent) {
      return nullptr;
    }

    auto* label = cif_static::create_outer_wnd(parent, cif_static::version_label_res(), options.rect, 0, 0);
    if (!label) {
      return nullptr;
    }

    apply_static_label(label, options.text, options.text_color);
    label->set_visible(options.visible);
    return label;
  }

  auto destroy_widget(cgwnd* widget) -> void {
    if (!widget) {
      return;
    }
    using scalar_dtor_fn = void*(__thiscall*)(cgwnd * self, char should_free);
    const auto vft = reinterpret_cast<std::uintptr_t*>(widget->vftable);
    const auto dtor = reinterpret_cast<scalar_dtor_fn>(vft[2]);
    dtor(widget, 1);
  }

  auto apply_static_label(cif_static* label, const wchar_t* text, std::uint32_t color) -> void {
    if (!label) {
      return;
    }
    label->set_text_color(color);
    if (text) {
      label->set_text(text);
    }
    label->refresh_layout();
  }

} // namespace ext_client::cif_manager
