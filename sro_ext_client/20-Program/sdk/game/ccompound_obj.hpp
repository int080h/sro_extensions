#pragma once

#include "utils/vectorf.hpp"

#include <d3d9.h>

class crt_skeleton;

// ---------------------------------------------------------------------------
// CCompoundObj — 3D Model Compound Object Instance
// Manages object name, model file path, alpha blending, and D3D world matrices.
// ---------------------------------------------------------------------------
class ccompound_obj {
public:
  // 1. Identity & File Paths
  auto name() const -> const char*;
  auto path() const -> const char*;

  // 2. Alpha & Blending State
  auto alpha() const -> float;
  auto alpha_mode() const -> int;
  auto set_alpha(float alpha, int force = 0) -> void;

  // 3. Transform & World Matrix
  auto forward_vector() const -> vector3f;
  auto world_matrix() const -> const D3DMATRIX*;
  auto set_world_matrix(const D3DMATRIX* matrix) -> void;

  // 4. Skeleton & Bone Hierarchy
  auto skeleton() const -> crt_skeleton*;
  auto get_bone_matrix(const char* bone_name) const -> const D3DMATRIX*;

  // 5. 3D Bounding Box (AABB in local model space)
  auto bounding_box(vector3f& out_min, vector3f& out_max) const -> bool;
};
