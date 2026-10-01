#include "pch.hpp"
#include "core/core_main.hpp"

#include "sdk/ui/calram_guide_mgr_wnd.hpp"

#include "sdk/ui/cif_decorated_static.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "sdk/process/cps_character_select.hpp"
#include "sdk/process/cps_title.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstring>
#include <vector>

namespace {

  using ext_client::off::as_fn;

  using promo_target = calram_guide_mgr_wnd::promo_target;

  auto match_widget_class(const void* ptr, const char* class_name) -> bool {
    return ext_client::gfx_runtime::is_class_name_match(ptr, class_name);
  }

  auto is_promo_mgr_class(const void* ptr) -> bool {
    return match_widget_class(ptr, "CAlramGuideMgrWnd");
  }

  auto alarm_guide_mgr_at(void* ptr) -> calram_guide_mgr_wnd* {
    if (!is_promo_mgr_class(ptr)) {
      return nullptr;
    }
    return reinterpret_cast<calram_guide_mgr_wnd*>(ptr);
  }

  auto is_live_outer_screen(const void* ptr, std::uint32_t expected_vftable) -> bool {
    if (!ptr) {
      return false;
    }
    std::uint32_t vft = 0;
    vft = *reinterpret_cast<const std::uint32_t*>(ptr);
    return vft == expected_vftable;
  }

  auto outer_screen_blocks_alarm_strip() -> bool {
    if (is_live_outer_screen(cps_character_select::get_current(), 0x0103298C)) {
      return true;
    }
    if (is_live_outer_screen(cps_title::current(), 0x010344F4)) {
      return true;
    }
    return false;
  }

  auto guide_from_list_value_ptr(void* value_ptr) -> cgwnd* {
    if (!value_ptr) {
      return nullptr;
    }
    auto* widget = *reinterpret_cast<cgwnd* const*>(value_ptr);
    return widget;
  }

  auto is_ingame_alarm_strip_active() -> bool {
    if (outer_screen_blocks_alarm_strip()) {
      return false;
    }

    auto* iface = cg_interface::get();
    if (!iface || !cg_interface::is_instance(iface)) {
      return false;
    }

    if (!iface->get_ui_child(1/*main_hud*/, true)) {
      return false;
    }

    auto* mgr = iface->get_alarm_guide_mgr();
    if (!mgr || !calram_guide_mgr_wnd::is_attached_to_iface(mgr) || !calram_guide_mgr_wnd::is_mgr_ready(mgr)) {
      return false;
    }

    return true;
  }

  auto try_cheap_child_lookup(cg_interface* iface) -> calram_guide_mgr_wnd* {
    if (auto* child = iface->get_ui_child(885/*alarm_guide_mgr_alt*/, true)) {
      auto* mgr = alarm_guide_mgr_at(child);
      if (mgr && calram_guide_mgr_wnd::is_attached_to_iface(mgr)) {
        return mgr;
      }
    }

    if (auto* child = iface->get_ui_child(0x9D, true)) {
      auto* mgr = alarm_guide_mgr_at(child);
      if (mgr && calram_guide_mgr_wnd::is_attached_to_iface(mgr)) {
        return mgr;
      }
    }

    return nullptr;
  }

  auto find_mgr_in_res_map(cg_interface* iface) -> calram_guide_mgr_wnd* {
    if (!iface || !cg_interface::is_instance(iface)) {
      return nullptr;
    }

    const auto* map = iface->get_ui_res_map();
    if (!map) {
      return nullptr;
    }

    calram_guide_mgr_wnd* found = nullptr;
    map->for_each([&](int, void* value) {
      if (found || !value) {
        return;
      }
      if (auto* mgr = alarm_guide_mgr_at(value)) {
        found = mgr;
      }
    });
    return (found && calram_guide_mgr_wnd::is_attached_to_iface(found)) ? found : nullptr;
  }

