#pragma once

#include "utils/vectorf.hpp"

#include <d3d9.h>
#include <cstdint>

class crt_skeleton;

// ---------------------------------------------------------------------------
// CRTBone — Joymax RenderTech Skeletal Joint Node
// Native VTable: 0x01079154
// ---------------------------------------------------------------------------
class crt_bone {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01079154;

  // 1. Identity & Hierarchy
  auto name() const -> const char*;
  auto is_dummy() const -> bool;
  auto parent() const -> crt_bone*;
  auto skeleton() const -> crt_skeleton*;

  // 2. Matrices & Transforms
  // Local transform relative to parent bone
  auto local_matrix() const -> const D3DMATRIX*;
  // Combined transform relative to model root
  auto combined_matrix() const -> const D3DMATRIX*;

  // 3. Positions
  auto local_position() const -> vector3f;
  auto world_position(const D3DMATRIX* compound_world_matrix) const -> vector3f;
};
