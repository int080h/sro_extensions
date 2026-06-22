#pragma once

#include "cgwnd.hpp"
#include "ctextboard.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class cif_wnd;

struct cif_wnd_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cif_wnd* self, float delta);
  CGWND_VTABLE_COMMON(cif_wnd, on_timer_id)
  VFN_THISCALL(on_focus, int, cif_wnd* self, int focus_param);
  VFN_THISCALL(on_blur, int, cif_wnd* self, int blur_param);
  VFN_THISCALL(on_enable, int, cif_wnd* self, int enable_param);
  VFN_THISCALL(on_disable, int, cif_wnd* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_THISCALL(null_37, int, cif_wnd* self);
  VFN_THISCALL(null_38, int, cif_wnd* self);
  VFN_THISCALL(null_39, int, cif_wnd* self);
  VFN_THISCALL(null_40, int, cif_wnd* self);
  VFN_THISCALL(null_41, int, cif_wnd* self);
  VFN_THISCALL(null_42, int, cif_wnd* self);
  VFN_THISCALL(null_43, int, cif_wnd* self);
  VFN_THISCALL(null_44, int, cif_wnd* self);
  VFN_THISCALL(null_45, int, cif_wnd* self);
  VFN_THISCALL(null_46, int, cif_wnd* self);
  VFN_THISCALL(null_47, int, cif_wnd* self);
};

// CIFWnd — CGWnd + CTextBoard @ +0x84, n_map<int,void*> @ +0x1C4 (CResIDManager region, 0x30 bytes). Derived IF controls @ +0x394.
using ui_res_map_t = ext_client::msvc9::n_map<int, void*>;

class cif_wnd : public cgwnd {
public:
  union {
    DEFINE_MEMBER_0(ctextboard_vtable* m_textboard_vftable, "textboard_vftable");
    DEFINE_MEMBER_N(ui_res_map_t m_ui_res_map, 0x140);
    DEFINE_MEMBER_N(std::uint8_t m_ui_res_map_tail[ext_client::offsets::cres_id_manager::size - sizeof(ui_res_map_t)], 0x14C);
  };

  DECLARE_SDK_VTABLE(cif_wnd_vtable, wnd_vftable)
  DECLARE_SDK_WND_CAST(cif_wnd)

  auto ui_res_map() -> ext_client::msvc9::n_map<int, void*>* { return &m_ui_res_map; }
  auto ui_res_map() const -> const ext_client::msvc9::n_map<int, void*>* { return &m_ui_res_map; }

  auto textboard() -> ctextboard*;
};

