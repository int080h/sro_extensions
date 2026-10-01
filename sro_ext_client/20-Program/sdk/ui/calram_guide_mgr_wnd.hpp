#pragma once

#include "sdk/ui/cif_decorated_static.hpp"
#include "sdk/ui/cif_wnd.hpp"
#include "utils/msvc9_stl.hpp"
#include <cstddef>
#include <cstdint>

class calram_guide_mgr_wnd;
class cg_interface;

class calram_guide_mgr_wnd : public cif_wnd {
public:
  enum class promo_target : unsigned {
    none = 0,
    facebook = 1u << 0,
    magic_lamp = 1u << 1,
    daily_login = 1u << 2,
    web_item_alarm = 1u << 3,
    macro_guide = 1u << 4,
  };

  static constexpr promo_target k_promo_all =
    static_cast<promo_target>(static_cast<unsigned>(promo_target::facebook) | static_cast<unsigned>(promo_target::magic_lamp) |
                              static_cast<unsigned>(promo_target::daily_login) | static_cast<unsigned>(promo_target::web_item_alarm) |
                              static_cast<unsigned>(promo_target::macro_guide));

  auto is_guide_available(unsigned guide_id) const -> bool;
  auto get_guide_icon_count() const -> std::uint8_t;
  auto get_guide(unsigned guide_id) -> cgwnd*;

  auto apply_promo_hide(promo_target targets = k_promo_all) -> void;
  auto apply_promo_show(promo_target targets = k_promo_all) -> void;
  auto update_alarm_state() -> int;
  auto create_guide_icon(int guide_id) -> cgwnd*;
  auto remove_guide(int guide_id) -> int;
  auto update_guide_positions() -> int;
  auto remove_all_guides() -> void;

  template<typename Fn> auto for_each_guide(Fn&& fn) const -> void;

  static auto is_mgr_ready(const calram_guide_mgr_wnd* mgr) -> bool;
  static auto is_attached_to_iface(const calram_guide_mgr_wnd* mgr) -> bool;

  static auto get_current() -> calram_guide_mgr_wnd*;
  static auto get_resolve() -> calram_guide_mgr_wnd*;
  static auto get_resolve_from_res_map() -> calram_guide_mgr_wnd*;

  static auto has_promo_target(promo_target mask, promo_target bit) -> bool;
  static auto apply_iface_promo_hide(cg_interface* iface, promo_target targets) -> void;
  static auto apply_iface_promo_show(cg_interface* iface, promo_target targets) -> void;
  static auto for_each_guide(const calram_guide_mgr_wnd* mgr, void (*visit)(cgwnd*, void*), void* ctx) -> void;
};

template<typename Fn> auto calram_guide_mgr_wnd::for_each_guide(Fn&& fn) const -> void {
  struct ctx_t {
    Fn* fn;
  };
  ctx_t ctx{&fn};
  calram_guide_mgr_wnd::for_each_guide(this, [](cgwnd* widget, void* raw) { (*static_cast<ctx_t*>(raw)->fn)(widget); }, &ctx);
}
