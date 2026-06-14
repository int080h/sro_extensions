#pragma once

#include <Windows.h>

namespace ext_client::hooks::loading_splash_overlay {

  struct config {
    int x = 0;
    int y = 0;
    int width = 400;
    int height = 148;
    int interval_ms = 250;
    int frame_count = 10;
    bool log_events = false;
    void** frames = nullptr; // Array of Gdiplus::Bitmap*
  };

  auto start(HWND owner, const config& cfg) -> bool;
  auto stop() -> void;
  auto is_running() -> bool;

} // namespace ext_client::hooks::loading_splash_overlay
