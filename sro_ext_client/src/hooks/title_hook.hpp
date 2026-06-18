#pragma once

#include "ext_client.hpp"
#include "sdk/cif_static.hpp"
#include "sdk/cps_title.hpp"

#include <cstdint>

namespace ext_client::hooks::title {

  using control = ext_client::config::settings::title_t;

  auto control_panel() -> control&;
  auto data_version_label() -> cif_static*;
  auto exe_version_label() -> cif_static*;
  auto set_version_label_clip_mode(cif_text_clip_mode mode, int ellipsis_width = k_default_ellipsis_clip_width) -> void;
  auto apply_version_label_layout() -> void;
  auto apply_logo_layout() -> void;
  auto apply_version_labels() -> void;
  auto captured_version_label_count() -> int;
  auto on_set_child_process(int process_type, int activate) -> void;
  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
  auto tick() -> void;
  auto apply_from_control() -> void;

} // namespace ext_client::hooks::title
