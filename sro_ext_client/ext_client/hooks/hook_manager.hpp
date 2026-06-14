#pragma once

namespace ext_client::hooks::manager {

  auto install_all() -> bool;
  auto uninstall_all() -> void;
  auto tick() -> void;

} // namespace ext_client::hooks::manager
