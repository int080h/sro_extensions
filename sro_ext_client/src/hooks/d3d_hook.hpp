#pragma once

#include <Windows.h>

namespace ext_client::hooks::d3d {

  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
  auto is_imgui_ready() -> bool;
  auto apply_from_control() -> void;
  auto game_hwnd() -> HWND;
  auto client_mouse_pos(int& x, int& y) -> bool;

} // namespace ext_client::hooks::d3d
