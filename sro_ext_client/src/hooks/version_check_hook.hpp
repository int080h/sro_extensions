#pragma once

#include "ext_client.hpp"
#include "sdk/cps_version_check.hpp"

#include <cstdint>

namespace ext_client::hooks::version_check {

  using control = ext_client::config::settings::version_check_t;

  auto control_panel() -> control&;
  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
  auto tick() -> void;
  auto apply_from_control() -> void;

} // namespace ext_client::hooks::version_check
