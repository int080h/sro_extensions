#include "sdk/cic_script_obj.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

auto cic_script_obj::from_ptr(void* ptr) -> cic_script_obj* {
  if (!ptr) {
    return nullptr;
  }
  return reinterpret_cast<cic_script_obj*>(ptr);
}

auto cic_script_obj::play_animation(int anim_id, int arg1, int arg2, int arg3, float speed, float start_time) -> void {
  if (!this) {
    return;
  }
  void** vtable = *reinterpret_cast<void***>(this);
  if (!vtable) {
    return;
  }
  using play_animation_fn = void(__thiscall*)(cic_script_obj* this_ptr, int anim_id, int arg1, int arg2, int arg3, float speed, float start_time);
  const auto play_anim = reinterpret_cast<play_animation_fn>(vtable[ext_client::offsets::cic_script_obj::vtable::play_animation]);
  if (!play_anim) {
    return;
  }
  play_anim(this, anim_id, arg1, arg2, arg3, speed, start_time);
}
