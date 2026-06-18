#pragma once

#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

// CTextBoard — secondary MI base @ +0x84 (vtable 0xFF47F4).

struct ctextboard_vtable {
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
};

class ctextboard {
public:
  ctextboard_vtable* vftable;
  DECLARE_SDK_VTABLE(ctextboard_vtable, board_vftable)
};
