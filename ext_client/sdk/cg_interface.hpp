#pragma once

#include "cgwnd.hpp"
#include "utils/layout.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class calarm_store;
class calarm_guide_mgr_wnd;

// CGInterface — in-world UI root (g_CGInterface @ ext_client::offsets::globals::cg_interface).
// Reversed from GInterface.cpp (VS2008 client, IDA).
//
// MSVC MI: CGInterface : CIFWnd, GIGInterface
//   +0x000  primary vtable
//   +0x084  GIGInterface vtable (after 0x84-byte CGWnd head)
//   +0x374  CResIDManager (main get_ui_child map — not cif_wnd::ui_res_map @ +0x1C4)
//   +0x9C8  sizeof(CGInterface)
//
// Per-field offsets: ext_client::offsets::cg_interface::fields in offsets.hpp

struct gig_interface_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, void* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, void* self);
  VFN_THISCALL(equals, int, void* self, void* other);
  VFN_THISCALL(null_05, void, void* self);
  VFN_CDECL(create_instance, void*);
  VFN_THISCALL(on_create, int, void* self, int mode);
  VFN_THISCALL(on_destroy, int, void* self, int mode);
  VFN_THISCALL(on_init, int, void* self, int mode);
  VFN_THISCALL(null_10, int, void* self);
  VFN_THISCALL(null_11, int, void* self);
  VFN_THISCALL(on_cmd, int, void* self, int cmd);
  VFN_THISCALL(null_14, int, void* self);
  VFN_THISCALL(null_15, int, void* self);
  VFN_THISCALL(null_16, int, void* self);
  VFN_THISCALL(on_update, int, void* self);
  VFN_THISCALL(on_timer_id, int, void* self, int timer_id);
};

struct cg_interface_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, void* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, void* self);
  VFN_THISCALL(equals, int, void* self, void* other);
  VFN_THISCALL(null_05, void, void* self);
  VFN_CDECL(create_instance, void*);
  VFN_THISCALL(on_create, int, void* self, int mode);
  VFN_THISCALL(on_destroy, int, void* self, int mode);
  VFN_THISCALL(on_init, int, void* self, int mode);
  VFN_THISCALL(null_10, int, void* self);
  VFN_THISCALL(null_11, int, void* self);
  VFN_THISCALL(null_12, void, void* self);
  VFN_THISCALL(on_cmd, int, void* self, int cmd);
  VFN_THISCALL(on_render, int, void* self, int pass);
  VFN_THISCALL(null_15, int, void* self);
  VFN_THISCALL(run_update_chain, int, void* self);
  VFN_THISCALL(on_wnd_message, int, void* self, int msg);
  VFN_THISCALL(on_timer_id, int, void* self, int timer_id);
  VFN_THISCALL(on_size, int, void* self, int size_param);
  VFN_THISCALL(null_19, int, void* self);
  VFN_THISCALL(on_move, int, void* self, int move_param);
  VFN_THISCALL(on_show, int, void* self, int show_param);
  VFN_THISCALL(on_hide, int, void* self, int hide_param);
  VFN_THISCALL(set_visible, int, void* self, std::uint8_t visible);
};

class cg_interface {
public:
  DECLARE_SDK_VTABLE(cg_interface_vtable, iface_vftable)
  DECLARE_SDK_CAST(cgwnd, as_gwnd)

  static auto get() -> cg_interface*;
  static auto is_ready() -> bool;
  static auto is_instance(const void* ptr) -> bool;
  // In-world HUD (main_hud exists); does not require alarm mgr / guide host.
  static auto is_ingame_hud_ready() -> bool;

  auto ui_res_map() -> void*;
  auto ui_res_map() const -> const void*;
  auto get_ui_child(int control_id, bool add_base_key = true) -> void*;

  auto alarm_store() -> calarm_store*;
  auto alarm_store() const -> const calarm_store*;
  auto alarm_guide_mgr() -> calarm_guide_mgr_wnd*;
  auto alarm_guide_mgr() const -> const calarm_guide_mgr_wnd*;
  auto guide_host(bool add_base_key = true) -> void*;

  auto show_magic_lamp_guide(bool show) -> unsigned int;
  auto show_daily_login_guide(bool show) -> unsigned int;
  auto show_facebook_guide(bool show) -> unsigned int;
  auto show_web_item_alarm_guide(bool show) -> unsigned int;
  auto show_macro_guide(bool show) -> unsigned int;

  static auto known_child_id_count() -> std::size_t;
  static auto known_child_id(std::size_t index) -> int;

  auto gig_vftable() -> gig_interface_vtable*;
  auto gig_vftable() const -> const gig_interface_vtable*;

  template<typename T> auto field_ptr(std::size_t byte_offset) const -> T* {
    return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(const_cast<cg_interface*>(this)) + byte_offset);
  }

  auto alarm_guide_mgr_popup() const -> void*;
  auto modal_wnd() const -> void*;

private:
  cg_interface_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cg_interface::fields::gig_interface_vftable);
  gig_interface_vtable* gig_interface_vftable;
  PAD_TO(ext_client::offsets::cg_interface::fields::gig_interface_vftable + sizeof(void*),
         ext_client::offsets::cg_interface::fields::ui_res_map);
  PAD_BYTES(m_ui_res_map, ext_client::msvc9::ui_res_map_size);
  PAD_TO(ext_client::offsets::cg_interface::fields::ui_res_map + ext_client::msvc9::ui_res_map_size,
         ext_client::offsets::cg_interface::size);

  static inline auto check_layout() -> void {
    static_assert(offsetof(cg_interface, gig_interface_vftable) == ext_client::offsets::cg_interface::fields::gig_interface_vftable);
    static_assert(offsetof(cg_interface, m_ui_res_map) == ext_client::offsets::cg_interface::fields::ui_res_map);
    static_assert(sizeof(cg_interface) == ext_client::offsets::cg_interface::size);
  }
};
