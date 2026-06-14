#pragma once

#include "utils/offsets.hpp"
#include "utils/layout.hpp"

#include <cstdint>

class cobj;
class cobj_child;
class cgobj;

// Root of the Silkroad process object diamond (virtual, mdisp=0).
struct cobj_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, cobj* self, char should_free);
};

struct cobj_child_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, cobj_child* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cobj_child* self);
  VFN_THISCALL(equals, int, cobj_child* self, cobj_child* other);
  VFN_THISCALL(null_05, void, cobj_child* self);
  VFN_CDECL(create_instance, cobj_child*);
  VFN_THISCALL(on_create, int, cobj_child* self, int mode);
  VFN_THISCALL(on_destroy, int, cobj_child* self, int mode);
  VFN_THISCALL(on_init, int, cobj_child* self, int mode);
  VFN_THISCALL(null_10, int, cobj_child* self);
  VFN_THISCALL(null_11, int, cobj_child* self);
  VFN_THISCALL(null_12, void, cobj_child* self);
  VFN_THISCALL(on_cmd, int, cobj_child* self, int cmd);
  VFN_THISCALL(null_14, int, cobj_child* self);
  VFN_THISCALL(null_15, int, cobj_child* self);
  VFN_THISCALL(null_16, int, cobj_child* self);
  VFN_THISCALL(on_update, int, cobj_child* self);
  VFN_THISCALL(on_timer, int, cobj_child* self, int timer_id);
  VFN_THISCALL(on_size, int, cobj_child* self, int size_param);
  VFN_THISCALL(null_19, int, cobj_child* self);
  VFN_THISCALL(on_move, int, cobj_child* self, int move_param);
  VFN_THISCALL(on_show, int, cobj_child* self, int show_param);
  VFN_THISCALL(on_hide, int, cobj_child* self, int hide_param);
  VFN_THISCALL(set_visible, int, cobj_child* self, std::uint8_t visible);
};

// cobj root (virtual, mdisp=0). No instance fields before cobj_child::field_0c.
class cobj {
public:
  cobj_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cobj::fields::region_end);
};

// cobj_child slice at the start of every outer process object.
class cobj_child : public cobj {
public:
  cobj_child_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cobj_child::fields::field_0c);
  int m_field_0c;
  PAD_TO(ext_client::offsets::cobj_child::fields::field_0c + sizeof(int), ext_client::offsets::cobj_child::fields::list_node);
  void* m_list_node[3];
};

// ======================================================================
// Real game base RTTI classes for multiple inheritance layout
// ======================================================================

class ci_entity : public cobj_child {
public:
  PAD_TO(sizeof(cobj_child), 84); // 0x54 bytes
};
static_assert(sizeof(ci_entity) == 84, "ci_entity size mismatch");

class c_animation_callback {
public:
  virtual ~c_animation_callback() = default;
};

// ci_object inherits from ci_entity and c_animation_callback at offset 84.
class ci_object : public ci_entity, public c_animation_callback {
public:
  // c_animation_callback starts exactly at offset 84 (0x54)
};
static_assert(sizeof(ci_object) == 88, "ci_object size mismatch");

class ci_gid_object : public ci_object {
public:
  PAD_TO(sizeof(ci_object), 856); // 0x358 bytes
};
static_assert(sizeof(ci_gid_object) == 856, "ci_gid_object size mismatch");

// ======================================================================
// cgobj
// ======================================================================

// cgobj — compact game-object base used by in-game net processes (CNetProcessIn).
// Uses a 7-slot vtable (not the full 24-slot cobj_child vtable).
struct cgobj_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, cgobj* self, char should_free);
  VFN_CDECL(get_type_info, void*);
  VFN_STDCALL(add_ref, int, int unused);
  VFN_STDCALL(release, void, int unused);
  VFN_STDCALL(query_interface, int, int iid, void** out);
};

class cgobj {
public:
  cgobj_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cobj_child::fields::field_0c);
  int m_field_0c;
  PAD_TO(ext_client::offsets::cobj_child::fields::field_0c + sizeof(int), ext_client::offsets::cobj_child::fields::list_node);
  void* m_list_node[3];
  PAD_TO(ext_client::offsets::cobj_child::fields::list_node + sizeof(void*) * 3, ext_client::offsets::cgobj::fields::region_end);
};

static_assert(offsetof(cgobj, m_field_0c) == ext_client::offsets::cobj_child::fields::field_0c, "cgobj::m_field_0c offset mismatch");
static_assert(sizeof(cgobj) == ext_client::offsets::cgobj::fields::region_end, "cgobj size mismatch");