  auto set_iface_guides_visible(cg_interface* iface, calram_guide_mgr_wnd::promo_target targets, bool visible) -> void {
    if (!iface || static_cast<unsigned>(targets) == 0) {
      return;
    }

    const char show = visible ? 1 : 0;
    if (calram_guide_mgr_wnd::has_promo_target(targets, calram_guide_mgr_wnd::promo_target::facebook)) {
      iface->show_facebook_guide(show != 0);
    }
    if (calram_guide_mgr_wnd::has_promo_target(targets, calram_guide_mgr_wnd::promo_target::magic_lamp)) {
      iface->show_magic_lamp_guide(show != 0);
    }
    if (calram_guide_mgr_wnd::has_promo_target(targets, calram_guide_mgr_wnd::promo_target::daily_login)) {
      iface->show_daily_login_guide(show != 0);
    }
    if (calram_guide_mgr_wnd::has_promo_target(targets, calram_guide_mgr_wnd::promo_target::web_item_alarm)) {
      iface->show_web_item_alarm_guide(show != 0);
    }
    if (calram_guide_mgr_wnd::has_promo_target(targets, calram_guide_mgr_wnd::promo_target::macro_guide)) {
      iface->show_macro_guide(show != 0);
    }
  }



} // namespace

auto calram_guide_mgr_wnd::has_promo_target(promo_target mask, promo_target bit) -> bool {
  return (static_cast<unsigned>(mask) & static_cast<unsigned>(bit)) != 0;
}

auto calram_guide_mgr_wnd::for_each_guide(const calram_guide_mgr_wnd* mgr, void (*visit)(cgwnd*, void*), void* ctx) -> void {
  if (!mgr || !visit) {
    return;
  }

  if (!match_widget_class(mgr, "CAlramGuideMgrWnd")) {
    return;
  }

  const auto* list_obj = reinterpret_cast<const std::uint8_t*>(mgr) + 0x378;
  if (!list_obj) {
    return;
  }

  ext_client::msvc9::list_ref::from_object(list_obj).for_each([&](void* value_ptr) {
    if (auto* widget = guide_from_list_value_ptr(value_ptr)) {
      visit(widget, ctx);
    }
  });
}

auto calram_guide_mgr_wnd::get_current() -> calram_guide_mgr_wnd* {
  return get_resolve();
}

auto calram_guide_mgr_wnd::is_mgr_ready(const calram_guide_mgr_wnd* mgr) -> bool {
  if (!mgr || !is_promo_mgr_class(mgr)) {
    return false;
  }
  return true;
}

auto calram_guide_mgr_wnd::is_attached_to_iface(const calram_guide_mgr_wnd* mgr) -> bool {
  if (!is_mgr_ready(mgr)) {
    return false;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return false;
  }

  auto* live_strip = iface->get_alarm_guide_mgr();
  auto* live_guide = iface->get_guide_host(true);
  return live_strip == mgr || live_guide == mgr;
}

auto calram_guide_mgr_wnd::get_resolve() -> calram_guide_mgr_wnd* {
  if (!is_ingame_alarm_strip_active()) {
    return nullptr;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return nullptr;
  }

  if (auto* mgr = iface->get_alarm_guide_mgr(); mgr && is_mgr_ready(mgr)) {
    return mgr;
  }

  return try_cheap_child_lookup(iface);
}

auto calram_guide_mgr_wnd::get_resolve_from_res_map() -> calram_guide_mgr_wnd* {
  if (auto* mgr = get_resolve()) {
    return mgr;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return nullptr;
  }

  return find_mgr_in_res_map(iface);
}

auto calram_guide_mgr_wnd::apply_promo_hide(promo_target targets) -> void {
  if (auto* iface = cg_interface::get()) {
    apply_iface_promo_hide(iface, targets);
  }
}

auto calram_guide_mgr_wnd::apply_promo_show(promo_target targets) -> void {
  if (auto* iface = cg_interface::get()) {
    apply_iface_promo_show(iface, targets);
  }
}

