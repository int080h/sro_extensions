#include "cgfx_video3d.hpp"

auto cgfx_video3d::get() -> cgfx_video3d* {
  return ext_client::offsets::global_at<cgfx_video3d*>(ext_client::offsets::globals::d3d_app);
}

auto cgfx_video3d::create_things(HWND hwnd_param, void* handler, int flags) -> bool {
  return gfx_vftable()->create_things(this, hwnd_param, handler, flags);
}

auto cgfx_video3d::destroy_things() -> bool {
  return gfx_vftable()->destroy_things() != 0;
}

auto cgfx_video3d::set_size(std::uint32_t width, std::uint32_t height) -> bool {
  return gfx_vftable()->set_size(this, width, height);
}

auto cgfx_video3d::begin_scene() -> bool {
  gfx_vftable()->begin_scene(this);
  return true;
}

auto cgfx_video3d::end_scene() -> bool {
  return gfx_vftable()->end_scene(this);
}

auto cgfx_video3d::present(int a2, int a3, int a4, int a5) -> bool {
  return gfx_vftable()->present(this, a2, a3, a4, a5);
}

auto cgfx_video3d::set_format(int format) -> int {
  return gfx_vftable()->set_format(this, format);
}

auto cgfx_video3d::render() -> int {
  return gfx_vftable()->render(this);
}

auto cgfx_video3d::frame_move() -> int {
  return gfx_vftable()->frame_move(this);
}
