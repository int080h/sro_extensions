#pragma once

#include "utils/msvc9_stl.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CClientConfig — Global client engine configuration
// Retrieved via CGWnd::GetClientConfig() @ 0x00412E20
// Holds selected server, media paths, resolution, audio, and network flags.
// ---------------------------------------------------------------------------
class cclient_config {
public:
  // 1. Server & Connection Settings
  auto get_selected_server() -> ext_client::msvc9::wstring;
};
