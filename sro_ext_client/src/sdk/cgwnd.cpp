#include "cgwnd.hpp"

#include "calarm_guide_mgr_wnd.hpp"
#include "ccontroler.hpp"
#include "cg_interface.hpp"
#include "cif_wnd.hpp"
#include "cps_outer_interface.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/rtti.hpp"

#include <cstring>
#include <unordered_set>

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

auto cgwnd::set_visible(bool visible) -> int {
  return set_visible(this, visible);
}

auto cgwnd::set_position(int x, int y) -> int {
  return set_position(this, x, y);
}

auto cgwnd::set_size(int width, int height) -> int {
  return set_size(this, width, height);
}

auto cgwnd::set_anim(int alpha, float speed, float delay, int mode) -> void {
  using set_anim_fn = void(__cdecl*)(cgwnd*, int, float, float, int);
  const auto fn = as_fn<set_anim_fn>(ext_client::offsets::cgwnd::functions::set_anim);
  fn(this, alpha, speed, delay, mode);
}

auto cgwnd::get_bounds() const -> cgwnd_bounds {
  return {rect_x(), rect_y(), rect_w(), rect_h()};
}

auto cgwnd::client_config() -> void* {
  using get_client_config_fn = void*(__cdecl*)();
  const auto fn = as_fn<get_client_config_fn>(ext_client::offsets::cgwnd::functions::get_client_config);
  return fn();
}

auto cgwnd::client_data_version() -> unsigned {
  const auto* cfg = static_cast<const std::uint32_t*>(client_config());
  if (!cfg) {
    return 1000u;
  }
  return cfg[ext_client::offsets::cgwnd::client_config_fields::data_version / sizeof(std::uint32_t)] + 1000u;
}

auto cgwnd::screen_height() -> int {
  using get_screen_size_fn = void*(__cdecl*)();
  const auto fn = as_fn<get_screen_size_fn>(ext_client::offsets::cgwnd::functions::get_screen_size);
  const auto* screen = static_cast<const std::uint8_t*>(fn());
  if (!screen) {
    return 0;
  }
  return ext_client::offsets::field_at<int>(screen, ext_client::offsets::cgwnd::screen_size::height);
}

auto cgwnd::screen_width() -> int {
  using get_screen_size_fn = void*(__cdecl*)();
  const auto fn = as_fn<get_screen_size_fn>(ext_client::offsets::cgwnd::functions::get_screen_size);
  const auto* screen = static_cast<const std::uint8_t*>(fn());
  if (!screen) {
    return 0;
  }
  return ext_client::offsets::field_at<int>(screen, ext_client::offsets::cgwnd::screen_size::width);
}

auto cgwnd::set_position(cgwnd* wnd, int x, int y) -> int {
  if (!wnd) {
    return 0;
  }

  using set_position_fn = int(__thiscall*)(cgwnd*, int, int);
  const auto fn = as_fn<set_position_fn>(ext_client::offsets::cgwnd::functions::set_position);
  return fn(wnd, x, y);
}

auto cgwnd::set_size(cgwnd* wnd, int width, int height) -> int {
  if (!wnd) {
    return 0;
  }

  using set_size_fn = int(__thiscall*)(cgwnd*, int, int);
  const auto fn = as_fn<set_size_fn>(ext_client::offsets::cgwnd::functions::set_size);
  return fn(wnd, width, height);
}

auto cgwnd::set_visible(cgwnd* wnd, bool visible) -> int {
  if (!wnd) {
    return 0;
  }

  using set_visible_fn = int(__thiscall*)(cgwnd*, std::uint8_t);
  const auto vis = static_cast<std::uint8_t>(visible ? 1 : 0);

  const auto* vft = reinterpret_cast<const std::uint32_t*>(wnd->vftable);
  if (vft) {
    const auto slot23 = vft[23];
    if (slot23 == ext_client::offsets::cif_static::functions::set_visible ||
        slot23 == ext_client::offsets::cif_button::functions::set_visible ||
        slot23 == ext_client::offsets::cnif_button::functions::set_visible) {
      return as_fn<set_visible_fn>(slot23)(wnd, vis);
    }
  }

  return as_fn<set_visible_fn>(ext_client::offsets::cgwnd::functions::set_visible)(wnd, vis);
}

auto cgwnd::interface_under_cursor() -> cgwnd* {
  auto* wnd = global_at<cgwnd*>(ext_client::offsets::globals::interface_under_cursor);
  if (!wnd) {
    return nullptr;
  }
  return wnd;
}

