#pragma once

#include <windows.h>

namespace ext_client::utils::window_style {

  [[nodiscard]] auto resolve_main_window() noexcept -> HWND;
  auto ensure_minimize_button(HWND hwnd) noexcept -> bool;
  auto ensure_main_window_minimize_button() noexcept -> bool;

} // namespace ext_client::utils::window_style
