#pragma once

#include "sdk/ui/cif_slot_with_help.hpp"

namespace ext_client::render {

  auto render_item_tooltip(const cso_item* item) -> void;
  auto render_item_tooltip_from_data(const sdk::game::item_tooltip_data& data) -> void;

} // namespace ext_client::render

namespace ext_client::sdk::game {
  // Forwarding alias for compatibility
  using ext_client::render::render_item_tooltip;
  using ext_client::render::render_item_tooltip_from_data;
} // namespace ext_client::sdk::game
