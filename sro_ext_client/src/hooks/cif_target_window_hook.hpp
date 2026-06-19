#pragma once

namespace ext_client::hooks {

  class cif_target_window_hook {
  public:
    static auto install() -> bool;
    static auto uninstall() -> void;
    static auto is_installed() -> bool;
    static auto render_hp_overlay() -> void;
  };

} // namespace ext_client::hooks
