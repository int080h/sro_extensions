#pragma once

namespace ext_client::hooks::manager {

  auto install_all() -> bool;
  auto uninstall_all() -> void;
  auto tick() -> void;
  auto try_install_lazy() -> void;

} // namespace ext_client::hooks::manager
