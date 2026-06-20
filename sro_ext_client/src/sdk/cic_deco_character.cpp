#include "sdk/cic_deco_character.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

namespace {
  namespace obj_off = ext_client::offsets::cic_deco_character::fields;
  namespace obj_vtbl = ext_client::offsets::cic_deco_character::vtable;
  constexpr std::size_t k_min_readable_size = 0x8C0;
} // namespace

auto cic_deco_character::from_ptr(void* ptr) -> cic_deco_character* {
  if (!ptr) {
    return nullptr;
  }
  return reinterpret_cast<cic_deco_character*>(ptr);
}

auto cic_deco_character::from_ptr(int ptr) -> cic_deco_character* {
  return from_ptr(reinterpret_cast<void*>(ptr));
}

auto cic_deco_character::sight_range() const -> float {
  return ext_client::offsets::field_at<float>(this, obj_off::sight_range);
}

auto cic_deco_character::sight_fade() const -> float {
  return ext_client::offsets::field_at<float>(this, obj_off::sight_fade);
}

auto cic_deco_character::set_sight_range(float range, bool create_character_path) -> void {
  ext_client::offsets::field_at<float>(this, obj_off::sight_range) = range;
  ext_client::offsets::field_at<float>(this, obj_off::sight_fade) = create_character_path ? (100.0f / range) : range;
}

auto cic_deco_character::play_animation(int anim_id, int arg1, int arg2, int arg3, float speed, float start_time) -> bool {
  if (!this) {
    return false;
  }

  void** vtable = *reinterpret_cast<void***>(this);
  if (!vtable) {
    return false;
  }

  auto* play_animation_func = vtable[obj_vtbl::play_animation];
  if (!play_animation_func) {
    return false;
  }

  using play_animation_fn = void(__thiscall*)(cic_deco_character*, int, int, int, int, float, float);
  reinterpret_cast<play_animation_fn>(play_animation_func)(this, anim_id, arg1, arg2, arg3, speed, start_time);
  return true;
}
