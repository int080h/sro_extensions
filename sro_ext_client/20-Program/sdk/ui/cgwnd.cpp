#include "pch.hpp"
#include "sdk/ui/cgwnd.hpp"

#include "sdk/ui/calram_guide_mgr_wnd.hpp"
#include "sdk/net/cclient_config.hpp"
#include "sdk/game/ccontroler.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/cif_wnd.hpp"
#include "sdk/process/cps_outer_interface.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstring>
#include <unordered_set>

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;

} // namespace

// --- instance: getters ---

auto cgwnd::get_parent() const -> cgwnd* {
  return ext_client::off::field_at<cgwnd*>(this, 0x030);
}

auto cgwnd::get_rect_x() const -> int {
  return ext_client::off::field_at<int>(this, 0x040);
}

auto cgwnd::get_rect_y() const -> int {
  return ext_client::off::field_at<int>(this, 0x044);
}

auto cgwnd::get_rect_w() const -> int {
  return ext_client::off::field_at<int>(this, 0x048);
}

auto cgwnd::get_rect_h() const -> int {
  return ext_client::off::field_at<int>(this, 0x04C);
}

auto cgwnd::get_control_id() const -> int {
  return ext_client::off::field_at<int>(this, 0x02C);
}

auto cgwnd::get_unique_id() const -> int {
  return ext_client::off::field_at<int>(this, 0x034);
}

auto cgwnd::get_bounds() const -> cgwnd_bounds {
  return {get_rect_x(), get_rect_y(), get_rect_w(), get_rect_h()};
}

auto cgwnd::get_topmost_ancestor() -> cgwnd* {
  if (!get_vftable()) {
    return nullptr;
  }
  auto* top = this;
  for (int guard = 0; guard < 64; ++guard) {
    auto* p = top->get_parent();
    if (!p || !p->get_vftable()) {
      break;
    }
    top = p;
  }
  return top;
}

// --- instance: predicates ---

auto cgwnd::is_visible() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x061) != 0;
}

auto cgwnd::is_hit_test_contains(int x, int y) const -> bool {
  if (!is_visible()) {
    return false;
  }
  const auto fn = as_fn<cgwnd_fn::hit_test_contains>(0x00D6A830);
  return fn(this, x, y);
}

auto cgwnd::is_live() const -> bool {
  return ext_client::gfx_runtime::get_runtime_class(this) != nullptr;
}

// --- instance: mutators ---

auto cgwnd::set_rect_w(int width) -> void {
  ext_client::off::field_at<int>(this, 0x048) = width;
}

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
  const auto fn = as_fn<cgwnd_fn::set_anim>(0x00956180);
  fn(this, alpha, speed, delay, mode);
}

auto cgwnd::destroy() -> void {
  const auto* vft = get_vftable();
  const auto dtor = as_fn<cgwnd_fn::scalar_deleting_dtor>(vft[2]);
  dtor(this, 1);
}

// --- static: getters ---

auto cgwnd::get_client_config() -> cclient_config* {
  const auto fn = as_fn<cgwnd_fn::get_client_config>(0x004DE120);
  return fn();
}

auto cgwnd::get_client_data_version() -> unsigned {
  const auto* cfg = get_client_config();
  if (!cfg) {
    return 1000u;
  }
  return ext_client::off::field_at<unsigned>(cfg, 0x10C) + 1000u;
}

auto cgwnd::get_screen_height() -> int {
  const auto fn = as_fn<cgwnd_fn::get_screen_size>(0x004DE150);
  const auto* screen = static_cast<const std::uint8_t*>(fn());
  if (!screen) {
    return 0;
  }
  return ext_client::off::field_at<int>(screen, 40/*height*/);
}

auto cgwnd::get_screen_width() -> int {
  const auto fn = as_fn<cgwnd_fn::get_screen_size>(0x004DE150);
  const auto* screen = static_cast<const std::uint8_t*>(fn());
  if (!screen) {
    return 0;
  }
  return ext_client::off::field_at<int>(screen, 36/*width*/);
}

auto cgwnd::get_interface_under_cursor() -> cgwnd* {
  auto* wnd = global_at<cgwnd*>(0x013BAFA8);
  if (!wnd) {
    return nullptr;
  }
  return wnd;
}

auto cgwnd::get_manager() -> void* {
  const auto fn = as_fn<cgwnd_fn::get_manager>(0x00583100);
  return fn();
}

auto cgwnd::get_game_ui_root() -> cgwnd* {
  const auto* gfx = global_at<const void*>(0x013BAE00);
  if (!gfx) {
    return nullptr;
  }
  auto* root = ext_client::off::field_at<cgwnd*>(gfx, 0x024);
  if (!root) {
    return nullptr;
  }
  return root;
}

