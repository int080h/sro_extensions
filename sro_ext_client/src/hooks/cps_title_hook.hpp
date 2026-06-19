#pragma once

#include "ext_client.hpp"
#include "sdk/cif_static.hpp"
#include "sdk/cps_title.hpp"

#include <cstdint>

namespace ext_client::hooks {

  class cps_title_hook {
  public:
    using control = ext_client::config::settings::title_t;

    static auto control_panel() -> control&;
    static auto data_version_label() -> cif_static*;
    static auto exe_version_label() -> cif_static*;
    static auto set_version_label_clip_mode(cif_text_clip_mode mode, int ellipsis_width = k_default_ellipsis_clip_width) -> void;
    static auto apply_version_label_layout() -> void;
    static auto apply_logo_layout() -> void;
    static auto apply_version_labels() -> void;
    static auto captured_version_label_count() -> int;
    static auto on_set_child_process(int process_type, int activate) -> void;
    static auto install() -> bool;
    static auto uninstall() -> void;
    static auto is_installed() -> bool;
    static auto tick() -> void;
    static auto apply_from_control() -> void;
  };

} // namespace ext_client::hooks
