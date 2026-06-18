#pragma once

#include "cicplayer.hpp"

// cic_script_obj: Script-object animation wrapper. Character-select previews use cic_deco_character instead.
class cic_script_obj : public cic_user {
public:
  static auto from_ptr(void* ptr) -> cic_script_obj*;

  // Play animation using virtual method 54 (offset 0xD8)
  auto play_animation(int anim_id, int arg1, int arg2, int arg3, float speed, float start_time) -> void;
};
