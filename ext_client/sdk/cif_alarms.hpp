#pragma once

#include "cif_decorated_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

// Forward declarations
class cif_daily_login_alarm;
class cif_magic_lamp_alarm;
class cif_facebook_link_alarm;

// Daily Login Alarm
using cif_daily_login_alarm_vtable = cif_decorated_static_vtable;

// CIFDailyLoginAlram: daily login shortcut (icon\etc\dailylogin_0.ddj).
class cif_daily_login_alarm : public cif_decorated_static {
public:
  DECLARE_SDK_VTABLE(cif_daily_login_alarm_vtable, daily_login_vftable)
  DECLARE_SDK_WND_HELPERS(cif_daily_login_alarm, ext_client::offsets::cif_daily_login_alram::vtable::address)
};

static_assert(sizeof(cif_daily_login_alarm) == ext_client::offsets::cif_daily_login_alram::size,
              "cif_daily_login_alarm size mismatch");


// Magic Lamp Alarm
using cif_magic_lamp_alarm_vtable = cif_decorated_static_vtable;

// CIFMagicLampAlram: web gacha / magic lamp shortcut (icon\etc\webgacha2_0.ddj).
class cif_magic_lamp_alarm : public cif_decorated_static {
public:
  DECLARE_SDK_VTABLE(cif_magic_lamp_alarm_vtable, magic_lamp_vftable)
  DECLARE_SDK_WND_HELPERS(cif_magic_lamp_alarm, ext_client::offsets::cif_magic_lamp_alram::vtable::address)
};

static_assert(sizeof(cif_magic_lamp_alarm) == ext_client::offsets::cif_magic_lamp_alram::size,
              "cif_magic_lamp_alarm size mismatch");


// Facebook Link Alarm
struct cif_facebook_link_alarm_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cif_facebook_link_alarm* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, cif_facebook_link_alarm* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cif_facebook_link_alarm* self);
  VFN_THISCALL(equals, int, cif_facebook_link_alarm* self, cif_facebook_link_alarm* other);
  VFN_THISCALL(null_05, void, cif_facebook_link_alarm* self);
  VFN_CDECL(create_instance, cif_facebook_link_alarm*);
  VFN_THISCALL(on_create, int, cif_facebook_link_alarm* self, int mode);
  VFN_THISCALL(on_destroy, int, cif_facebook_link_alarm* self, int mode);
  VFN_THISCALL(on_init, int, cif_facebook_link_alarm* self, int mode);
  VFN_THISCALL(null_10, int, cif_facebook_link_alarm* self);
  VFN_THISCALL(null_11, int, cif_facebook_link_alarm* self);
  VFN_THISCALL(null_12, void, cif_facebook_link_alarm* self);
  VFN_THISCALL(on_cmd, int, cif_facebook_link_alarm* self, int cmd);
  VFN_THISCALL(on_render, int, cif_facebook_link_alarm* self, int pass);
  VFN_THISCALL(null_15, int, cif_facebook_link_alarm* self);
  VFN_THISCALL(run_update_chain, int, cif_facebook_link_alarm* self);
  VFN_THISCALL(on_update, int, cif_facebook_link_alarm* self);
  VFN_THISCALL(on_timer_id, int, cif_facebook_link_alarm* self, int timer_id);
  VFN_THISCALL(on_size, int, cif_facebook_link_alarm* self, int size_param);
  VFN_THISCALL(null_19, int, cif_facebook_link_alarm* self);
  VFN_THISCALL(on_move, int, cif_facebook_link_alarm* self, int move_param);
  VFN_THISCALL(on_show, int, cif_facebook_link_alarm* self, int show_param);
  VFN_THISCALL(on_hide, int, cif_facebook_link_alarm* self, int hide_param);
  VFN_THISCALL(set_visible, int, cif_facebook_link_alarm* self, std::uint8_t visible);
  VFN_THISCALL(on_wnd_message, void, cif_facebook_link_alarm* self, void* msg);
  VFN_THISCALL(open_link_dialog, int, cif_facebook_link_alarm* self, int a2, int a3, int a4);
};

// CIFFacebookLinkAlram: Facebook shortcut button (icon\etc\facebook_*.ddj).
class cif_facebook_link_alarm : public cif_decorated_static {
public:
  DECLARE_SDK_VTABLE(cif_facebook_link_alarm_vtable, facebook_vftable)
  DECLARE_SDK_WND_HELPERS(cif_facebook_link_alarm, ext_client::offsets::cif_facebook_link_alram::vtable::address)
};

static_assert(sizeof(cif_facebook_link_alarm) == ext_client::offsets::cif_facebook_link_alram::size,
              "cif_facebook_link_alarm size mismatch");
