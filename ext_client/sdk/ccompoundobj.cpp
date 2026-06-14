#include "sdk/ccompoundobj.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

auto ccompoundobj::name() const -> const char* {
  using string_ref = ext_client::msvc9::string_ref;
  const auto* str_ptr = &ext_client::offsets::field_at<char>(this, ext_client::offsets::ccompoundobj::fields::name);
  return string_ref::from(str_ptr).data();
}

auto ccompoundobj::path() const -> const char* {
  using string_ref = ext_client::msvc9::string_ref;
  const auto* str_ptr = &ext_client::offsets::field_at<char>(this, ext_client::offsets::ccompoundobj::fields::path);
  return string_ref::from(str_ptr).data();
}

auto ccompoundobj::alpha() const -> float {
  return ext_client::offsets::field_at<float>(this, ext_client::offsets::ccompoundobj::fields::alpha);
}

auto ccompoundobj::alpha_mode() const -> int {
  return ext_client::offsets::field_at<int>(this, ext_client::offsets::ccompoundobj::fields::alpha_mode);
}

auto ccompoundobj::forward_vector() const -> D3DVECTOR {
  const auto* vec = &ext_client::offsets::field_at<float>(this, ext_client::offsets::ccompoundobj::fields::forward_vector);
  return D3DVECTOR{ vec[0], vec[1], vec[2] };
}

auto ccompoundobj::world_matrix() const -> const D3DMATRIX* {
  return ext_client::offsets::field_at<const D3DMATRIX*>(this, ext_client::offsets::ccompoundobj::fields::world_matrix);
}

auto ccompoundobj::set_alpha(float alpha, int force) -> void {
  using set_alpha_fn = void(__thiscall*)(ccompoundobj* this_ptr, float alpha, int force);
  auto* self = const_cast<ccompoundobj*>(this);
  auto** vtable = *reinterpret_cast<void***>(self);
  const auto set_alpha_func = reinterpret_cast<set_alpha_fn>(vtable[ext_client::offsets::ccompoundobj::vtable::set_alpha]);
  set_alpha_func(self, alpha, force);
}

auto ccompoundobj::set_world_matrix(const D3DMATRIX* matrix) -> void {
  using set_world_matrix_fn = void(__thiscall*)(ccompoundobj* this_ptr, const D3DMATRIX* matrix);
  auto* self = const_cast<ccompoundobj*>(this);
  auto** vtable = *reinterpret_cast<void***>(self);
  const auto set_world_matrix_func = reinterpret_cast<set_world_matrix_fn>(vtable[ext_client::offsets::ccompoundobj::vtable::set_world_matrix]);
  set_world_matrix_func(self, matrix);
}
