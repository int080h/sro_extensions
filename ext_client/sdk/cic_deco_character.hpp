#pragma once

#include "cicharactor.hpp"

// cic_deco_character: Decorative/lobby character (inherits from ci_charactor).
class cic_deco_character : public ci_charactor {
public:
  static auto from_ptr(void* ptr) -> cic_deco_character*;
  static auto from_ptr(int ptr) -> cic_deco_character*;

  auto set_sight_range(float range, bool create_character_path) -> void;
  auto sight_range() const -> float;
  auto sight_fade() const -> float;

  auto play_animation(int anim_id, int arg1, int arg2, int arg3, float speed, float start_time) -> bool;
};

