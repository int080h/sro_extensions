#include "game_state.hpp"
#include "cg_interface.hpp"

namespace ext_client::sdk {

  auto is_game_ready() -> bool {
    return cg_interface::is_ready();
  }

} // namespace ext_client::sdk
