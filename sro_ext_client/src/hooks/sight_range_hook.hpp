#pragma once

namespace ext_client::hooks::sight_range {
  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
  auto apply_settings() -> void;
} // namespace ext_client::hooks::sight_range