auto cgwnd::get_pick_at_point(cgwnd* root, int x, int y) -> cgwnd* {
  if (!root) {
    return nullptr;
  }
  const auto fn = as_fn<cgwnd_fn::pick_at_point>(0x00D6CAA0);
  const int result = fn(root, x, y);
  auto* wnd = reinterpret_cast<cgwnd*>(result);
  return wnd ? wnd : nullptr;
}

auto cgwnd::get_child_by_unique_id(cgwnd* parent, int unique_id) -> cgwnd* {
  if (!parent) {
    return nullptr;
  }
  const auto fn = as_fn<cgwnd_fn::get_child_by_unique_id>(0x00407E50);
  auto* child = fn(parent, unique_id);
  return child ? child : nullptr;
}

auto cgwnd::get_type_name(const void* obj) -> const char* {
  return ext_client::gfx_runtime::get_class_name(obj);
}

auto cgwnd::get_type_name_vftable(std::uint32_t vftable) -> const char* {
  (void)vftable;
  return "unknown";
}

// --- static: predicates ---

auto cgwnd::is_pickable(const cgwnd* wnd) -> bool {
  if (!wnd) {
    return false;
  }
  const auto fn = as_fn<cgwnd_fn::is_visible>(0x00D6A990);
  return fn(const_cast<cgwnd*>(wnd)) != 0;
}

// --- static: mutators ---

auto cgwnd::set_position(cgwnd* wnd, int x, int y) -> int {
  if (!wnd) {
    return 0;
  }
  const auto fn = as_fn<cgwnd_fn::set_position>(0x00D6BEE0);
  return fn(wnd, x, y);
}

auto cgwnd::set_size(cgwnd* wnd, int width, int height) -> int {
  if (!wnd) {
    return 0;
  }
  const auto fn = as_fn<cgwnd_fn::set_size>(0x00D6CBD0);
  return fn(wnd, width, height);
}

auto cgwnd::set_visible(cgwnd* wnd, bool visible) -> int {
  if (!wnd) {
    return 0;
  }
  const auto vis = static_cast<std::uint8_t>(visible ? 1 : 0);
  const auto* vft = wnd->get_vftable();
  if (vft) {
    const auto slot23 = vft[23];
    if (slot23 == 0x0072F4B0 ||
        slot23 == 0x00732AA0 ||
        slot23 == 0x0040F230) {
      return as_fn<cgwnd_fn::set_visible>(slot23)(wnd, vis);
    }
  }
  return as_fn<cgwnd_fn::set_visible>(0x00D6A960)(wnd, vis);
}

auto cgwnd::refresh_interface_under_cursor() -> bool {
  auto* gfx = global_at<void*>(0x013BAE00);
  if (!gfx) {
    return false;
  }
  const auto fn = as_fn<cgwnd_fn::refresh_interface_under_cursor>(
      0x00D70440);
  fn(gfx);
  return true;
}

// --- child iteration ---

namespace {

  auto has_outer_map(const cgwnd* root) -> bool {
    if (!root || !root->get_vftable()) {
      return false;
    }
    const char* name = ccontroler::factory_entry_name(root);
    return name != nullptr && std::strncmp(name, "CPS", 3) == 0;
  }

  auto cg_interface_map(const cgwnd* root) -> const ext_client::msvc9::n_map<int, void*>* {
    if (!cg_interface::is_instance(root)) {
      return nullptr;
    }
    return &ext_client::off::field_at<ext_client::msvc9::n_map<int, void*>>(reinterpret_cast<const cg_interface*>(root), 0x374);
  }

  auto if_wnd_map(const cgwnd* wnd) -> const ext_client::msvc9::n_map<int, void*>* {
    if (!wnd || cg_interface::is_instance(wnd)) {
      return nullptr;
    }
    const char* class_name = ext_client::gfx_runtime::get_class_name(wnd);
    if (std::strncmp(class_name, "CIF", 3) != 0 &&
        std::strncmp(class_name, "CAlramGuideMgr", 14) != 0) {
      return nullptr;
    }
    return &ext_client::off::field_at<ext_client::msvc9::n_map<int, void*>>(static_cast<cif_wnd*>(const_cast<cgwnd*>(wnd)), 0x1C4);
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
    if (!ext_client::off::field_at<std::n_list<cgwnd*>>(child, 0x078).empty()) {
      ext_client::off::field_at<std::n_list<cgwnd*>>(child, 0x078).for_each([&](cgwnd* c) {
        if (!c || !c->is_live() || c == child) {
          return;
        }
        visit_each_child_unique(c, seen, visit, ctx);
        walk_each_child_subtrees(c, depth + 1, max_depth, seen, visit, ctx);
      });
    }
    if (auto* map = cg_interface_map(child)) {
      map->for_each([&](int, void* value) {
        auto* c = reinterpret_cast<cgwnd*>(value);
        if (!c || c == child) {
          return;
        }
        visit_each_child_unique(c, seen, visit, ctx);
        walk_each_child_subtrees(c, depth + 1, max_depth, seen, visit, ctx);
      });
    }
    if (auto* map = if_wnd_map(child)) {
      map->for_each([&](int, void* value) {
        auto* c = reinterpret_cast<cgwnd*>(value);
        if (!c || c == child) {
          return;
        }
        visit_each_child_unique(c, seen, visit, ctx);
        walk_each_child_subtrees(c, depth + 1, max_depth, seen, visit, ctx);
      });
    }
  }

