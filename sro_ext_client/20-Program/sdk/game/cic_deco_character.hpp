#pragma once

#include "sdk/game/ci_charactor.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CICDecoCharacter — Decorative 3D preview character entity in lobby/selection
// Size: 0x8C0 (2240 bytes) | Extends CICharactor
// Manages 3D character preview meshes, sight range, and skeletal animations.
// ---------------------------------------------------------------------------
class cic_deco_character : public ci_charactor {
public:
  static constexpr std::size_t k_class_size = 0x8C0;

  // 1. Pointer Conversion & Validation
  static auto from_ptr(void* ptr) -> cic_deco_character*;
  static auto from_ptr(int ptr) -> cic_deco_character*;

  // 2. Visibility & Sight Range
  auto sight_range() const -> float;
  auto sight_fade() const -> float;
  auto set_sight_range(float range, bool create_character_path) -> void;

  // 3. 3D Skeletal Animation
  auto play_animation(int anim_id, int arg1, int arg2, int arg3, float speed, float start_time) -> bool;
};
