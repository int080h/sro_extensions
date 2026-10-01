#pragma once

#include <cstdint>

class cgwnd;

struct ingame_res_lookup {
  int res_key = 0;
  void* raw = nullptr;
  cgwnd* wnd = nullptr;
  bool map_readable = false;
  bool found = false;
  bool live = false;
  std::uint32_t vftable = 0;
};
