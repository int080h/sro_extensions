#include "hooks/packet_hook.hpp"

#include "sdk/net_manager.hpp"
#include "sdk/cmsg.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "utils/log.hpp"
#include "utils/hooks.hpp"
#include "utils/offsets.hpp"

#include <functional>
#include <mutex>
#include <vector>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::packet {
  namespace {

    // ---------------------------------------------------------------------------
    // Capture points (see packet_hook.hpp)
    // ---------------------------------------------------------------------------

    make_hook<convention_type::thiscall_t, cmsg_stream_buffer*, cmsg_stream_buffer*, std::uint16_t, char*, int, int> g_from_wire;
    make_hook<convention_type::thiscall_t, int, void*, cmsg_stream_buffer*> g_stream_dispatch;
    make_hook<convention_type::cdecl_t, void*, cmsg_stream_buffer*> g_send_from_buffer;
    make_hook<convention_type::thiscall_t, int, void*, cmsg*> g_cmsg_dispatch;
    make_hook<convention_type::stdcall_t, int, int, int, cmsg*> g_send_pool_buffer;

    hook_group g_hooks;

    std::mutex g_override_mutex;

    struct override_rule {
      std::uint16_t opcode = 0;
      bool apply_all = false;
      std::vector<std::uint8_t> payload;
    };

    override_rule g_outgoing_override{};
    override_rule g_incoming_override{};

    thread_local cmsg_stream_buffer* g_incoming_override_msg = nullptr;
    thread_local bool g_incoming_override_modified = false;

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    auto should_block(packet_direction direction, std::uint16_t opcode) -> bool {
      const auto& cfg = ext_client::net_manager::control_panel();
      if (direction == packet_direction::client_to_server && cfg.block_outgoing) {
        return true;
      }
      if (direction == packet_direction::server_to_client && cfg.block_incoming) {
        return true;
      }
      return ext_client::net_manager::should_block_opcode(opcode);
    }

    template<typename... Args> auto debug_log(const char* fmt, Args... args) -> void {
      if (control_panel().log_events) {
        log_msg(fmt, args...);
      }
    }

    auto apply_override(cmsg_stream_buffer* msg, const override_rule& rule) -> bool {
      if (!msg || rule.payload.empty()) {
        return false;
      }
      const auto opcode = msg->opcode();
      if (!rule.apply_all && opcode != rule.opcode) {
        return false;
      }
      return msg->replace_payload(rule.payload.data(), rule.payload.size());
    }

    auto take_incoming_modified(cmsg_stream_buffer* msg) -> bool {
      const auto modified = msg && msg == g_incoming_override_msg && g_incoming_override_modified;
      g_incoming_override_msg = nullptr;
      g_incoming_override_modified = false;
      return modified;
    }

    auto capture_stream(packet_direction direction, cmsg_stream_buffer* msg, bool blocked, bool modified) -> void {
      if (!msg) {
        return;
      }

      const auto opcode = msg->opcode();
      const auto size = msg->payload_size();
      const auto dir = direction == packet_direction::client_to_server ? "C->S" : "S->C";

      if (direction == packet_direction::client_to_server) {
        ext_client::net_manager::record_outgoing(msg, blocked, modified, opcode);
      } else {
        ext_client::net_manager::record_incoming(msg, blocked, modified, opcode);
      }

      debug_log("[net] stream %s opcode=%s len=%zu%s%s",
                dir,
                ext_client::net_manager::format_opcode(opcode).c_str(),
                size,
                blocked ? " blocked" : "",
                modified ? " modified" : "");
    }

    auto capture_cmsg(packet_direction direction, cmsg* msg, bool blocked, bool modified) -> void {
      if (!msg) {
        return;
      }

      const auto opcode = msg->opcode();
      const auto size = msg->body_size();
      const auto dir = direction == packet_direction::client_to_server ? "C->S" : "S->C";

      if (direction == packet_direction::client_to_server) {
        ext_client::net_manager::record_cmsg_outgoing(msg, blocked, modified, opcode);
      } else {
        ext_client::net_manager::record_cmsg_incoming(msg, blocked, modified, opcode);
      }

      if (direction == packet_direction::server_to_client) {
        debug_log("[net] CMsg %s opcode=%s len=%zu encrypted=%s",
                  dir,
                  ext_client::net_manager::format_opcode(opcode).c_str(),
                  size,
                  msg->is_encrypted() ? "yes" : "no");
      } else {
        debug_log("[net] CMsg %s opcode=%s len=%zu%s%s",
                  dir,
                  ext_client::net_manager::format_opcode(opcode).c_str(),
                  size,
                  blocked ? " blocked" : "",
                  modified ? " modified" : "");
      }
    }

    auto process_outgoing_stream(cmsg_stream_buffer* msg, bool& blocked, bool& modified) -> void {
      if (!msg) {
        return;
      }

      const auto opcode = msg->opcode();

      if (!ext_client::net_manager::detail::programmatic_send &&
          ext_client::net_manager::dispatch_outgoing(msg) == ext_client::net_manager::handler_result::consume) {
        blocked = true;
      }

      if (control_panel().enabled) {
        std::lock_guard lock(g_override_mutex);
        if (control_panel().edit_outgoing) {
          modified = apply_override(msg, g_outgoing_override) || modified;
        }
      }

      blocked = blocked || should_block(packet_direction::client_to_server, opcode);
    }

    // ---------------------------------------------------------------------------
    // Detours — stream layer
    // ---------------------------------------------------------------------------

    // SendMsg entry (cdecl). Covers sync sends and deferred queueing; stream_to_wire is
    // __usercall (buf in ESI) and only called downstream — hooking it is unnecessary.
    auto __cdecl send_from_buffer_detour(cmsg_stream_buffer* msg) -> void* {
      bool blocked = false;
      bool modified = false;
      process_outgoing_stream(msg, blocked, modified);
      capture_stream(packet_direction::client_to_server, msg, blocked, modified);

      if (blocked) {
        return nullptr;
      }

      return g_send_from_buffer.call_original(msg);
    }

    auto __fastcall from_wire_detour(cmsg_stream_buffer* self, void* /*edx*/, std::uint16_t opcode, char* src, int size, int mode)
      -> cmsg_stream_buffer* {
      cmsg_stream_buffer* msg = g_from_wire.call_original(self, opcode, src, size, mode);
      if (!msg) {
        return msg;
      }

      g_incoming_override_msg = nullptr;
      g_incoming_override_modified = false;

      bool modified = false;
      if (control_panel().enabled) {
        std::lock_guard lock(g_override_mutex);
        if (control_panel().edit_incoming && apply_override(msg, g_incoming_override)) {
          g_incoming_override_msg = msg;
          g_incoming_override_modified = true;
          modified = true;
        }
      }

      capture_stream(packet_direction::server_to_client, msg, false, modified);
      return msg;
    }

    // Block/consume only — packets are logged at from_wire.
    auto __fastcall stream_dispatch_detour(void* self, void* /*edx*/, cmsg_stream_buffer* msg) -> int {
      if (msg) {
        const auto opcode = msg->opcode();
        take_incoming_modified(msg);

        if (ext_client::net_manager::dispatch_incoming(msg) == ext_client::net_manager::handler_result::consume) {
          debug_log("[net] consumed by handler opcode=%s", ext_client::net_manager::format_opcode(opcode).c_str());
          return 1;
        }

        if (control_panel().enabled && should_block(packet_direction::server_to_client, opcode)) {
          debug_log("[net] dropped dispatch opcode=%s", ext_client::net_manager::format_opcode(opcode).c_str());
          return 1;
        }
      }
      return g_stream_dispatch.call_original(self, msg);
    }

    // ---------------------------------------------------------------------------
    // Detours — CMsg layer
    // ---------------------------------------------------------------------------

    auto __fastcall cmsg_dispatch_detour(void* session, void* /*edx*/, cmsg* msg) -> int {
      capture_cmsg(packet_direction::server_to_client, msg, false, false);
      return g_cmsg_dispatch.call_original(session, msg);
    }

    auto __stdcall send_pool_buffer_detour(int socket, int session, cmsg* msg) -> int {
      if (msg) {
        capture_cmsg(packet_direction::client_to_server, msg, false, false);
      }
      return g_send_pool_buffer.call_original(socket, session, msg);
    }

  } // namespace

  auto control_panel() -> control& {
    return ext_client::config::data().packet;
  }

  auto set_outgoing_override(std::uint16_t opcode, const std::uint8_t* payload, std::size_t size, bool apply_to_all_matching) -> void {
    std::lock_guard lock(g_override_mutex);
    g_outgoing_override.opcode = opcode;
    g_outgoing_override.apply_all = apply_to_all_matching;
    g_outgoing_override.payload.assign(payload, payload + size);
    control_panel().edit_outgoing = !g_outgoing_override.payload.empty();
    control_panel().edit_outgoing_opcode = opcode;
    control_panel().edit_outgoing_apply_all = apply_to_all_matching;
  }

  auto set_incoming_override(std::uint16_t opcode, const std::uint8_t* payload, std::size_t size, bool apply_to_all_matching) -> void {
    std::lock_guard lock(g_override_mutex);
    g_incoming_override.opcode = opcode;
    g_incoming_override.apply_all = apply_to_all_matching;
    g_incoming_override.payload.assign(payload, payload + size);
    control_panel().edit_incoming = !g_incoming_override.payload.empty();
    control_panel().edit_incoming_opcode = opcode;
    control_panel().edit_incoming_apply_all = apply_to_all_matching;
  }

  auto clear_overrides() -> void {
    std::lock_guard lock(g_override_mutex);
    g_outgoing_override = {};
    g_incoming_override = {};
    control_panel().edit_outgoing = false;
    control_panel().edit_incoming = false;
  }

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    if (!g_hooks.install(
          g_from_wire, ext_client::offsets::cmsg_stream_buffer::functions::from_wire, from_wire_detour, "packet_hook", "from_wire") ||
        !g_hooks.install(g_stream_dispatch,
                         ext_client::offsets::cmsg_stream_buffer::functions::dispatch_to_process,
                         stream_dispatch_detour,
                         "packet_hook",
                         "stream_dispatch") ||
        !g_hooks.install(g_send_from_buffer,
                         ext_client::offsets::cmsg::functions::send_from_buffer,
                         send_from_buffer_detour,
                         "packet_hook",
                         "send_from_buffer") ||
        !g_hooks.install(
          g_cmsg_dispatch, ext_client::offsets::cmsg::functions::dispatch_handler, cmsg_dispatch_detour, "packet_hook", "cmsg_dispatch") ||
        !g_hooks.install(g_send_pool_buffer,
                         ext_client::offsets::cnet_engine::functions::send_pool_buffer,
                         send_pool_buffer_detour,
                         "packet_hook",
                         "send_pool_buffer")) {
      return false;
    }

    log_msg("[packet_hook] installed (5 hooks)");
    return true;
  }

  auto uninstall() -> void {
    if (!g_hooks.is_installed()) {
      return;
    }

    g_hooks.uninstall();
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

} // namespace ext_client::hooks::packet
