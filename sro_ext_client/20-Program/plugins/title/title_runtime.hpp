#pragma once

#include "render/menu_builder.hpp"

namespace ext_client::plugins::title {

auto apply_from_control() -> void;
auto handle_tick() -> void;
auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void;

} // namespace ext_client::plugins::title
