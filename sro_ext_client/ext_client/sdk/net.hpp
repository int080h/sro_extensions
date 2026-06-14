#pragma once

// Silkroad client networking stack (reversed from vftables 0x103A544 .. 0x108678C).
//
//   CClientNet (IClientNet)     facade, globals at 0x01420400
//     CNetEngine (IBSNet)       embedded at CClientNet+0x08
//     cclient_session (Src)     heap login state, pointer at 0x014203FC
//
//   CNetProcessThird / Second   gateway & agent opcode dispatch (cobj_child-based)
//   CNetProcessIn               in-game net process (cgobj-based)
//
// Packet path:
//   recv CMsg:   socket -> recv_assemble -> dispatch_handler -> CClientNet queue
//   recv stream: recv_pump -> from_wire -> dispatch_to_process -> CPS handlers
//   send stream: SendMsg -> stream_to_wire -> IBSMsg pool
//   send CMsg:   send_pool_buffer (vtable+0x50) -> socket

#include "cclient_net.hpp"
#include "cnet_engine.hpp"
#include "cnet_process.hpp"
#include "cobj.hpp"
