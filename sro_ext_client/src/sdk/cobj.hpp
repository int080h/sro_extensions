#pragma once

#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

// SPosition: 3D coordinates in Silkroad's regional space
struct SPosition {
  std::uint16_t region_id;
  float x;
  float z;
  float y;

  auto region_x() const -> std::uint8_t {
    return static_cast<std::uint8_t>(region_id & 0xFF);
  }
  auto region_y() const -> std::uint8_t {
    return static_cast<std::uint8_t>(region_id >> 8);
  }
};

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

// cobj root (virtual, mdisp=0).
class cobj {
public:
  cobj_vtable* vftable;
};

class ccompoundobj;

// cobj_child slice at the start of every outer process object.
class cobj_child : public cobj {
public:
  DECLARE_SDK_VTABLE(cobj_child_vtable, child_vftable)
  union {
    DEFINE_MEMBER_0(ccompoundobj* m_compound_obj, "compound_obj");
    DEFINE_MEMBER_N(int m_field_0c, 0x08);
    DEFINE_MEMBER_N(void* m_list_prev, 0x10);
    DEFINE_MEMBER_N(void* m_list_next, 0x14);
    DEFINE_MEMBER_N(int m_list_end, 0x18);
  };
};

// ======================================================================
// Real game base RTTI classes for multiple inheritance layout
// ======================================================================

class ci_entity : public cobj_child {
public:
  union {
    DEFINE_MEMBER_0(std::uint8_t m_boundary_sub[16], "boundary");
    DEFINE_MEMBER_N(std::uint32_t m_unknown_30, 0x10);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_38, 0x18);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3c, 0x1C);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_40, 0x20);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_44, 0x24);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_48, 0x28);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_4c, 0x2C);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_50, 0x30);
  };
};

class c_animation_callback {
public:
  virtual ~c_animation_callback() = default;
};

// ci_object inherits from ci_entity and c_animation_callback at offset 84.
class ci_object : public ci_entity, public c_animation_callback {
public:
  union {
    DEFINE_MEMBER_N(float m_position_offset[3], 0x08);
    DEFINE_MEMBER_N(float m_alpha_fade, 0x18);
    DEFINE_MEMBER_N(float m_unknown_74, 0x1C);
    DEFINE_MEMBER_N(float m_unknown_78, 0x20);
    DEFINE_MEMBER_N(SPosition m_coords, 0x24);
    DEFINE_MEMBER_N(float m_unknown_9c, 0x44);
    DEFINE_MEMBER_N(float m_bound_min_x, 0x48);
    DEFINE_MEMBER_N(float m_bound_min_y, 0x4C);
    DEFINE_MEMBER_N(float m_bound_min_z, 0x50);
    DEFINE_MEMBER_N(float m_bound_max_x, 0x54);
    DEFINE_MEMBER_N(float m_bound_max_y, 0x58);
    DEFINE_MEMBER_N(float m_bound_max_z, 0x5C);
    DEFINE_MEMBER_N(float m_unknown_b8, 0x60);
    DEFINE_MEMBER_N(std::uint8_t m_rendering_mode, 0x64);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_bd, 0x65);
    DEFINE_MEMBER_N(float m_scale_x, 0x68);
    DEFINE_MEMBER_N(float m_scale_y, 0x6C);
    DEFINE_MEMBER_N(float m_scale_z, 0x70);
    DEFINE_MEMBER_N(float m_unknown_cc, 0x74);
    DEFINE_MEMBER_N(float m_unknown_d0, 0x78);
    DEFINE_MEMBER_N(float m_unknown_d4, 0x7C);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_d8, 0x80);
  };
};

class ci_gid_object : public ci_object {
public:
  union {
    DEFINE_MEMBER_0(std::uint8_t m_gid_sub[72], "gid_sub");
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_res_name, 0x38);
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_tag_name, 0x54);
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_status_name, 0x70);
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_display_name, 0x88);
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_gid_tag, 0xF8);
    DEFINE_MEMBER_N(size_t m_tag_list_size, 0x17C);
    DEFINE_MEMBER_N(size_t m_tag_list_cap, 0x180);
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_unknown_wstr, 0x184);
    DEFINE_MEMBER_N(float m_view_distance, 0x228);
    DEFINE_MEMBER_N(float m_fade_timer, 0x274);
    DEFINE_MEMBER_N(float m_fade_speed, 0x278);
  };
public:
  ci_gid_object() {}
  ~ci_gid_object() override {}
};

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
  union {
    DEFINE_MEMBER_N(int m_field_0c, 0x08);
    DEFINE_MEMBER_N(std::n_list<void*> m_list, 0x10);
  };
};


