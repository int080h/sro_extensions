#pragma once

namespace ext_client::hooks::d3d {

  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
  auto is_imgui_ready() -> bool;

} // namespace ext_client::hooks::d3d
