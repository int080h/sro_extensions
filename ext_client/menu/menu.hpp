#pragma once

#include <windows.h>
#include <d3d9.h>

namespace ext_client::menu {
  auto on_create(HWND hwnd, IDirect3DDevice9* device) -> void;
  auto shutdown() -> void;
  auto render() -> void;
  auto is_visible() -> bool;
  auto set_visible(bool visible) -> void;
} // namespace ext_client::menu
