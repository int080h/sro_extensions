#include "pch.hpp"
#include "sdk/render/cgfx_video3d.hpp"

#include "utils/memory.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::off::as_fn;

  using create_fn = bool(__thiscall*)(cgfx_video3d* self, HWND hwnd, void* handler, int flags);
  using release_fn = char(__cdecl*)();
  using set_size_fn = bool(__thiscall*)(cgfx_video3d* self, std::uint32_t width, std::uint32_t height);
  using begin_scene_fn = char*(__thiscall*)(cgfx_video3d* self);
  using end_scene_fn = bool(__thiscall*)(cgfx_video3d* self);
  using present_fn = bool(__thiscall*)(cgfx_video3d* self, int a2, int a3, int a4, int a5);
  using set_format_fn = int(__thiscall*)(cgfx_video3d* self, int format);
  using render_fn = int(__thiscall*)(cgfx_video3d* self);
  using frame_move_fn = int(__thiscall*)(cgfx_video3d* self);
} // namespace

// ===========================================================================
// 1. Singleton Access
// ===========================================================================
auto cgfx_video3d::get() -> cgfx_video3d* {
  auto* ptr = ext_client::off::global_at<cgfx_video3d*>(k_singleton_addr);
  return ext_client::utils::memory::is_game_ptr(ptr) ? ptr : nullptr;
}


// ===========================================================================
// 2. Lifecycle & Context Management
// ===========================================================================
auto cgfx_video3d::create(HWND hwnd_param, void* handler, int flags) -> bool {
  const auto fn = as_fn<create_fn>(0x00D79210);
  return fn && fn(this, hwnd_param, handler, flags);
}

auto cgfx_video3d::release() -> bool {
  const auto fn = as_fn<release_fn>(0x00D783E0);
  return fn && fn() != 0;
}

auto cgfx_video3d::set_size(std::uint32_t width, std::uint32_t height) -> bool {
  const auto fn = as_fn<set_size_fn>(0x00D783F0);
  return fn && fn(this, width, height);
}

auto cgfx_video3d::set_format(int format) -> int {
  const auto fn = as_fn<set_format_fn>(0x00D78610);
  if (!fn) {
    return 0;
  }
  return fn(this, format);
}

// ===========================================================================
// 3. Scene Render Pipeline
// ===========================================================================
auto cgfx_video3d::begin_scene() -> bool {
  const auto fn = as_fn<begin_scene_fn>(0x00D785A0);
  if (!fn) {
    return false;
  }
  fn(this);
  return true;
}

auto cgfx_video3d::end_scene() -> bool {
  const auto fn = as_fn<end_scene_fn>(0x00D785B0);
  return fn && fn(this);
}

auto cgfx_video3d::present(int a2, int a3, int a4, int a5) -> bool {
  const auto fn = as_fn<present_fn>(0x00D785D0);
  return fn && fn(this, a2, a3, a4, a5);
}

auto cgfx_video3d::render() -> int {
  const auto fn = as_fn<render_fn>(0x00D78F50);
  if (!fn) {
    return 0;
  }
  return fn(this);
}

auto cgfx_video3d::frame_move() -> int {
  const auto fn = as_fn<frame_move_fn>(0x00D78ED0);
  if (!fn) {
    return 0;
  }
  return fn(this);
}

// ===========================================================================
// 4. Camera & Matrix Access (Zero-Hook Direct Engine State)
// ===========================================================================
auto cgfx_video3d::get_camera() -> ccamera* {
  if (!this || !ext_client::utils::memory::is_game_ptr(this)) {
    return nullptr;
  }
  auto* cam = reinterpret_cast<ccamera*>(reinterpret_cast<std::uint8_t*>(this) + 0x35C);
  return ext_client::utils::memory::is_game_ptr(cam) ? cam : nullptr;
}

auto cgfx_video3d::get_camera() const -> const ccamera* {
  if (!this || !ext_client::utils::memory::is_game_ptr(this)) {
    return nullptr;
  }
  const auto* cam = reinterpret_cast<const ccamera*>(reinterpret_cast<const std::uint8_t*>(this) + 0x35C);
  return ext_client::utils::memory::is_game_ptr(cam) ? cam : nullptr;
}

auto cgfx_video3d::get_view_projection_matrix() const -> const D3DMATRIX* {
  if (!this || !ext_client::utils::memory::is_game_ptr(this)) {
    return nullptr;
  }
  const auto* vp = reinterpret_cast<const D3DMATRIX*>(reinterpret_cast<const std::uint8_t*>(this) + 0x550);
  return ext_client::utils::memory::is_game_ptr(vp) ? vp : nullptr;
}

auto cgfx_video3d::get_viewport_size(int& width, int& height) const -> bool {
  if (!this || !ext_client::utils::memory::is_game_ptr(this)) {
    return false;
  }
  const auto* base = reinterpret_cast<const std::uint8_t*>(this);
  const int w = *reinterpret_cast<const int*>(base + 0x350);
  const int h = *reinterpret_cast<const int*>(base + 0x354);
  if (w > 0 && h > 0) {
    width = w;
    height = h;
    return true;
  }

  // Fallback to D3D device
  auto* dev = const_cast<cgfx_video3d*>(this)->get_device();
  if (dev && ext_client::utils::memory::is_game_ptr(dev)) {
    D3DVIEWPORT9 vp{};
    if (SUCCEEDED(dev->GetViewport(&vp)) && vp.Width > 0 && vp.Height > 0) {
      width = static_cast<int>(vp.Width);
      height = static_cast<int>(vp.Height);
      return true;
    }
  }
  return false;
}
