#pragma once

namespace ext_client::plugins::net_log {

  auto handle_packet(void* raw_ctx) -> void;
  auto handle_menu(void* raw_ctx) -> void;

} // namespace ext_client::plugins::net_log
