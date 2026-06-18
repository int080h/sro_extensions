#pragma once

#include "cif_static.hpp"

#include <cstdint>

#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

class cif_decorated_static;

struct cif_decorated_static_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cif_decorated_static* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, cif_decorated_static* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cif_decorated_static* self);
  VFN_THISCALL(equals, int, cif_decorated_static* self, cif_decorated_static* other);
  VFN_THISCALL(null_05, void, cif_decorated_static* self);
  VFN_CDECL(create_instance, cif_decorated_static*);
  VFN_THISCALL(on_create, int, cif_decorated_static* self, int mode);
  VFN_THISCALL(on_destroy, int, cif_decorated_static* self, int mode);
  VFN_THISCALL(on_init, int, cif_decorated_static* self, int mode);
  VFN_THISCALL(null_10, int, cif_decorated_static* self);
  VFN_THISCALL(null_11, int, cif_decorated_static* self);
  VFN_THISCALL(null_12, void, cif_decorated_static* self);
  VFN_THISCALL(on_cmd, int, cif_decorated_static* self, int cmd);
  VFN_THISCALL(on_render, int, cif_decorated_static* self, int pass);
  VFN_THISCALL(null_15, int, cif_decorated_static* self);
  VFN_THISCALL(run_update_chain, int, cif_decorated_static* self);
  VFN_THISCALL(on_update, int, cif_decorated_static* self);
  VFN_THISCALL(on_timer_id, int, cif_decorated_static* self, int timer_id);
  VFN_THISCALL(on_size, int, cif_decorated_static* self, int size_param);
  VFN_THISCALL(null_19, int, cif_decorated_static* self);
  VFN_THISCALL(on_move, int, cif_decorated_static* self, int move_param);
  VFN_THISCALL(on_show, int, cif_decorated_static* self, int show_param);
  VFN_THISCALL(on_hide, int, cif_decorated_static* self, int hide_param);
  VFN_THISCALL(set_visible, int, cif_decorated_static* self, std::uint8_t visible);
};

// CIFDecoratedStatic: icon button used by the right-side alarm strip (dword_1179970).
class cif_decorated_static : public cif_static {
public:
  PAD_TO(ext_client::offsets::cif_static::size, ext_client::offsets::cif_decorated_static::size);

  DECLARE_SDK_VTABLE(cif_decorated_static_vtable, decorated_vftable)
  DECLARE_SDK_WND_HELPERS(cif_decorated_static, ext_client::offsets::cif_decorated_static::vtable::address)
};

static_assert(sizeof(cif_decorated_static) == ext_client::offsets::cif_decorated_static::size, "cif_decorated_static size mismatch");
