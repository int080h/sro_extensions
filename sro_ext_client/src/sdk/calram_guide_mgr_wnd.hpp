#pragma once

#include "cif_decorated_static.hpp"
#include "cif_wnd.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include <cstddef>
#include <cstdint>

struct calram_guide_mgr_wnd;
struct cg_interface;

struct calram_guide_mgr_wnd_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, calram_guide_mgr_wnd* self, float delta);
  CGWND_VTABLE_COMMON(calram_guide_mgr_wnd, on_timer_id)
  VFN_THISCALL(on_focus, int, calram_guide_mgr_wnd* self, int focus_param);
  VFN_THISCALL(on_blur, int, calram_guide_mgr_wnd* self, int blur_param);
  VFN_THISCALL(on_enable, int, calram_guide_mgr_wnd* self, int enable_param);
  VFN_THISCALL(on_disable, int, calram_guide_mgr_wnd* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_THISCALL(null_37, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_38, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_39, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_40, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_41, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_42, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_43, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_44, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_45, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_46, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(null_47, int, calram_guide_mgr_wnd* self);
  VFN_THISCALL(update_alarm_state, int, calram_guide_mgr_wnd* self);
};

class calram_guide_mgr_wnd : public cif_wnd {
public:
  union {
    DEFINE_MEMBER_N(cif_decorated_static* m_alarm_slots[8], 0x554 - 0x394);
    DEFINE_MEMBER_N(cif_decorated_static* m_effect_widgets[3], 0x578 - 0x394);
    DEFINE_MEMBER_N(std::uint8_t m_alarm_status[8], 0x584 - 0x394);
    DEFINE_MEMBER_N(int m_slot_flags[8], 0x58C - 0x394);
    DEFINE_MEMBER_N(int m_slot_extra_state[8], 0x5AC - 0x394);
  };

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

  friend auto clear_promo_alarm_data(calram_guide_mgr_wnd* mgr, promo_target targets) -> void;

  DECLARE_SDK_VTABLE(calram_guide_mgr_wnd_vtable, mgr_vftable)
  DECLARE_SDK_WND_HELPERS(calram_guide_mgr_wnd, "CAlramGuideMgrWnd")

  static auto current() -> calram_guide_mgr_wnd*;
  static auto resolve() -> calram_guide_mgr_wnd*;
  static auto resolve_from_res_map() -> calram_guide_mgr_wnd*;
  static auto mgr_ready(const calram_guide_mgr_wnd* mgr) -> bool;
  static auto is_attached_to_iface(const calram_guide_mgr_wnd* mgr) -> bool;
  static auto ingame_alarm_strip_active() -> bool;
  static auto ingame_promo_ui_active() -> bool;

  auto alarm_slot(std::size_t index) -> cif_decorated_static*;
  auto alarm_slot(std::size_t index) const -> const cif_decorated_static*;
  auto effect_widget(std::size_t index) -> cif_decorated_static*;
  auto effect_widget(std::size_t index) const -> const cif_decorated_static*;
  auto alarm_status(std::size_t index) const -> std::uint8_t;

  static auto has_promo_target(promo_target mask, promo_target bit) -> bool;

  auto suppress_promo(promo_target targets = k_promo_all) -> void;
  auto soft_hide_promo(promo_target targets = k_promo_all) -> void;
  auto restore_promo_visibility(promo_target targets = k_promo_all) -> void;
  auto hide_promo(promo_target targets = k_promo_all) -> void;
  auto hide_promo_effects(promo_target targets = k_promo_all) -> void;
  auto hide_promo_guides(promo_target targets = k_promo_all) -> void;
  auto remove_promo_guides(promo_target targets = k_promo_all) -> void;

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

  static auto read_slot_debug(const calram_guide_mgr_wnd* mgr, std::size_t index, slot_debug_info& out) -> void;

  static auto loose_slot_widget(const calram_guide_mgr_wnd* mgr, std::size_t index) -> cgwnd*;
  static auto loose_effect_widget(const calram_guide_mgr_wnd* mgr, std::size_t index_0based) -> cgwnd*;
  static auto refresh_slots_safe(const calram_guide_mgr_wnd* mgr) -> bool;

  auto guide_icon_count() const -> std::uint8_t;

  auto get_guide(unsigned guide_id) -> cgwnd*;
  auto create_guide_icon(int guide_id) -> cgwnd*;
  auto remove_guide(int guide_id) -> int;
  auto update_guide_positions() -> int;
  auto is_guide_available(unsigned guide_id) const -> bool;
  auto remove_all_guides() -> void;

  static auto set_effect_state(calram_guide_mgr_wnd* mgr, unsigned char fx_index, int visible) -> unsigned char;
  static auto show_promo_effect(calram_guide_mgr_wnd* mgr, char slot_index) -> int;
  static auto refresh_slot_icons(calram_guide_mgr_wnd* mgr) -> int;
  static auto apply_iface_promo_hide(cg_interface* iface, promo_target targets) -> void;
  static auto apply_iface_promo_show(cg_interface* iface, promo_target targets) -> void;

  template<typename Fn> auto for_each_guide(Fn&& fn) const -> void;


  static auto for_each_guide(const calram_guide_mgr_wnd* mgr, void (*visit)(cgwnd*, void*), void* ctx) -> void;
};

template<typename Fn> auto calram_guide_mgr_wnd::for_each_guide(Fn&& fn) const -> void {
  struct ctx_t {
    Fn* fn;
  };
  ctx_t ctx{&fn};
  calram_guide_mgr_wnd::for_each_guide(this, [](cgwnd* widget, void* raw) { (*static_cast<ctx_t*>(raw)->fn)(widget); }, &ctx);
}
