#pragma once

#include "cmsg.hpp"
#include "cnet_engine.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

struct cclient_net;
struct cclient_session;

// CClientNet / IClientNet — top-level client networking facade (dword_1420400).
struct iclientnet_vtable {
  VFN_THISCALL(connect,
               char,
               cclient_net* self,
               int shard_id,
               int a3,
               char a4,
               char* source,
               int a6,
               int a7,
               std::uint8_t* out,
               int a9,
               const char* host,
               int a11,
               int a12);
  VFN_THISCALL(disconnect, int, cclient_net* self);
  VFN_CDECL(get_shard_info_a, void*);
  VFN_CDECL(get_shard_info_b, void*);
  VFN_CDECL(get_shard_info_c, void*);
  VFN_THISCALL(send_login, char, cclient_net* self, char* user, char* pass, int shard_id, char flag);
  VFN_CDECL(pump_messages, int);
  VFN_CDECL(is_connected, bool);
  VFN_THISCALL(send_patch_request, char, cclient_net* self);
  VFN_THISCALL(send_patch_confirm, char, cclient_net* self);
  VFN_STDCALL(alloc_msg_buffer, cmsg*, std::uint16_t opcode, int pool_id);
  VFN_STDCALL(free_msg_buffer, int, cmsg* msg);
  VFN_STDCALL(send_msg_buffer, int, cmsg* msg, int a3, int a4, int a5);
  VFN_CDECL(get_username, char*);
  VFN_CDECL(get_server_name_a, char*);
  VFN_CDECL(get_server_name_b, char*);
  VFN_CDECL(get_server_name_c, char*);
  VFN_CDECL(get_global_data, int*);
  VFN_STDCALL(build_msg, cmsg*, std::uint16_t opcode, int shard_slot);
  VFN_STDCALL(send_handshake, int, char mode, int a2);
  VFN_THISCALL(send_to_agent, int, cclient_net* self, cmsg* msg);
  VFN_STDCALL(copy_session_id, int, std::uint32_t* out, int a2);
  VFN_THISCALL(flush_and_send, int, cclient_net* self, cmsg* msg);
  VFN_CDECL(flush_session, int);
  VFN_STDCALL(get_challenge, int, void* source, int a2);
  VFN_STDCALL(verify_challenge, int, void* source, int a2, int a3);
  VFN_CDECL(get_secondary_user, char*);
  VFN_CDECL(has_active_socket, int);
  VFN_STDCALL(copy_credentials_a, int, int a1, int a2);
  VFN_STDCALL(copy_credentials_b, int, int a1, int a2);
  VFN_CDECL(get_locale_byte, int);
  VFN_STDCALL(get_version_pair, int*, int*);
  VFN_STDCALL(get_gate_port, int, int a1);
  VFN_THISCALL(send_keepalive, char, cclient_net* self);
  VFN_STDCALL(get_security_version, char, std::uint16_t* a, std::uint16_t* b, std::uint32_t* c);
  VFN_THISCALL(handle_security_blob, char, cclient_net* self, char mode, void* blob);
  VFN_STDCALL(get_security_flag, char, std::uint8_t* out);
  VFN_THISCALL(set_event_sink, int, cclient_net* self, int sink);
  VFN_THISCALL(scalar_deleting_dtor, void*, cclient_net* self, char should_free);
};

// Heap-allocated login/session state (global Src pointer, 0x300 bytes).
struct cclient_session {
  union {
    DEFINE_MEMBER_0(std::uint32_t m_connected, "connected");
    DEFINE_MEMBER_N(std::uint32_t m_connection_id, 0x30);
    DEFINE_MEMBER_N(std::uint32_t m_locale_byte, 0x34);
    DEFINE_MEMBER_N(char m_username[24], 0xA0);
    DEFINE_MEMBER_N(char m_password[108], 0xB8);
    DEFINE_MEMBER_N(std::uint8_t m_challenge_block[56], 0x124);
    DEFINE_MEMBER_N(char m_server_name[256], 0x15C);
    DEFINE_MEMBER_N(char m_secondary_user[28], 0x25C);
    DEFINE_MEMBER_N(char m_gate_host[124], 0x278);
    DEFINE_MEMBER_N(std::uint16_t m_security_version_a, 0x2F4);
    DEFINE_MEMBER_N(std::uint16_t m_security_version_b, 0x2F6);
    DEFINE_MEMBER_N(std::uint32_t m_security_token, 0x2F8);
    DEFINE_MEMBER_N(std::uint32_t m_security_flag, 0x2FC);
  };
};

struct cclient_net {
public:
  static auto instance() -> cclient_net*;
  static auto active_socket() -> void*;
  static auto session() -> cclient_session*;
  static auto alloc_msg(std::uint16_t opcode, int pool_id = 0) -> cmsg*;
  static auto free_msg(cmsg* msg) -> int;
  static auto send_msg(cmsg* msg) -> int;
  static auto is_connected() -> bool;
  static auto pump_messages() -> int;

public:
  iclientnet_vtable* vftable;
  union {
    DEFINE_MEMBER_0(int m_event_sink, "event_sink");
    DEFINE_MEMBER_N(cnet_engine m_engine, 0x04);
    DEFINE_MEMBER_N(void* m_process_registry, 0x4E4);
  };
};

