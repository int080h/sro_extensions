#pragma once

#include <Windows.h>
#include <cstddef>

namespace ext_client::plugins::quest {

  auto handle_get_quest_definition(void* raw_ctx) -> void;

} // namespace ext_client::plugins::quest
