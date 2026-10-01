#pragma once

#include <cstdint>

struct cprocess_msg {
  void* vtable;
  std::uint32_t msg_id;
  std::uintptr_t param1;
  std::uintptr_t param2;
};
