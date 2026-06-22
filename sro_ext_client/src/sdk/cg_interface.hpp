#pragma once

#include "cif_wnd.hpp"
#include "ctextboard.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class cif_main_popup;
class calram_guide_mgr_wnd;

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
  DECLARE_SDK_CAST(cif_wnd, as_cif_wnd)
  DECLARE_SDK_CAST(cgwnd, as_gwnd)

  static auto get() -> cg_interface*;
  static auto is_ready() -> bool;
  static auto is_instance(const void* ptr) -> bool;
  static auto is_ingame_hud_ready() -> bool;

  auto ui_res_map() -> ext_client::msvc9::n_map<int, void*>* { return &m_ui_res_map; }
  auto ui_res_map() const -> const ext_client::msvc9::n_map<int, void*>* { return &m_ui_res_map; }
  auto get_ui_child(int control_id, bool add_base_key = true) -> void*;
  auto textboard() -> ctextboard*;
  auto textboard() const -> const ctextboard*;

  auto alarm_store() -> cif_main_popup*;
  auto alarm_store() const -> const cif_main_popup*;
  auto alarm_guide_mgr() -> calram_guide_mgr_wnd*;
  auto alarm_guide_mgr() const -> const calram_guide_mgr_wnd*;
  auto guide_host(bool add_base_key = true) -> void*;

  auto show_magic_lamp_guide(bool show) -> unsigned int;
  auto show_daily_login_guide(bool show) -> unsigned int;
  auto show_facebook_guide(bool show) -> unsigned int;
  auto show_web_item_alarm_guide(bool show) -> unsigned int;
  auto show_macro_guide(bool show) -> unsigned int;

  static auto known_child_id_count() -> std::size_t;
  static auto known_child_id(std::size_t index) -> int;

  auto textboard_vftable() -> ctextboard_vtable*;
  auto textboard_vftable() const -> const ctextboard_vtable*;

  template<typename T> auto field_ptr(std::size_t byte_offset) const -> T* {
    return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(const_cast<cg_interface*>(this)) + byte_offset);
  }

  auto alarm_guide_mgr_popup() const -> void*;
  auto modal_wnd() const -> void*;

  using res_map_t = ext_client::msvc9::n_map<int, void*>;

  cg_interface_vtable* vftable;
  union {
    DEFINE_MEMBER_N(ctextboard_vtable* m_textboard_vftable, ext_client::offsets::cg_interface::fields::textboard_vftable - sizeof(void*));
    DEFINE_MEMBER_N(res_map_t m_ui_res_map, ext_client::offsets::cg_interface::fields::ui_res_map - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_ui_res_map_tail[ext_client::offsets::cres_id_manager::size - sizeof(res_map_t)],
                    ext_client::offsets::cg_interface::fields::ui_res_map + sizeof(res_map_t) - sizeof(void*));
  };
};

