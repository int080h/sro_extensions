#pragma once

#include <Windows.h>
#include <cstring>

namespace ext_client::plugins::assert_bypass {

  auto handle_assert_report(void* raw_ctx) -> void;

} // namespace ext_client::plugins::assert_bypass