auto calram_guide_mgr_wnd::update_alarm_state() -> int {
  using update_alarm_state_fn = int(__thiscall*)(calram_guide_mgr_wnd*);
  const auto fn = as_fn<update_alarm_state_fn>(0x00739A80);
  return fn(this);
}

auto calram_guide_mgr_wnd::get_guide_icon_count() const -> std::uint8_t {
  const auto* field = reinterpret_cast<const std::uint8_t*>(this) + 0x374;
  return *field;
}

auto calram_guide_mgr_wnd::get_guide(const unsigned guide_id) -> cgwnd* {
  if (!calram_guide_mgr_wnd::is_mgr_ready(this)) {
    return nullptr;
  }

  using get_guide_fn = void*(__thiscall*)(calram_guide_mgr_wnd * self, unsigned int guide_id);
  const auto fn = reinterpret_cast<get_guide_fn>(0x007390B0);
  auto* raw = fn(this, guide_id);
  return raw ? static_cast<cgwnd*>(raw) : nullptr;
}

auto calram_guide_mgr_wnd::create_guide_icon(const int guide_id) -> cgwnd* {
  return get_guide(static_cast<unsigned int>(guide_id));
}

auto calram_guide_mgr_wnd::remove_guide(const int guide_id) -> int {
  if (!calram_guide_mgr_wnd::is_mgr_ready(this)) {
    return 0;
  }

  using remove_guide_fn = int(__thiscall*)(calram_guide_mgr_wnd * self, int guide_id);
  const auto fn = reinterpret_cast<remove_guide_fn>(0x007393D0);
  return fn(this, guide_id);
}

auto calram_guide_mgr_wnd::update_guide_positions() -> int {
  if (!calram_guide_mgr_wnd::is_mgr_ready(this)) {
    return 0;
  }

  using update_positions_fn = int(__thiscall*)(calram_guide_mgr_wnd * self);
  const auto fn = reinterpret_cast<update_positions_fn>(0x00738DF0);
  return fn(this);
}

auto calram_guide_mgr_wnd::is_guide_available(const unsigned guide_id) const -> bool {
  if (!calram_guide_mgr_wnd::is_mgr_ready(this)) {
    return false;
  }

  struct ctx_t {
    unsigned int wanted = 0;
    bool found = false;
  } ctx{guide_id, false};

  calram_guide_mgr_wnd::for_each_guide(
    this,
    [](cgwnd* widget, void* raw) {
      auto* visit = static_cast<ctx_t*>(raw);
      if (!widget || !visit) {
        return;
      }
      if (static_cast<unsigned int>(widget->get_unique_id()) == visit->wanted) {
        visit->found = true;
      }
    },
    &ctx);

  return ctx.found;
}

auto calram_guide_mgr_wnd::remove_all_guides() -> void {
  if (!calram_guide_mgr_wnd::is_mgr_ready(this)) {
    return;
  }

  std::vector<int> guide_ids;
  calram_guide_mgr_wnd::for_each_guide(
    this,
    [](cgwnd* widget, void* ctx) {
      if (!widget || !ctx) {
        return;
      }
      static_cast<std::vector<int>*>(ctx)->push_back(widget->get_unique_id());
    },
    &guide_ids);

  for (const int id : guide_ids) {
    remove_guide(id);
  }
}

auto calram_guide_mgr_wnd::apply_iface_promo_hide(cg_interface* iface, promo_target targets) -> void {
  if (!cg_interface::is_ingame_hud_ready() || static_cast<unsigned>(targets) == 0) {
    return;
  }
  if (!iface || !cg_interface::is_instance(iface)) {
    return;
  }

  set_iface_guides_visible(iface, targets, false);
}

auto calram_guide_mgr_wnd::apply_iface_promo_show(cg_interface* iface, promo_target targets) -> void {
  if (!cg_interface::is_ingame_hud_ready() || static_cast<unsigned>(targets) == 0) {
    return;
  }
  if (!iface || !cg_interface::is_instance(iface)) {
    return;
  }

  set_iface_guides_visible(iface, targets, true);
}
