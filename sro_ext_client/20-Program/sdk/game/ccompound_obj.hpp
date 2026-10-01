#pragma once

#include <d3d9.h>
#include "utils/vectorf.hpp"

// ccompound_obj: 3D model compound object instance
class ccompound_obj {
public:
  // Get compound object name
  auto name() const -> const char*;

  // Get compound object file path
  auto path() const -> const char*;

  // Get current transparency/alpha value
  auto alpha() const -> float;

  // Get alpha mode
  auto alpha_mode() const -> int;

  // Get direction/forward vector
  auto forward_vector() const -> vector3f;

  // Get pointer to world matrix
  auto world_matrix() const -> const D3DMATRIX*;

  // Set alpha/transparency (virtual slot 2)
  auto set_alpha(float alpha, int force = 0) -> void;

  // Set world matrix (virtual slot 3)
  auto set_world_matrix(const D3DMATRIX* matrix) -> void;
};
