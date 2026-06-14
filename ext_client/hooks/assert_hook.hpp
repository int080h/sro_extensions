#pragma once

namespace ext_client::hooks::assertion {
  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
} // namespace ext_client::hooks::assertion
