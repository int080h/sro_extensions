#pragma once

#include "plugins/net_log/net_log_internal.hpp"

namespace ext_client::plugins::net_log {

auto process_incoming_cmsg(ext_client::core::event::packet_context& ctx, bool& blocked, bool& modified) -> void;
auto process_outgoing_stream(ext_client::core::event::packet_context& ctx, bool& blocked, bool& modified) -> void;
} // namespace ext_client::plugins::net_log
