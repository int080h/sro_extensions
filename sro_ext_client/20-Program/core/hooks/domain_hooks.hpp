#pragma once
namespace ext_client::core::hooks {
  namespace client_hooks {
    auto install() -> bool;
    auto uninstall() -> bool;
  } // namespace client_hooks
  namespace network_hooks {
    auto install() -> bool;
    auto uninstall() -> bool;
  } // namespace network_hooks
  namespace shutdown_hooks {
    auto install() -> bool;
    auto uninstall() -> bool;
  } // namespace shutdown_hooks
  namespace d3d_hooks {
    auto install_lazy() -> void;
    auto uninstall() -> bool;
    auto is_installed() -> bool;
  } // namespace d3d_hooks
} // namespace ext_client::core::hooks
