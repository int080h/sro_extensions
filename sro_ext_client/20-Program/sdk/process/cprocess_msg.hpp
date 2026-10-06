#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// cprocess_msg — Message packet dispatched to CProcess::ProcessMsg
// Contains the virtual table, message identifier, and two message parameters.
// ---------------------------------------------------------------------------
struct cprocess_msg {
  void* vtable;
  std::uint32_t msg_id;
  std::uintptr_t param1;
  std::uintptr_t param2;
};
