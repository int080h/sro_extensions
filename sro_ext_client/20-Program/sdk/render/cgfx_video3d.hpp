#pragma once

#include "sdk/render/ccamera.hpp"
#include "sdk/render/cd3d_application.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CGfxVideo3d — Global 3D graphics rendering engine singleton
// Native Global: 0x013BAE1C | Extends CD3DApplication
// Manages device initialization, scene begin/end, and swap chain presentation.
// ---------------------------------------------------------------------------
class cgfx_video3d : public cd3d_application {
public:
  static constexpr std::uint32_t k_singleton_addr = 0x013BAE1C;

  // 1. Singleton Access
  static auto get() -> cgfx_video3d*;

  // 2. Lifecycle & Context Management
  auto create(HWND hwnd_param, void* handler, int flags) -> bool;
  auto release() -> bool;
  auto set_size(std::uint32_t width, std::uint32_t height) -> bool;
  auto set_format(int format) -> int;

  // 3. Scene Render Pipeline
  auto begin_scene() -> bool;
  auto end_scene() -> bool;
  auto present(int a2 = 0, int a3 = 0, int a4 = 0, int a5 = 0) -> bool;
  auto render() -> int;
  auto frame_move() -> int;

  // 4. Camera & Matrix Access (Zero-Hook Direct Engine State)
  auto get_camera() -> ccamera*;
  auto get_camera() const -> const ccamera*;
  auto get_view_projection_matrix() const -> const D3DMATRIX*;
  auto get_viewport_size(int& width, int& height) const -> bool;
};
