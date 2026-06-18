#pragma once

#include <cstdint>

namespace ext_client::utils {

  inline auto every_n_ticks(std::uint32_t& counter, std::uint32_t interval) -> bool {
    if (interval == 0) {
      return true;
    }
    if (++counter >= interval) {
      counter = 0;
      return true;
    }
    return false;
  }

} // namespace ext_client::utils
