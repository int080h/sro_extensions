#pragma once

#include "sdk/game/cic_user.hpp"

// ---------------------------------------------------------------------------
// CICScriptObj — Scripted entity animation wrapper in game cutscenes
// Extends CICUser
// ---------------------------------------------------------------------------
class cic_script_obj : public cic_user {
public:
  // 1. Instance Resolution
  static auto from_ptr(void* ptr) -> cic_script_obj*;

  // 2. Animation Methods
  auto play_animation(int anim_id, int arg1, int arg2, int arg3, float speed, float start_time) -> void;
};
