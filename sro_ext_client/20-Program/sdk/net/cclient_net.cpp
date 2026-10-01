#include "pch.hpp"
#include "sdk/net/cclient_net.hpp"

#include "utils/offsets.hpp"



namespace {



  using ext_client::off::as_fn;

  using ext_client::off::global_at;



  using alloc_msg_fn = cmsg*(__stdcall*)(std::uint16_t opcode, int pool_id);

  using free_msg_fn = int(__stdcall*)(cmsg* msg);

  using send_msg_fn = int(__stdcall*)(cmsg* msg, int a3, int a4, int a5);



  auto client_ptr() -> cclient_net* {

    return global_at<cclient_net*>(0x01420400);

  }



} // namespace



auto cclient_net::get_instance() -> cclient_net* {

  return client_ptr();

}



auto cclient_net::get_active_socket() -> void* {

  return global_at<void*>(0x01420404);

}



auto cclient_net::get_session() -> cclient_session* {

  return global_at<cclient_session*>(0x014203FC);

}



auto cclient_net::alloc_msg(std::uint16_t opcode, int pool_id) -> cmsg* {

  const auto fn = as_fn<alloc_msg_fn>(0x0045D200);

  if (!fn) {

    return nullptr;

  }

  return fn(opcode, pool_id);

}



auto cclient_net::free_msg(cmsg* msg) -> int {

  if (!msg) {

    return -1;

  }

  const auto fn = as_fn<free_msg_fn>(0x0045D220);

  if (!fn) {

    return -1;

  }

  return fn(msg);

}



auto cclient_net::send_msg(cmsg* msg) -> int {

  if (!msg) {

    return -1;

  }

  const auto fn = as_fn<send_msg_fn>(0x0045D250);

  if (!fn) {

    return -1;

  }

  return fn(msg, 0, 0, 0);

}



auto cclient_net::is_connected() -> bool {

  return as_fn<bool (*)()>(0x0045D800)();

}



auto cclient_net::pump_messages() -> int {

  return as_fn<int (*)()>(0x0045D5B0)();

}