auto cgwnd::refresh_interface_under_cursor() -> bool {
  auto* gfx = global_at<void*>(ext_client::offsets::cgfx_main_frame::address);
  if (!gfx) {
    return false;
  }

  using refresh_fn = char(__thiscall*)(void*);
  const auto fn = as_fn<refresh_fn>(ext_client::offsets::cgwnd::functions::refresh_interface_under_cursor);
  fn(gfx);
  return true;
}

auto cgwnd::get_manager() -> void* {
  using get_manager_fn = void*(__cdecl*)();
  const auto fn = as_fn<get_manager_fn>(ext_client::offsets::cgwnd::functions::get_manager);
  return fn();
}

auto cgwnd::game_ui_root() -> cgwnd* {
  const auto* gfx = global_at<const void*>(ext_client::offsets::cgfx_main_frame::address);
  if (!gfx) {
    return nullptr;
  }
  auto* root = ext_client::offsets::field_at<cgwnd*>(gfx, ext_client::offsets::cgfx_main_frame::fields::ui_root);
  if (!root) {
    return nullptr;
  }
  return root;
}

auto cgwnd::pick_at_point(cgwnd* root, int x, int y) -> cgwnd* {
  if (!root) {
    return nullptr;
  }
  using pick_at_point_fn = int(__thiscall*)(cgwnd*, int, int);
  const auto fn = as_fn<pick_at_point_fn>(ext_client::offsets::cgwnd::functions::pick_at_point);
  const int result = fn(root, x, y);
  auto* wnd = reinterpret_cast<cgwnd*>(result);
  return wnd ? wnd : nullptr;
}

auto cgwnd::get_child_by_unique_id(cgwnd* parent, int unique_id) -> cgwnd* {
  if (!parent) {
    return nullptr;
  }
  using get_child_fn = cgwnd*(__thiscall*)(cgwnd*, int);
  const auto fn = as_fn<get_child_fn>(ext_client::offsets::cgwnd::functions::get_child_by_unique_id);
  auto* child = fn(parent, unique_id);
  return child ? child : nullptr;
}

auto cgwnd::is_pickable(const cgwnd* wnd) -> bool {
  if (!wnd) {
    return false;
  }

  using is_pickable_fn = char(__thiscall*)(cgwnd*);
  const auto fn = as_fn<is_pickable_fn>(ext_client::offsets::cgwnd::functions::is_visible);
  return fn(const_cast<cgwnd*>(wnd)) != 0;
}

auto cgwnd::hit_test_contains(int x, int y) const -> bool {
  if (!visible()) {
    return false;
  }

  using hit_test_fn = bool(__thiscall*)(const cgwnd*, int, int);
  const auto fn = as_fn<hit_test_fn>(ext_client::offsets::cgwnd::functions::hit_test_contains);
  return fn(this, x, y);
}

// ---------------------------------------------------------------------------
// Runtime type identification
// ---------------------------------------------------------------------------

auto cgwnd::is_live() const -> bool {
  if (!vftable) {
    return false;
  }
  const auto vft = reinterpret_cast<std::uint32_t>(vftable);
  char scratch[64]{};
  return ext_client::rtti::class_name(vft, scratch, sizeof(scratch));
}

auto cgwnd::type_name(const void* obj) -> const char* {
  if (!obj) {
    return "none";
  }
  const auto vft = *reinterpret_cast<const std::uint32_t*>(obj);
  return type_name_vftable(vft);
}

auto cgwnd::type_name_vftable(std::uint32_t vftable) -> const char* {
  return ext_client::rtti::class_name_cached(vftable);
}

auto cgwnd::topmost_ancestor() -> cgwnd* {
  if (!vftable) {
    return nullptr;
  }
  auto* top = this;
  for (int guard = 0; guard < 64; ++guard) {
    auto* p = top->parent();
    if (!p || !p->vftable) {
      break;
    }
    top = p;
  }
  return top;
}

// ---------------------------------------------------------------------------
// Child iteration
// ---------------------------------------------------------------------------

namespace {

  auto child_list(const cgwnd* wnd) -> const void* {
    if (!wnd) {
      return nullptr;
    }
    using ext_client::offsets::field_at;
    return field_at<const void*>(wnd, ext_client::offsets::cgwnd::fields::child_list);
  }

  auto embedded_res_map_ptr(cps_outer_interface* outer) -> void* {
    if (!outer) {
      return nullptr;
    }
    return reinterpret_cast<std::uint8_t*>(outer) + ext_client::offsets::cps_silkroad::fields::res_ui_root;
  }

  auto has_outer_res_map(const cgwnd* root) -> bool {
    if (!root || !root->vftable) {
      return false;
    }
    const char* name = ccontroler::factory_entry_name(root);
    return name != nullptr && std::strncmp(name, "CPS", 3) == 0;
  }

