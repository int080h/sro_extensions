#include "pch.hpp"
#include "sdk/game/ccompound_obj.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

auto ccompound_obj::name() const -> const char* {
  using string_ref = ext_client::msvc9::string_ref;
  const auto* str_ptr = &ext_client::off::field_at<char>(this, 0x054);
  return string_ref::from(str_ptr).data();
}

auto ccompound_obj::path() const -> const char* {
  using string_ref = ext_client::msvc9::string_ref;
  const auto* str_ptr = &ext_client::off::field_at<char>(this, 0x0D4);
  return string_ref::from(str_ptr).data();
}

auto ccompound_obj::alpha() const -> float {
  return ext_client::off::field_at<float>(this, 0x07C);
}

auto ccompound_obj::alpha_mode() const -> int {
  return ext_client::off::field_at<int>(this, 0x080);
}

auto ccompound_obj::forward_vector() const -> vector3f {
  const auto* vec = &ext_client::off::field_at<float>(this, 0x0A0);
  return vector3f(vec[0], vec[1], vec[2]);
}

auto ccompound_obj::world_matrix() const -> const D3DMATRIX* {
  return ext_client::off::field_at<const D3DMATRIX*>(this, 0x0AC);
}

auto ccompound_obj::set_alpha(float alpha, int force) -> void {
  using set_alpha_fn = void(__thiscall*)(ccompound_obj* this_ptr, float alpha, int force);
  auto* self = const_cast<ccompound_obj*>(this);
  auto** vtable = *reinterpret_cast<void***>(self);
  const auto set_alpha_func = reinterpret_cast<set_alpha_fn>(vtable[2/*set_alpha*/]);
  set_alpha_func(self, alpha, force);
}

auto ccompound_obj::set_world_matrix(const D3DMATRIX* matrix) -> void {
  using set_world_matrix_fn = void(__thiscall*)(ccompound_obj* this_ptr, const D3DMATRIX* matrix);
  auto* self = const_cast<ccompound_obj*>(this);
  auto** vtable = *reinterpret_cast<void***>(self);
  const auto set_world_matrix_func = reinterpret_cast<set_world_matrix_fn>(vtable[3/*set_world_matrix*/]);
  set_world_matrix_func(self, matrix);
}
