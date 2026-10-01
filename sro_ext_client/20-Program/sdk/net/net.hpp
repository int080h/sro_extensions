#pragma once

// Silkroad client networking — send via ext_client::net::packet_builder::send(), intercept via on_packet hooks.

#include "sdk/net/cmsg.hpp"
#include "sdk/net/packet_builder.hpp"
#include "sdk/net/cmsg_stream_buffer.hpp"
#include "sdk/net/cclient_net.hpp"
#include "sdk/net/cnet_engine.hpp"
#include "sdk/net/msg_define.hpp"