  auto walk_each_recursive(cgwnd* root, int depth, int max_depth, std::unordered_set<cgwnd*>& seen, cgwnd::child_visitor_fn visit, void* ctx) -> void {
    if (!root || walk_depth_exceeded(depth, max_depth) || !visit) {
      return;
    }
    if (!ext_client::off::field_at<std::n_list<cgwnd*>>(root, 0x078).empty()) {
      ext_client::off::field_at<std::n_list<cgwnd*>>(root, 0x078).for_each([&](cgwnd* c) {
        if (!c || !c->is_live() || c == root) {
          return;
        }
        visit_each_child_unique(c, seen, visit, ctx);
        walk_each_child_subtrees(c, depth + 1, max_depth, seen, visit, ctx);
      });
    }
  }

} // namespace

auto cgwnd::for_each_child(child_visitor_fn visit, void* ctx) -> void {
  if (!visit) {
    return;
  }

  if (!ext_client::off::field_at<std::n_list<cgwnd*>>(this, 0x078).empty()) {
    ext_client::off::field_at<std::n_list<cgwnd*>>(this, 0x078).for_each([&](cgwnd* child) {
      if (!child || !child->is_live() || child == this) {
        return;
      }
      visit(child, ctx);
    });
  }

  if (has_outer_map(this)) {
    auto* outer = reinterpret_cast<cps_outer_interface*>(this);
    ext_client::off::field_at<ext_client::msvc9::n_map<int, void*>>(outer, 0x0B0).for_each([&](int, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || !child->is_live() || child == this) {
        return;
      }
      visit(child, ctx);
    });
  }

  if (auto* map = cg_interface_map(this)) {
    map->for_each([&](int, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || !child->is_live() || child == this) {
        return;
      }
      visit(child, ctx);
    });
  }

  if (auto* map = if_wnd_map(this)) {
    map->for_each([&](int, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || !child->is_live() || child == this) {
        return;
      }
      visit(child, ctx);
    });
  }

  if (ext_client::gfx_runtime::is_class_name_match(this, "CAlramGuideMgrWnd")) {
    auto* mgr = reinterpret_cast<calram_guide_mgr_wnd*>(this);
    std::pair<child_visitor_fn, void*> ctx_pair{visit, ctx};
    calram_guide_mgr_wnd::for_each_guide(
      mgr,
      [](cgwnd* guide, void* c) {
        if (guide && guide->is_live()) {
          auto* p = static_cast<std::pair<child_visitor_fn, void*>*>(c);
          p->first(guide, p->second);
        }
      },
      &ctx_pair);
  }
}

auto cgwnd::walk_each(int max_depth, child_visitor_fn visit, void* ctx) -> void {
  if (!visit) {
    return;
  }

  std::unordered_set<cgwnd*> seen;
  visit_each_child_unique(this, seen, visit, ctx);
  walk_each_recursive(this, 1, max_depth, seen, visit, ctx);

  if (has_outer_map(this)) {
    auto* outer = reinterpret_cast<cps_outer_interface*>(this);
    ext_client::off::field_at<ext_client::msvc9::n_map<int, void*>>(outer, 0x0B0).for_each([&](int, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || child == this) {
        return;
      }
      visit_each_child_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, 1, max_depth, seen, visit, ctx);
    });
  }

  if (auto* map = cg_interface_map(this)) {
    map->for_each([&](int, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || child == this) {
        return;
      }
      visit_each_child_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, 1, max_depth, seen, visit, ctx);
    });
  }

  if (auto* map = if_wnd_map(this)) {
    map->for_each([&](int, void* value) {
      auto* child = reinterpret_cast<cgwnd*>(value);
      if (!child || child == this) {
        return;
      }
      visit_each_child_unique(child, seen, visit, ctx);
      walk_each_child_subtrees(child, 1, max_depth, seen, visit, ctx);
    });
  }
}
