#include "utils/window_style.hpp"

namespace ext_client::utils::window_style {

  namespace {
    constexpr LONG k_minimize_style = WS_SYSMENU | WS_MINIMIZEBOX;
  }

  auto resolve_main_window() noexcept -> HWND {
    return FindWindowA(nullptr, "SRO_CLIENT");
  }

  auto ensure_minimize_button(HWND hwnd) noexcept -> bool {
    if (!hwnd || !IsWindow(hwnd)) {
      return false;
    }

    const LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    const LONG wanted_style = style | k_minimize_style;
    if (style == wanted_style) {
      return true;
    }

    if (!SetWindowLongA(hwnd, GWL_STYLE, wanted_style)) {
      return false;
    }

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    return true;
  }

  auto ensure_main_window_minimize_button() noexcept -> bool {
    return ensure_minimize_button(resolve_main_window());
  }

} // namespace ext_client::utils::window_style
