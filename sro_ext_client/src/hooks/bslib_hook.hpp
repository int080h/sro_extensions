#pragma once

namespace ext_client::hooks::bslib {
  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
} // namespace ext_client::hooks::bslib
