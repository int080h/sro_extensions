#pragma once

namespace ext_client::utils::shutdown_guard {

  auto arm(const char* reason) -> void;
  auto disarm() -> void;
  auto poll() -> void;
  auto is_armed() -> bool;

} // namespace ext_client::utils::shutdown_guard
