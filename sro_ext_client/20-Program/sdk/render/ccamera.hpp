#pragma once

#include "utils/vectorf.hpp"

#include <d3d9.h>
#include <cmath>
#include <cstdint>

// ---------------------------------------------------------------------------
// CCamera — Silkroad native camera structure
// Stored inside CGfxVideo3d at offset +0x35C (and CGame at +0x340).
// Total Size: 0x154 bytes (340 bytes).
//
// Reversed layout (from sub_9022A0, sub_D78F40, sub_D79D00):
//   +0x00: std::uint32_t type         (2 = Perspective, 1 = Orthographic)
//   +0x04: vector3f      eye_pos      (Camera world position)
//   +0x10: vector3f      target_pos   (Camera look-at target)
//   +0x1C: vector3f      up_vec       (Camera up vector)
//   +0x28: float         yaw          (Horizontal rotation)
//   +0x2C: float         pitch        (Vertical elevation)
//   +0x30: float         roll         (Roll angle)
//   +0x34: float         distance     (Distance to target / current zoom)
//   +0x38: float         min_distance (Zoom in clamp)
//   +0x3C: float         max_distance (Zoom out clamp)
//   +0x40: std::uint32_t state_flags
//   +0x44: std::uint8_t  active_flag
//   +0x45: std::uint8_t  pad_45[3]
//   +0x48: float         near_z       (Near clipping plane, default 1.0f)
//   +0x4C: float         far_z        (Far clipping plane, default 500000.0f)
//   +0x50: float         fov          (Field of view in degrees, default 60.0f)
//   +0x54: D3DMATRIX     view         (View matrix, 64 bytes)
//   +0x94: D3DMATRIX     proj         (Projection matrix, 64 bytes)
//   +0xD4: D3DMATRIX     inv_view     (Inverse view matrix, 64 bytes)
//   +0x114: D3DMATRIX    inv_proj     (Inverse projection matrix, 64 bytes)
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct ccamera {
  std::uint32_t type;            // +0x00
  vector3f      eye_pos;         // +0x04
  vector3f      target_pos;      // +0x10
  vector3f      up_vec;          // +0x1C
  float         yaw;             // +0x28
  float         pitch;           // +0x2C
  float         roll;            // +0x30
  float         distance;        // +0x34
  float         min_distance;    // +0x38
  float         max_distance;    // +0x3C
  std::uint32_t state_flags;     // +0x40
  std::uint8_t  active_flag;     // +0x44
  std::uint8_t  pad_45[3];       // +0x45
  float         near_z;          // +0x48
  float         far_z;           // +0x4C
  float         fov;             // +0x50
  D3DMATRIX     view;            // +0x54
  D3DMATRIX     proj;            // +0x94
  D3DMATRIX     inv_view;        // +0xD4
  D3DMATRIX     inv_proj;        // +0x114

  // --- Type & Projection Checks ---
  [[nodiscard]] auto is_perspective() const noexcept -> bool {
    return std::fabs(proj._44) < 0.05f && std::fabs(proj._34) > 0.05f;
  }

  [[nodiscard]] auto aspect_ratio() const noexcept -> float {
    if (std::fabs(proj._11) > 0.0001f) {
      return proj._22 / proj._11;
    }
    return 16.0f / 9.0f;
  }

  // --- Directional Vectors (3D World Space) ---
  [[nodiscard]] auto forward_direction() const noexcept -> vector3f {
    vector3f dir = target_pos - eye_pos;
    const float len = dir.length();
    return len > 0.0001f ? (dir / len) : vector3f(0.0f, 0.0f, 1.0f);
  }

  [[nodiscard]] auto right_direction() const noexcept -> vector3f {
    const vector3f fwd = forward_direction();
    vector3f r = fwd.cross(up_vec);
    const float len = r.length();
    return len > 0.0001f ? (r / len) : vector3f(1.0f, 0.0f, 0.0f);
  }

  // --- Camera Control Helpers ---
  auto set_zoom(float new_distance) noexcept -> void {
    distance = new_distance;
  }

  auto set_zoom_limits(float min_dist, float max_dist) noexcept -> void {
    min_distance = min_dist;
    max_distance = max_dist;
  }

  auto set_fov(float fov_degrees) noexcept -> void {
    fov = fov_degrees;
  }
};
#pragma pack(pop)

static_assert(sizeof(ccamera) == 0x154, "ccamera size must be exactly 0x154 bytes");
static_assert(offsetof(ccamera, view) == 0x54, "ccamera::view offset mismatch");
static_assert(offsetof(ccamera, proj) == 0x94, "ccamera::proj offset mismatch");
static_assert(offsetof(ccamera, inv_view) == 0xD4, "ccamera::inv_view offset mismatch");
static_assert(offsetof(ccamera, inv_proj) == 0x114, "ccamera::inv_proj offset mismatch");
