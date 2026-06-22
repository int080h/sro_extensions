#pragma once

namespace ext_client::plugins::welcome_msg {

  auto handle_show_notice(void* raw_ctx) -> void;
  auto handle_menu(void* raw_ctx) -> void;

} // namespace ext_client::plugins::welcome_msg