  auto cg_interface_res_map_ptr(const cgwnd* root) -> const void* {
    if (!cg_interface::is_instance(root)) {
      return nullptr;
    }
    const auto* map = reinterpret_cast<const cg_interface*>(root)->ui_res_map();
    return map ? map->raw() : nullptr;
  }

  auto if_wnd_res_map_ptr(const cgwnd* wnd) -> const void* {
    if (!wnd || cg_interface::is_instance(wnd)) {
      return nullptr;
    }
    const auto vft = reinterpret_cast<std::uint32_t>(wnd->vftable);
    char class_name[64]{};
    if (!ext_client::rtti::class_name(vft, class_name, sizeof(class_name))) {
      return nullptr;
    }
    if (std::strncmp(class_name, "CIF", 3) != 0 &&
        std::strncmp(class_name, "CAlramGuideMgr", 14) != 0) {
      return nullptr;
    }
    return cif_wnd::from(const_cast<cgwnd*>(wnd))->ui_res_map();
  }

  auto walk_depth_exceeded(int depth, int max_depth) -> bool {
    return max_depth > 0 && depth > max_depth;
  }

  auto visit_each_child_unique(cgwnd* wnd, std::unordered_set<cgwnd*>& seen, cgwnd::child_visitor_fn visit, void* ctx) -> void {
    if (!wnd || !visit || !wnd->is_live()) {
      return;
    }
    if (!seen.insert(wnd).second) {
      return;
    }
    visit(wnd, ctx);
  }

  auto walk_each_recursive(cgwnd* root, int depth, int max_depth, std::unordered_set<cgwnd*>& seen, cgwnd::child_visitor_fn visit, void* ctx) -> void;

  auto walk_each_child_subtrees(cgwnd* child, int depth, int max_depth, std::unordered_set<cgwnd*>& seen, cgwnd::child_visitor_fn visit, void* ctx) -> void {
    if (!child || walk_depth_exceeded(depth, max_depth)) {
      return;
    }
    // Child list
    for (auto* c : child->children()) {
      if (!c || !c->is_live() || c == child) {
        continue;
      }
      visit_each_child_unique(c, seen, visit, ctx);
      walk_each_child_subtrees(c, depth, max_depth, seen, visit, ctx);
    }
    // CGInterface res map
    if (auto* res_map = cg_interface_res_map_ptr(child)) {
      ext_client::msvc9::map_ref::from(res_map).for_each([&](std::uint32_t, void* value) {
        auto* c = reinterpret_cast<cgwnd*>(value);
        if (!c || c == child) {
          return;
        }
        visit_each_child_unique(c, seen, visit, ctx);
        walk_each_child_subtrees(c, depth, max_depth, seen, visit, ctx);
      });
    }
    // IFWnd res map
    if (auto* if_map = if_wnd_res_map_ptr(child)) {
      ext_client::msvc9::map_ref::from(if_map).for_each([&](std::uint32_t, void* value) {
        auto* c = reinterpret_cast<cgwnd*>(value);
        if (!c || c == child) {
          return;
        }
        visit_each_child_unique(c, seen, visit, ctx);
        walk_each_child_subtrees(c, depth, max_depth, seen, visit, ctx);
      });
    }
  }

  auto walk_each_recursive(cgwnd* root, int depth, int max_depth, std::unordered_set<cgwnd*>& seen, cgwnd::child_visitor_fn visit, void* ctx) -> void {
    if (!root || walk_depth_exceeded(depth, max_depth) || !visit) {
      return;
    }
    for (auto* c : root->children()) {
      if (!c || !c->is_live() || c == root) {
        continue;
      }
      visit_each_child_unique(c, seen, visit, ctx);
      walk_each_child_subtrees(c, depth, max_depth, seen, visit, ctx);
    }
  }

} // namespace

auto cgwnd::children() const -> ext_client::msvc9::list<cgwnd*> {
  if (const auto* child_list_ptr = child_list(this)) {
    return ext_client::msvc9::list<cgwnd*>::from(child_list_ptr);
  }
  return {};
}

