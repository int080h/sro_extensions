#pragma once

namespace ext_client::core::hooks::core_hooks {

  auto install_all() -> bool;
  auto uninstall_all() -> bool;
  auto is_render_installed() -> bool;
  auto is_installed() -> bool;

  auto install_lazy() -> void;

  auto tick() -> void;

} // namespace ext_client::core::hooks::core_hooks
