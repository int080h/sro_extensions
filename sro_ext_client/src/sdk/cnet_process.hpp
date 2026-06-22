#pragma once

#include "cobj.hpp"
#include "cmsg_stream_buffer.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class cnet_process_third;
class cnet_process_second;
class cnet_process_in;

// Shared 7-slot vtable for gateway/agent net process objects (cobj_child-based).
struct cnet_process_obj_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
  VFN_CDECL(get_type_info, void*);
  VFN_STDCALL(add_ref, int, int unused);
  VFN_STDCALL(release, void, int unused);
  VFN_THISCALL(dispatch_msg, int, void* self, cmsg_stream_buffer* packet);
};

// Handler map + opcode registry (+0x24 .. +0x47) used by gateway/agent phases.
struct cnet_process_phase_handlers {
  union {
    DEFINE_MEMBER_N(cmsg_stream_buffer* m_scratch_msg, ext_client::offsets::cnet_process::fields::handler_map_size);
    DEFINE_MEMBER_N(std::uint8_t m_storage[ext_client::offsets::cnet_process::fields::handler_map_size], 0x00);
  };
};

// Common base structure for connection phase net processes.
struct cnet_process_phase {
public:
  cnet_process_obj_vtable* vftable;
  union {
    DEFINE_MEMBER_N(cnet_process_phase_handlers handlers, ext_client::offsets::cnet_process::fields::handler_map - sizeof(void*));
  };
};

// CNetProcessThird — agent-server phase opcode dispatch.
class cnet_process_third : public cnet_process_phase {};

// CNetProcessSecond — gateway/download phase opcode dispatch (same layout as Third).
class cnet_process_second : public cnet_process_phase {};

// CNetProcessIn — in-game net process (cgobj-based).
// Slot 6 is query_interface, not dispatch_msg (unlike Second/Third).
struct cnet_process_in_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, cnet_process_in* self, char should_free);
  VFN_CDECL(get_type_info, void*);
  VFN_STDCALL(add_ref, int, int unused);
  VFN_STDCALL(release, void, int unused);
  VFN_STDCALL(query_interface, int, int iid, void** out);
};

using packet_handler_fn = void (__thiscall cnet_process_in::*)(cmsg_stream_buffer&);
using packet_handler_map = std::n_hash_map<int, packet_handler_fn>;

class cnet_process_in : public cgobj {
public:
  DECLARE_SDK_VTABLE(cnet_process_in_vtable, in_vftable)
  union {
    DEFINE_MEMBER_N(packet_handler_map m_handlers, 0x04);
    DEFINE_MEMBER_N(void* m_handler_registry, 0x28);
    DEFINE_MEMBER_N(void* m_handler_node, 0x2C);
    DEFINE_MEMBER_N(cmsg_stream_buffer* m_scratch_msg, 0x34);
    DEFINE_MEMBER_N(std::uint8_t m_name_flag, 0x3C);
    DEFINE_MEMBER_N(void* m_secondary_obj, 0x58);
  };
};

