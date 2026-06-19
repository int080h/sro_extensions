#pragma once

#include "ext_client.hpp"
#include "sdk/cps_version_check.hpp"

#include <cstdint>

namespace ext_client::hooks {

  class cps_version_check_hook {
  public:
    using control = ext_client::config::settings::version_check_t;

    static auto control_panel() -> control&;
    static auto install() -> bool;
    static auto uninstall() -> void;
    static auto is_installed() -> bool;
    static auto tick() -> void;
    static auto apply_from_control() -> void;
  };

} // namespace ext_client::hooks
