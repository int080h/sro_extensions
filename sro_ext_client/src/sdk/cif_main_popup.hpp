#pragma once

#include "cif_wnd.hpp"
#include <cstddef>
#include <cstdint>

inline constexpr std::size_t calram_entry_count = 13;

struct alram_entry {
  auto active() const -> bool;
  auto type() const -> int;
  auto is_facebook() const -> bool;
  auto is_magic_lamp() const -> bool;
  auto is_daily_login() const -> bool;
  auto is_hidden() const -> bool;
  auto set_active(bool active) -> void;
  auto set_type(int type) -> void;
  auto ref_data_ptr() -> void*;

  std::uint8_t pad_00[0x28];
  std::uint8_t m_active;
  std::uint8_t pad_29[0x98 - 0x29];
  int m_type;
  std::uint8_t pad_9C[0x1F8 - 0x9C];
};

struct alram_data {
  auto entry(std::size_t index) -> alram_entry*;
  auto entry(std::size_t index) const -> const alram_entry*;
  static auto entry_at_raw(alram_data* data, std::size_t index) -> alram_entry*;
  static auto entry_at_raw(const alram_data* data, std::size_t index) -> const alram_entry*;

  std::uint8_t pad_00[0x2450];
  alram_entry m_entries[13];
};

class cif_main_popup;

struct cif_main_popup_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cif_main_popup* self, float delta);
  CGWND_VTABLE_COMMON(cif_main_popup, on_timer_id)
  VFN_THISCALL(on_focus, int, cif_main_popup* self, int focus_param);
  VFN_THISCALL(on_blur, int, cif_main_popup* self, int blur_param);
  VFN_THISCALL(on_enable, int, cif_main_popup* self, int enable_param);
  VFN_THISCALL(on_disable, int, cif_main_popup* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_THISCALL(null_37, int, cif_main_popup* self);
  VFN_THISCALL(null_38, int, cif_main_popup* self);
  VFN_THISCALL(null_39, int, cif_main_popup* self);
  VFN_THISCALL(null_40, int, cif_main_popup* self);
  VFN_THISCALL(null_41, int, cif_main_popup* self);
  VFN_THISCALL(null_42, int, cif_main_popup* self);
  VFN_THISCALL(null_43, int, cif_main_popup* self);
  VFN_THISCALL(null_44, int, cif_main_popup* self);
  VFN_THISCALL(null_45, int, cif_main_popup* self);
  VFN_THISCALL(null_46, int, cif_main_popup* self);
  VFN_THISCALL(null_47, int, cif_main_popup* self);
  VFN_THISCALL(null_48, int, cif_main_popup* self);
  VFN_THISCALL(null_49, int, cif_main_popup* self);
};

class cif_main_popup : public cif_wnd {
public:
  union {
    DEFINE_MEMBER_N(alram_data* m_alram, 0x7C8 - 0x394);
  };

public:
  DECLARE_SDK_VTABLE(cif_main_popup_vtable, main_popup_vftable)
  DECLARE_SDK_WND_HELPERS(cif_main_popup, "CIFMainPopup")

  auto get_alram() -> alram_data*;
  auto get_alram() const -> const alram_data*;

  static auto from_interface(void* cg_interface) -> cif_main_popup*;
};
