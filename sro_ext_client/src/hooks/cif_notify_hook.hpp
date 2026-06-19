#pragma once

namespace ext_client::hooks {
  class cif_notify_hook {
  public:
    static auto install() -> bool;
    static auto uninstall() -> void;
    static auto is_installed() -> bool;
  };
} // namespace ext_client::hooks