auto cgwnd::for_each_child(child_visitor_fn visit, void* ctx) -> void {
  if (!visit) {
    return;
  }

  // Intrusive child list
  for (auto* child : children()) {
    if (!child || !child->is_live() || child == this) {
      continue;
    }
    visit(child, ctx);
  }

  // CPS outer res map
  if (has_outer_res_map(this)) {
    const auto* map = embedded_res_map_ptr(reinterpret_cast<cps_outer_interface*>(this));
    if (map) {
      ext_client::msvc9::map_ref::from(map).for_each([&](std::uint32_t, void* value) {
        auto* child = reinterpret_cast<cgwnd*>(value);
        if (!child || !child->is_live() || child == this) {
          return;
        }
        visit(child, ctx);
      });
    }
  }

  // CGInterface res map
  if (const auto* res_map = cg_interface_res_map_ptr(this)) {
    ext_client::msvc9::map_ref::from(res_map).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || !child->is_live() || child == this) {
        return;
      }
      visit(child, ctx);
    });
  }

  // CIFWnd res map
  if (const auto* if_map = if_wnd_res_map_ptr(this)) {
    ext_client::msvc9::map_ref::from(if_map).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || !child->is_live() || child == this) {
        return;
      }
      visit(child, ctx);
    });
  }

  // CAlramGuideMgrWnd loose slots
  if (calarm_guide_mgr_wnd::is_instance(this)) {
    auto* mgr = calarm_guide_mgr_wnd::from(this);
    for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
      if (auto* slot = calarm_guide_mgr_wnd::loose_slot_widget(mgr, i); slot && slot->is_live()) {
        visit(slot, ctx);
      }
    }
    for (std::size_t i = 0; i < 3; ++i) {
      if (auto* effect = calarm_guide_mgr_wnd::loose_effect_widget(mgr, i); effect && effect->is_live()) {
        visit(effect, ctx);
      }
    }
  }
}

auto cgwnd::walk_each(int max_depth, child_visitor_fn visit, void* ctx) -> void {
  if (!visit) {
    return;
  }

  std::unordered_set<cgwnd*> seen;
  visit_each_child_unique(this, seen, visit, ctx);
  walk_each_recursive(this, 1, max_depth, seen, visit, ctx);

  if (has_outer_res_map(this)) {
    auto* outer = reinterpret_cast<cps_outer_interface*>(this);
    const auto* map = embedded_res_map_ptr(outer);
    if (map) {
      ext_client::msvc9::map_ref::from(map).for_each([&](std::uint32_t, void* value) {
        auto* child = reinterpret_cast<cgwnd*>(value);
        if (!child || child == this) {
          return;
        }
        visit_each_child_unique(child, seen, visit, ctx);
        walk_each_child_subtrees(child, 1, max_depth, seen, visit, ctx);
      });
    }
  }

  if (auto* res_map = cg_interface_res_map_ptr(this)) {
    ext_client::msvc9::map_ref::from(res_map).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || child == this) {
        return;
      }
      visit_each_child_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, 1, max_depth, seen, visit, ctx);
    });
  }

  if (auto* if_map = if_wnd_res_map_ptr(this)) {
    ext_client::msvc9::map_ref::from(if_map).for_each([&](std::uint32_t, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || child == this) {
        return;
      }
      visit_each_child_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, 1, max_depth, seen, visit, ctx);
    });
  }
}

auto cgwnd::destroy() -> void {
  using scalar_dtor_fn = void*(__thiscall*)(cgwnd * self, char should_free);
  const auto vft = reinterpret_cast<std::uintptr_t*>(vftable);
  const auto dtor = reinterpret_cast<scalar_dtor_fn>(vft[2]);
  dtor(this, 1);
}

auto cgwnd::find_ingame_res_raw(int res_key) -> void* {
  auto* map = reinterpret_cast<void*>(ext_client::offsets::ingame_ui_map::globals::address);
  if (!map) {
    return nullptr;
  }
  using find_fn = int(__thiscall*)(void*, int);
  const auto fn = as_fn<find_fn>(ext_client::offsets::ingame_ui_map::functions::find);
  const auto raw = fn(map, res_key);
  if (raw == 0) {
    return nullptr;
  }
  return reinterpret_cast<void*>(raw);
}

auto cgwnd::find_ingame_res(int res_key) -> cgwnd* {
  auto* raw = find_ingame_res_raw(res_key);
  if (!raw) {
    return nullptr;
  }
  auto* wnd = reinterpret_cast<cgwnd*>(raw);
  return wnd && wnd->is_live() ? wnd : nullptr;
}

auto cgwnd::walk_ingame_res_roots(child_visitor_fn visit, void* ctx, int child_depth) -> void {
  if (!visit) {
    return;
  }
  auto* map = reinterpret_cast<void*>(ext_client::offsets::ingame_ui_map::globals::address);
  if (!map) {
    return;
  }
  ext_client::msvc9::map_ref::from(map).for_each([&](std::uint32_t /*key*/, void* value) {
    auto* wnd = reinterpret_cast<cgwnd*>(value);
    if (!wnd || !wnd->is_live()) {
      return;
    }
    visit(wnd, ctx);
    wnd->walk_each(child_depth, visit, ctx);
  });
}
