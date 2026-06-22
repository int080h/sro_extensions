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
//   recv S->C:  socket -> recv_assemble -> dispatch_handler (0xDA4B30) -> game handlers
//   send C->S:  send_from_buffer (0x941600) -> stream_to_wire -> CMsg -> wire
//   send C->S session: session_send_a (0xDA5B20) / session_send_b (0xDA5C00) -> wire

#include "cclient_net.hpp"
#include "cnet_engine.hpp"
#include "cnet_process.hpp"
#include "cobj.hpp"
