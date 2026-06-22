#pragma once

#include "cif_decorated_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

// Forward declarations
class cif_daily_login_alram;
class cif_magic_lamp_alram;
class cif_facebook_link_alram;
class cif_web_item_alram;

// Daily Login Alram
using cif_daily_login_alram_vtable = cif_decorated_static_vtable;

// CIFDailyLoginAlram: daily login shortcut (icon\etc\dailylogin_0.ddj).
class cif_daily_login_alram : public cif_decorated_static {
public:
  DECLARE_SDK_VTABLE(cif_daily_login_alram_vtable, daily_login_vftable)
  DECLARE_SDK_WND_HELPERS(cif_daily_login_alram, "CIFDailyLoginAlram")
};

// Magic Lamp Alram
using cif_magic_lamp_alram_vtable = cif_decorated_static_vtable;

// CIFMagicLampAlram: web gacha / magic lamp shortcut (icon\etc\webgacha2_0.ddj).
class cif_magic_lamp_alram : public cif_decorated_static {
public:
  DECLARE_SDK_VTABLE(cif_magic_lamp_alram_vtable, magic_lamp_vftable)
  DECLARE_SDK_WND_HELPERS(cif_magic_lamp_alram, "CIFMagicLampAlram")
};

// Web Item Alram
using cif_web_item_alram_vtable = cif_decorated_static_vtable;

// CIFWebItemAlram: web item shortcut
class cif_web_item_alram : public cif_decorated_static {
public:
  DECLARE_SDK_VTABLE(cif_web_item_alram_vtable, web_item_vftable)
  DECLARE_SDK_WND_HELPERS(cif_web_item_alram, "CIFWebItemAlram")
};

// Facebook Link Alram
struct cif_facebook_link_alram_vtable {
  VFN_CDECL(get_static_res, void*);
  VFN_THISCALL(on_timer, void, cif_facebook_link_alram* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, cif_facebook_link_alram* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cif_facebook_link_alram* self);
  VFN_THISCALL(equals, int, cif_facebook_link_alram* self, cif_facebook_link_alram* other);
  VFN_THISCALL(null_05, void, cif_facebook_link_alram* self);
  VFN_CDECL(create_instance, cif_facebook_link_alram*);
  VFN_THISCALL(on_create, int, cif_facebook_link_alram* self, int mode);
  VFN_THISCALL(on_destroy, int, cif_facebook_link_alram* self, int mode);
  VFN_THISCALL(on_init, int, cif_facebook_link_alram* self, int mode);
  VFN_THISCALL(null_10, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_11, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_12, void, cif_facebook_link_alram* self);
  VFN_THISCALL(on_cmd, int, cif_facebook_link_alram* self, int cmd);
  VFN_THISCALL(on_render, int, cif_facebook_link_alram* self, int pass);
  VFN_THISCALL(null_15, int, cif_facebook_link_alram* self);
  VFN_THISCALL(run_update_chain, int, cif_facebook_link_alram* self);
  VFN_THISCALL(on_update, int, cif_facebook_link_alram* self);
  VFN_THISCALL(on_timer_id, int, cif_facebook_link_alram* self, int timer_id);
  VFN_THISCALL(on_size, int, cif_facebook_link_alram* self, int size_param);
  VFN_THISCALL(null_19, int, cif_facebook_link_alram* self);
  VFN_THISCALL(on_move, int, cif_facebook_link_alram* self, int move_param);
  VFN_THISCALL(on_show, int, cif_facebook_link_alram* self, int show_param);
  VFN_THISCALL(on_hide, int, cif_facebook_link_alram* self, int hide_param);
  VFN_THISCALL(set_visible, int, cif_facebook_link_alram* self, std::uint8_t visible);
  VFN_THISCALL(on_focus, int, cif_facebook_link_alram* self, int focus_param);
  VFN_THISCALL(on_blur, int, cif_facebook_link_alram* self, int blur_param);
  VFN_THISCALL(on_enable, int, cif_facebook_link_alram* self, int enable_param);
  VFN_THISCALL(on_disable, int, cif_facebook_link_alram* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_THISCALL(null_37, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_38, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_39, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_40, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_41, int, cif_facebook_link_alram* self);
  VFN_THISCALL(update_text_layout, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_43, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_44, int, cif_facebook_link_alram* self);
  VFN_THISCALL(set_text, char, cif_facebook_link_alram* self, const wchar_t* text);
  VFN_THISCALL(null_46, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_47, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_48, int, cif_facebook_link_alram* self);
  VFN_CDECL(set_text_fmt, char, cif_facebook_link_alram* self, const wchar_t* fmt, ...);
  VFN_THISCALL(null_50, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_51, int, cif_facebook_link_alram* self);
  VFN_THISCALL(null_52, int, cif_facebook_link_alram* self);
  VFN_THISCALL(open_link_dialog, int, cif_facebook_link_alram* self, int a2, int a3, int a4); // 54th method
};

// CIFFacebookLinkAlram: Facebook shortcut button (icon\etc\facebook_*.ddj).
class cif_facebook_link_alram : public cif_decorated_static {
public:
  DECLARE_SDK_VTABLE(cif_facebook_link_alram_vtable, facebook_vftable)
  DECLARE_SDK_WND_HELPERS(cif_facebook_link_alram, "CIFFacebookLinkAlram")
};
