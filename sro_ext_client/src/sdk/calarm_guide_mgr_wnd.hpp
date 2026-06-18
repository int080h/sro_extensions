#pragma once

#include "cif_decorated_static.hpp"
#include "cif_wnd.hpp"
#include "utils/layout.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

struct calarm_guide_mgr_wnd;
struct cg_interface;

struct calarm_guide_mgr_wnd_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, calarm_guide_mgr_wnd* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, calarm_guide_mgr_wnd* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(equals, int, calarm_guide_mgr_wnd* self, calarm_guide_mgr_wnd* other);
  VFN_THISCALL(null_05, void, calarm_guide_mgr_wnd* self);
  VFN_CDECL(create_instance, calarm_guide_mgr_wnd*);
  VFN_THISCALL(on_create, int, calarm_guide_mgr_wnd* self, int mode);
  VFN_THISCALL(on_destroy, int, calarm_guide_mgr_wnd* self, int mode);
  VFN_THISCALL(on_init, int, calarm_guide_mgr_wnd* self, int mode);
  VFN_THISCALL(null_10, int, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(null_11, int, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(null_12, void, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(on_cmd, int, calarm_guide_mgr_wnd* self, int cmd);
  VFN_THISCALL(on_render, int, calarm_guide_mgr_wnd* self, int pass);
  VFN_THISCALL(null_15, int, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(run_update_chain, int, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(on_update, int, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(on_timer_id, int, calarm_guide_mgr_wnd* self, int timer_id);
  VFN_THISCALL(on_size, int, calarm_guide_mgr_wnd* self, int size_param);
  VFN_THISCALL(null_19, int, calarm_guide_mgr_wnd* self);
  VFN_THISCALL(on_move, int, calarm_guide_mgr_wnd* self, int move_param);
  VFN_THISCALL(on_show, int, calarm_guide_mgr_wnd* self, int show_param);
  VFN_THISCALL(on_hide, int, calarm_guide_mgr_wnd* self, int hide_param);
  VFN_THISCALL(set_visible, int, calarm_guide_mgr_wnd* self, std::uint8_t visible);
  VFN_THISCALL(update_alarm_state, int, calarm_guide_mgr_wnd* self);
};

// CAlramGuideMgrWnd: right-side alarm icon strip (calendar, lamp, chest, facebook, ...).
class calarm_guide_mgr_wnd : public cif_wnd {
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

  friend auto clear_promo_alarm_data(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void;

  DECLARE_SDK_VTABLE(calarm_guide_mgr_wnd_vtable, mgr_vftable)
  DECLARE_SDK_WND_HELPERS(calarm_guide_mgr_wnd, ext_client::offsets::calarm_guide_mgr_wnd::vtable::address)

  // Live instance captured from OnCreate (same pattern as cps_title::current).
  static auto current() -> calarm_guide_mgr_wnd*;
  // Cheap lookup: current -> cached -> get_ui_child (no map walk).
  static auto resolve() -> calarm_guide_mgr_wnd*;
  // Inspector only — walks CGInterface res map (never call per-frame).
  static auto resolve_from_res_map() -> calarm_guide_mgr_wnd*;
  // All 8 strip slots constructed and readable — safe to hide.
  static auto mgr_ready(const calarm_guide_mgr_wnd* mgr) -> bool;
  // True when mgr is the live CGInterface alarm-strip child (not a stale cached pointer).
  static auto is_attached_to_iface(const calarm_guide_mgr_wnd* mgr) -> bool;
  // In-world HUD only — false on login / character select / teardown.
  static auto ingame_alarm_strip_active() -> bool;
  // Promo corner guides (guide_host 0x9E) — does not require strip slot refresh.
  static auto ingame_promo_ui_active() -> bool;
  static auto invalidate_cache() -> void;

  auto alarm_slot(std::size_t index) -> cif_decorated_static*;
  auto alarm_slot(std::size_t index) const -> const cif_decorated_static*;
  auto effect_widget(std::size_t index) -> cif_decorated_static*;
  auto effect_widget(std::size_t index) const -> const cif_decorated_static*;
  auto alarm_status(std::size_t index) const -> std::uint8_t;

  static auto has_promo_target(promo_target mask, promo_target bit) -> bool;

  // Masked strip/guide hide (facebook = slot 6 / fx 2, magic_lamp = slot 7 / fx 3, daily_login = guide 280).
  auto suppress_promo(promo_target targets = k_promo_all) -> void;
  // Visibility-only hide — does not destroy guides or clear alarm entries (reversible).
  auto soft_hide_promo(promo_target targets = k_promo_all) -> void;
  auto restore_promo_visibility(promo_target targets = k_promo_all) -> void;
  auto hide_promo(promo_target targets = k_promo_all) -> void;
  auto hide_promo_effects(promo_target targets = k_promo_all) -> void;
  auto hide_promo_guides(promo_target targets = k_promo_all) -> void;
  auto remove_promo_guides(promo_target targets = k_promo_all) -> void;

  // Strip mgr (child 0x9D): neutralize calarm_data + vanilla update_alarm_state (sub_739A80).
  auto apply_promo_hide(promo_target targets = k_promo_all) -> void;
  auto apply_promo_show(promo_target targets = k_promo_all) -> void;
  auto update_alarm_state() -> int;

  auto suppress_promo_strip() -> void;
  auto hide_promo_strip() -> void;

  static auto hide_promo_widget(cgwnd* widget) -> void;

  struct slot_debug_info {
    int entry_type = -1;
    bool entry_active = false;
    std::uint8_t alarm_status = 0;
    char yellow_path[128]{};
    char red_path[128]{};
    char ref_icon_path[128]{};
    char tex_path[128]{};
    cif_decorated_static* slot_widget = nullptr;
    cif_decorated_static* effect_widget = nullptr;
  };

  static auto read_slot_debug(const calarm_guide_mgr_wnd* mgr, std::size_t index, slot_debug_info& out) -> void;

  // Validated slot/effect pointers (null if missing or wrong vtable).
  static auto loose_slot_widget(const calarm_guide_mgr_wnd* mgr, std::size_t index) -> cgwnd*;
  static auto loose_effect_widget(const calarm_guide_mgr_wnd* mgr, std::size_t index_0based) -> cgwnd*;
  // refresh_slot_icons calls into each active slot's CIFStatic subobject — skip if any are missing.
  static auto refresh_slots_safe(const calarm_guide_mgr_wnd* mgr) -> bool;

  // Guide list lives in the CIFWnd tail (+0x374); use accessors, not derived-class fields.
  auto guide_icon_count() const -> std::uint8_t;

  // DevKit-parity guide API (vanilla GetGuide also creates on miss).
  auto get_guide(unsigned guide_id) -> cgwnd*;
  auto create_guide_icon(int guide_id) -> cgwnd*;
  auto remove_guide(int guide_id) -> int;
  auto update_guide_positions() -> int;
  auto is_guide_available(unsigned guide_id) const -> bool;
  auto remove_all_guides() -> void;

  static auto set_effect_state(calarm_guide_mgr_wnd* mgr, unsigned char fx_index, int visible) -> unsigned char;
  static auto show_promo_effect(calarm_guide_mgr_wnd* mgr, char slot_index) -> int;
  static auto refresh_slot_icons(calarm_guide_mgr_wnd* mgr) -> int;
  static auto apply_iface_promo_hide(cg_interface* iface, promo_target targets) -> void;
  static auto apply_iface_promo_show(cg_interface* iface, promo_target targets) -> void;

  template<typename Fn> auto for_each_guide(Fn&& fn) const -> void;

  static auto set_current(calarm_guide_mgr_wnd* instance) -> void;
  static auto for_each_guide(const calarm_guide_mgr_wnd* mgr, void (*visit)(cgwnd*, void*), void* ctx) -> void;

private:
  PAD_TO(ext_client::offsets::cif_wnd::size, ext_client::offsets::calarm_guide_mgr_wnd::fields::alarm_slots);
  cif_decorated_static* m_alarm_slots[8];
  PAD_TO(ext_client::offsets::calarm_guide_mgr_wnd::fields::alarm_slots + 8 * sizeof(void*),
         ext_client::offsets::calarm_guide_mgr_wnd::fields::effect_widgets);
  cif_decorated_static* m_effect_widgets[3];
  std::uint8_t m_alarm_status[8];
  int m_slot_flags[8];
  int m_slot_extra_state[8];
  PAD_TO(ext_client::offsets::calarm_guide_mgr_wnd::fields::slot_extra_state + sizeof(m_slot_extra_state),
         ext_client::offsets::calarm_guide_mgr_wnd::size);
};

template<typename Fn> auto calarm_guide_mgr_wnd::for_each_guide(Fn&& fn) const -> void {
  struct ctx_t {
    Fn* fn;
  };
  ctx_t ctx{&fn};
  calarm_guide_mgr_wnd::for_each_guide(this, [](cgwnd* widget, void* raw) { (*static_cast<ctx_t*>(raw)->fn)(widget); }, &ctx);
}

static_assert(sizeof(calarm_guide_mgr_wnd) == ext_client::offsets::calarm_guide_mgr_wnd::size, "calarm_guide_mgr_wnd size mismatch");
