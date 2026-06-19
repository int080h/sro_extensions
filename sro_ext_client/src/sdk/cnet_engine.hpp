#pragma once

#include "utils/offsets.hpp"

#include <cstdint>

class cnet_engine;
struct cmsg;

// Socket bootstrap blob copied onto the object at +8 (qmemcpy 0x114 bytes in IBSNet::Initialize).
// x86 passes it inlined on the stack; IDA decompiler expands that into dozens of scalar args.
struct ibsnet_init_config {
  PAD_BYTES(raw, 0x114);
};

// CNetEngine / IBSNet — Joymax socket layer (Winsock, CMsg pools, active/passive sockets).
struct ibsnet_vtable {
  VFN_STDCALL(query_interface, int, void* iid, void** out);
  VFN_STDCALL(add_ref, int, int unused);
  VFN_STDCALL(release, int, int unused);
  VFN_STDCALL(initialize, int, void* socket, const ibsnet_init_config* config);
  VFN_STDCALL(create_passive_socket, int, void* local_addr, void* remote_addr, int backlog, int flags);
  VFN_STDCALL(connect, int, char* host, std::uint16_t port, char* bind_addr, int a5, int a6);
  VFN_STDCALL(bind, int, void* sockaddr);
  VFN_STDCALL(get_host_by_name, int, int* out, char* hostname, std::uint16_t port);
  VFN_STDCALL(create_active_socket, int, char mode, void*** socket_out, int a4, int a5);
  VFN_STDCALL(listen, int, int a2, int a3, int a4, int a5);
  VFN_STDCALL(accept, int, char a2, int a3, int a4, int a5, int a6, int a7);
  VFN_STDCALL(on_socket_event, int, int socket, int a2);
  VFN_STDCALL(on_socket_event_ex, int, int socket, int a2);
  VFN_STDCALL(close_socket, int, int socket, int a2);
  VFN_STDCALL(shutdown, int, int a1, int a2, int a3);
  VFN_STDCALL(get_local_address, int, int socket, void* host_buf, std::uint32_t* ip, std::uint16_t* port);
  VFN_STDCALL(set_socket_mode, int, int socket, char mode);
  VFN_STDCALL(send, int, int* socket, int a2, void* data);
  VFN_STDCALL(alloc_msg_pool, cmsg*, int socket, int encrypted);
  VFN_STDCALL(free_msg_pool, void, int socket, cmsg* msg);
  VFN_STDCALL(send_pool_buffer, int, int socket, int session, cmsg* msg);
  VFN_STDCALL(
    alloc_msg, int, int socket, int a2, int a3, void* block, int a5, int a6, int a7, int a8, unsigned int a9, int a10, int a11, int a12);
  VFN_STDCALL(free_msg, int, int socket, cmsg* msg, int a3, int a4);
  VFN_STDCALL(flush_send, int, int socket, int a2, int a3);
  VFN_STDCALL(set_recv_callback, int, int socket, int a2, char a3);
  VFN_STDCALL(get_pending_count, int, int socket, int a2);
  VFN_STDCALL(get_send_count, int, int socket, int a2);
  VFN_STDCALL(pump_socket, int, int* socket, int a2);
  VFN_STDCALL(connect_ex, int, int a1, char* host, std::uint16_t port, char* bind_addr, int a5, int a6);
  VFN_STDCALL(null_29, int, int a1);
  VFN_STDCALL(null_30, void, int a1, int a2, int a3);
  VFN_STDCALL(is_socket_alive, char, int socket, char a2);
  VFN_STDCALL(get_socket_state, int, int socket);
  VFN_STDCALL(get_engine_state, int, int socket);
  VFN_STDCALL(get_socket_error, int, int socket, int a2, int* out);
  VFN_STDCALL(set_socket_opt, int, int socket, char opt);
  VFN_STDCALL(cleanup_socket, void, int socket);
  VFN_STDCALL(get_socket_flag, int, int socket, int a2);
  VFN_STDCALL(set_socket_flag, int, int socket, char optval);
  VFN_STDCALL(ioctl_socket, int, int socket, int a2, int a3);
  VFN_THISCALL(scalar_deleting_dtor, char*, cnet_engine* self, char should_free);
  VFN_STDCALL(is_connected, char, int socket, int a2);
};

static_assert(ext_client::offsets::cnet_engine::vtable::method_count * sizeof(void*) == sizeof(ibsnet_vtable));

class cnet_engine {
public:
  DECLARE_SDK_VTABLE(ibsnet_vtable, engine_vftable)

private:
  ibsnet_vtable* vftable;
  int m_ref_count;
  PAD_TO(ext_client::offsets::cnet_engine::fields::ref_count + sizeof(int), ext_client::offsets::cnet_engine::fields::com_ptr);
  void* m_com_ptr;
  PAD_TO(ext_client::offsets::cnet_engine::fields::com_ptr + sizeof(void*), ext_client::offsets::cnet_engine::fields::socket_manager);
  void* m_socket_manager;
  void* m_socket_manager_tail;
  PAD_TO(ext_client::offsets::cnet_engine::fields::socket_manager_tail + sizeof(void*), ext_client::offsets::cnet_engine::size);

  static inline auto check_layout() -> void {
    static_assert(offsetof(cnet_engine, m_com_ptr) == ext_client::offsets::cnet_engine::fields::com_ptr,
                  "cnet_engine::m_com_ptr offset mismatch");
    static_assert(offsetof(cnet_engine, m_socket_manager) == ext_client::offsets::cnet_engine::fields::socket_manager,
                  "cnet_engine::m_socket_manager offset mismatch");
    static_assert(sizeof(cnet_engine) == ext_client::offsets::cnet_engine::size, "cnet_engine size mismatch");
  }
};
