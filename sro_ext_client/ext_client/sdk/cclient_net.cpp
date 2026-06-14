#include "sdk/cclient_net.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

  auto client_ptr() -> cclient_net* {
    return global_at<cclient_net*>(ext_client::offsets::cclient_net::globals::instance_ptr);
  }

} // namespace

auto cclient_net::instance() -> cclient_net* {
  return client_ptr();
}

auto cclient_net::active_socket() -> void* {
  // dword_1420404 — embedded engine early, active socket sub-object after connect.
  return global_at<void*>(ext_client::offsets::cclient_net::globals::active_socket);
}

auto cclient_net::session() -> cclient_session* {
  return global_at<cclient_session*>(ext_client::offsets::cclient_session::globals::instance_ptr);
}

auto cclient_net::alloc_msg(std::uint16_t opcode, int pool_id) -> cmsg* {
  auto* client = client_ptr();
  if (!client || !client->vftable) {
    return nullptr;
  }
  return client->vftable->alloc_msg_buffer(opcode, pool_id);
}

auto cclient_net::free_msg(cmsg* msg) -> int {
  auto* client = client_ptr();
  if (!client || !client->vftable || !msg) {
    return -1;
  }
  return client->vftable->free_msg_buffer(msg);
}

auto cclient_net::send_msg(cmsg* msg) -> int {
  auto* client = client_ptr();
  if (!client || !client->vftable || !msg) {
    return -1;
  }
  return client->vftable->send_msg_buffer(msg, 0, 0, 0);
}

auto cclient_net::is_connected() -> bool {
  return as_fn<bool (*)()>(ext_client::offsets::cclient_net::functions::is_connected)();
}

auto cclient_net::pump_messages() -> int {
  return as_fn<int (*)()>(ext_client::offsets::cclient_net::functions::pump_messages)();
}
