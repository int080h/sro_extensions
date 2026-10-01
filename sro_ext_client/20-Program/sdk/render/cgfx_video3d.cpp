#include "pch.hpp"
#include "sdk/render/cgfx_video3d.hpp"

#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;

  using create_things_fn = bool(__thiscall*)(cgfx_video3d* self, HWND hwnd, void* handler, int flags);
  using destroy_things_fn = char(__cdecl*)();
  using set_size_fn = bool(__thiscall*)(cgfx_video3d* self, std::uint32_t width, std::uint32_t height);
  using begin_scene_fn = char*(__thiscall*)(cgfx_video3d* self);
  using end_scene_fn = bool(__thiscall*)(cgfx_video3d* self);
  using present_fn = bool(__thiscall*)(cgfx_video3d* self, int a2, int a3, int a4, int a5);
  using set_format_fn = int(__thiscall*)(cgfx_video3d* self, int format);
  using render_fn = int(__thiscall*)(cgfx_video3d* self);
  using frame_move_fn = int(__thiscall*)(cgfx_video3d* self);

} // namespace

auto cgfx_video3d::get() -> cgfx_video3d* {
  return ext_client::off::global_at<cgfx_video3d*>(0x013BAE1C);
}

auto cgfx_video3d::create_things(HWND hwnd_param, void* handler, int flags) -> bool {
  const auto fn = as_fn<create_things_fn>(0x00D79210);
  return fn && fn(this, hwnd_param, handler, flags);
}

auto cgfx_video3d::destroy_things() -> bool {
  const auto fn = as_fn<destroy_things_fn>(0x00D783E0);
  return fn && fn() != 0;
}

auto cgfx_video3d::set_size(std::uint32_t width, std::uint32_t height) -> bool {
  const auto fn = as_fn<set_size_fn>(0x00D783F0);
  return fn && fn(this, width, height);
}

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

auto cgfx_video3d::set_format(int format) -> int {
  const auto fn = as_fn<set_format_fn>(0x00D78610);
  return fn ? fn(this, format) : 0;
}

auto cgfx_video3d::render() -> int {
  const auto fn = as_fn<render_fn>(0x00D78F00);
  return fn ? fn(this) : 0;
}

auto cgfx_video3d::frame_move() -> int {
  const auto fn = as_fn<frame_move_fn>(0x00D78EF0);
  return fn ? fn(this) : 0;
}
