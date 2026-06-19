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
  PAD_BYTES(m_storage, ext_client::offsets::cnet_process::fields::handler_map_size);
  cmsg_stream_buffer* m_scratch_msg;
};

// Common base structure for connection phase net processes.
struct cnet_process_phase {
public:
  cnet_process_obj_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cobj_child::fields::region_end);
  PAD_TO(ext_client::offsets::cobj_child::fields::region_end, ext_client::offsets::cnet_process::fields::handler_map);
  cnet_process_phase_handlers handlers;
};

// CNetProcessThird — agent-server phase opcode dispatch.
class cnet_process_third : public cnet_process_phase {};

static_assert(offsetof(cnet_process_third, handlers) == ext_client::offsets::cnet_process_third::fields::handler_map, "cnet_process_third::handlers offset mismatch");
static_assert(offsetof(cnet_process_third, handlers.m_scratch_msg) == ext_client::offsets::cnet_process_third::fields::scratch_msg, "cnet_process_third::handlers.m_scratch_msg offset mismatch");
static_assert(sizeof(cnet_process_third) == ext_client::offsets::cnet_process_third::size, "cnet_process_third size mismatch");

// CNetProcessSecond — gateway/download phase opcode dispatch (same layout as Third).
class cnet_process_second : public cnet_process_phase {};

static_assert(offsetof(cnet_process_second, handlers) == ext_client::offsets::cnet_process_second::fields::handler_map, "cnet_process_second::handlers offset mismatch");
static_assert(offsetof(cnet_process_second, handlers.m_scratch_msg) == ext_client::offsets::cnet_process_second::fields::scratch_msg, "cnet_process_second::handlers.m_scratch_msg offset mismatch");
static_assert(sizeof(cnet_process_second) == ext_client::offsets::cnet_process_second::size, "cnet_process_second size mismatch");

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

class cnet_process_in {
public:
  cnet_process_in_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cobj_child::fields::field_0c);
  int m_field_0c;
  PAD_TO(ext_client::offsets::cobj_child::fields::field_0c + sizeof(int), ext_client::offsets::cobj_child::fields::list_node);
  void* m_list_node[3];
  PAD_TO(ext_client::offsets::cobj_child::fields::list_node + sizeof(void*) * 3, ext_client::offsets::cnet_process_in::fields::handler_map);
  PAD_BYTES(m_handler_map, ext_client::offsets::cnet_process_in::fields::handler_map_size);
  void* m_handler_registry;
  void* m_handler_node;
  PAD_TO(ext_client::offsets::cnet_process_in::fields::handler_node + sizeof(void*),
         ext_client::offsets::cnet_process_in::fields::scratch_msg);
  cmsg_stream_buffer* m_scratch_msg;
  PAD_TO(ext_client::offsets::cnet_process_in::fields::scratch_msg + sizeof(void*),
         ext_client::offsets::cnet_process_in::fields::name_flag);
  std::uint8_t m_name_flag;
  PAD_TO(ext_client::offsets::cnet_process_in::fields::name_flag + sizeof(std::uint8_t),
         ext_client::offsets::cnet_process_in::fields::secondary_obj);
  void* m_secondary_obj;
  PAD_TO(ext_client::offsets::cnet_process_in::fields::secondary_obj + sizeof(void*), ext_client::offsets::cnet_process_in::size);
};

static_assert(offsetof(cnet_process_in, m_handler_map) == ext_client::offsets::cnet_process_in::fields::handler_map);
static_assert(offsetof(cnet_process_in, m_scratch_msg) == ext_client::offsets::cnet_process_in::fields::scratch_msg);
static_assert(sizeof(cnet_process_in) == ext_client::offsets::cnet_process_in::size);
