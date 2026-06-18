#pragma once

#include <windows.h>
#include <d3d9.h>

namespace ext_client::menu {
  auto on_create(HWND hwnd, IDirect3DDevice9* device) -> void;
  auto shutdown() -> void;
  auto render() -> void;
  auto render_external(void (*draw_fn)()) -> void;
  auto is_initialized() -> bool;
  auto is_visible() -> bool;
  auto set_visible(bool visible) -> void;
  auto wants_capture_mouse() -> bool;
  auto wants_capture_keyboard() -> bool;
} // namespace ext_client::menu
