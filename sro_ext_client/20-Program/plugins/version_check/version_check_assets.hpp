#pragma once

#include "plugins/version_check/version_check_internal.hpp"

namespace ext_client::plugins::version_check {

auto ensure_gdiplus_initialized() -> void;
auto read_loading_banner_state(cif_static* banner) -> loading_banner_state;
auto convert_banner_texture_to_bitmap(cif_static* frame) -> Gdiplus::Bitmap*;
} // namespace ext_client::plugins::version_check
