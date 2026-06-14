#pragma once

#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class sworld;

// SWorld — in-game world / terrain / graphics state (g_sw @ ext_client::offsets::sworld::globals::address).
// Reversed from World.cpp (VS2008 client, IDA).
//
// Per-field offsets: ext_client::offsets::sworld::fields in offsets.hpp

struct sworld_vtable {
  VFN_THISCALL(func_00, char, sworld* self, int, int);
  VFN_THISCALL(func_01, char, sworld* self, int);
  VFN_THISCALL(func_02, void*, sworld* self, void*);
  VFN_THISCALL(func_03, char, sworld* self, int);
  VFN_THISCALL(func_04, int, sworld* self, float);
  VFN_THISCALL(func_05, void*, sworld* self, char);
  VFN_THISCALL(func_06, int, sworld* self, int, int);
  VFN_THISCALL(func_07, char, sworld* self, std::uint16_t, std::uint16_t, float*);
  VFN_THISCALL(func_08, int, sworld* self, void*);
  VFN_THISCALL(func_09, int, sworld* self, int, int);
  VFN_THISCALL(func_10, int, sworld* self, int, int);
  VFN_THISCALL(func_11, int, sworld* self, int);
  VFN_THISCALL(func_12, int, sworld* self, int);
  VFN_THISCALL(func_13, void, sworld* self, int, int);
  VFN_THISCALL(func_14, int*, sworld* self, int, int);
  VFN_THISCALL(func_15, int, sworld* self, int, int);
  VFN_THISCALL(func_16, int, sworld* self);
  VFN_THISCALL(func_17, int, sworld* self, int);
  VFN_THISCALL(func_18, int, sworld* self);
  VFN_THISCALL(func_19, double, sworld* self);
  VFN_THISCALL(func_20, int, sworld* self, float, int);
  VFN_THISCALL(func_21, std::int16_t, sworld* self, std::int16_t);
  VFN_THISCALL(func_22, int, sworld* self, int, int);
  VFN_THISCALL(func_23, void, sworld* self, int, int, void*, int, int);
  VFN_THISCALL(func_24, int, sworld* self, float, int, int, float, float, int, int, int);
  VFN_THISCALL(func_25, int, sworld* self, float, float, int, float, float, int, int, int, int);
  VFN_THISCALL(func_26, int, sworld* self, int, int);
  VFN_THISCALL(func_27, void, sworld* self, int, int, int, char, int);
  VFN_THISCALL(func_28, int, sworld* self, int, int, int, float, float, float, int, int);
  VFN_THISCALL(func_29, int, sworld* self, int);
  VFN_THISCALL(func_30, int, sworld* self, int);
  VFN_THISCALL(func_31, int, sworld* self, int);
  VFN_THISCALL(func_32, bool, sworld* self, float*, float*, float*, int);
  VFN_THISCALL(func_33, double, sworld* self, unsigned int, float);
  VFN_THISCALL(func_34, double, sworld* self);
  VFN_THISCALL(func_35, char*, sworld* self);
  VFN_THISCALL(func_36, int, sworld* self, int, float, float, float, float, float, float, int, int);
  VFN_THISCALL(func_37, int, sworld* self);
  VFN_THISCALL(func_38, int, sworld* self);
  VFN_THISCALL(func_39, int, sworld* self);
  VFN_THISCALL(func_40, int, sworld* self);
  VFN_THISCALL(func_41, int, sworld* self);
  using func_42_t = int(__cdecl*)(std::uint32_t);
  VFN_THISCALL(func_42, func_42_t, sworld* self, func_42_t);
  VFN_THISCALL(func_43, double, sworld* self, float, float*);
  VFN_THISCALL(func_44, int, sworld* self);
  VFN_THISCALL(set_graphics_option, int, sworld* self, unsigned int setting_id, int value);
  VFN_THISCALL(func_46, int, sworld* self, int);
  VFN_THISCALL(func_47, int, sworld* self, int, int, int, float, int);
  VFN_THISCALL(func_48, int, sworld* self, int, int);
  VFN_THISCALL(func_49, char, sworld* self, char);
  VFN_THISCALL(func_50, void, sworld* self, int, int, int, int);
};

class sworld {
public:
  DECLARE_SDK_VTABLE(sworld_vtable, world_vftable)

  static auto instance() -> sworld*;
  static auto is_instance() -> bool;

  auto cell_limit() const -> std::int32_t;
  auto sight_range() const -> float;
  auto option(unsigned int index) const -> std::int32_t;

  auto set_cell_limit(std::int32_t limit) -> void;
  auto set_sight_range(float range) -> void;
  auto set_option(unsigned int index, std::int32_t value) -> void;
  auto set_graphics_option(unsigned int setting_id, int value) -> int;

private:
  sworld_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::sworld::size);
};

static_assert(sizeof(sworld) == ext_client::offsets::sworld::size, "sworld size mismatch");
