#include "pch.hpp"
#include "core/hooks/engine_hooks.hpp"
#include "core/event_bus.hpp"
#include "utils/hooks.hpp"
#include "sdk/net/cmsg_stream_buffer.hpp"
#include "sdk/net/cmsg.hpp"

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;
using namespace ext_client::core::event;

namespace ext_client::core::hooks::network_hooks {
  namespace {
    hook_group g_hooks;
    inline auto set_packet_msg(packet_context &ctx, cmsg *msg) -> void {
      ctx.cmsg_msg = msg;
    }
    inline auto set_packet_msg(packet_context &ctx, cmsg_stream_buffer *msg) -> void {
      ctx.stream_msg = msg;
    }

    inline auto opcode_of(const cmsg *msg) -> std::uint16_t {
      return msg->opcode();
    }
    inline auto opcode_of(const cmsg_stream_buffer *msg) -> std::uint16_t {
      return msg->get_opcode();
    }

    template <typename Msg>
    auto make_packet_context(void *session, Msg *msg, ext_client::packet_direction direction, packet_layer layer,
                             const char *capture_point) -> packet_context {
      packet_context ctx{};
      ctx.session = session;
      set_packet_msg(ctx, msg);
      ctx.direction = direction;
      ctx.layer = layer;
      ctx.opcode = msg ? opcode_of(msg) : 0;
      ctx.capture_point = capture_point;
      return ctx;
    }

    // 7. Network Hooks
    make_hook<convention_type::thiscall_t, int, void *, void *, void *> g_dispatch_handler;
    make_hook<convention_type::thiscall_t, int, void *, void *, cmsg **> g_send_cmsg;
    make_hook<convention_type::cdecl_t, void *, cmsg_stream_buffer *> g_send_from_buffer;

    auto __fastcall dispatch_handler_detour(void *self, void *edx, void *msg) -> int {
      ext_client::utils::hook_call_scope active_call;
      const bool has_listeners = event_handler<EVENT_ON_PACKET>::instance().has_active_listeners();
      if (!has_listeners) {
        return g_dispatch_handler.call_original(self, edx, msg);
      }
      auto *cmsg_ptr = static_cast<cmsg *>(msg);
      auto ctx = make_packet_context(self, cmsg_ptr, ext_client::packet_direction::server_to_client, packet_layer::cmsg,
                                     "dispatch_handler");
      TRIGGER_EVENT(EVENT_ON_PACKET, ctx);
      if (ctx.blocked) {
        return ctx.result;
      }
      return g_dispatch_handler.call_original(self, edx, msg);
    }

    auto __fastcall send_cmsg_detour(void *self, void *edx, cmsg **pmsg) -> int {
      ext_client::utils::hook_call_scope active_call;
      if (!pmsg || !*pmsg) {
        return g_send_cmsg.call_original(self, edx, pmsg);
      }
      const bool has_listeners = event_handler<EVENT_ON_PACKET>::instance().has_active_listeners();
      if (!has_listeners) {
        return g_send_cmsg.call_original(self, edx, pmsg);
      }
      auto *cmsg_ptr = *pmsg;
      auto ctx = make_packet_context(self, cmsg_ptr, ext_client::packet_direction::client_to_server, packet_layer::cmsg,
                                     "send_cmsg");
      TRIGGER_EVENT(EVENT_ON_PACKET, ctx);
      if (ctx.blocked) {
        return ctx.result ? ctx.result : 0x8002;
      }
      return g_send_cmsg.call_original(self, edx, pmsg);
    }

    auto __cdecl send_from_buffer_detour(cmsg_stream_buffer *msg) -> void * {
      ext_client::utils::hook_call_scope active_call;
      const bool has_listeners = event_handler<EVENT_ON_PACKET>::instance().has_active_listeners();
      if (!has_listeners) {
        return g_send_from_buffer.call_original(msg);
      }
      auto ctx = make_packet_context(nullptr, msg, ext_client::packet_direction::client_to_server, packet_layer::stream,
                                     "send_from_buffer");
      TRIGGER_EVENT(EVENT_ON_PACKET, ctx);
      if (ctx.blocked) {
        return nullptr;
      }
      return g_send_from_buffer.call_original(msg);
    }
  } // namespace
  auto install() -> bool {
    return g_hooks.install_all(g_dispatch_handler, 0x00DA4B30, &dispatch_handler_detour, "network_hooks",
                               "dispatch_handler", g_send_cmsg, 0x00DA4850, &send_cmsg_detour,
                               "network_hooks", "send_cmsg", g_send_from_buffer, 0x00941600,
                               &send_from_buffer_detour, "network_hooks", "send_from_buffer");
  }
  auto uninstall() -> bool {
    return g_hooks.uninstall();
  }
} // namespace ext_client::core::hooks::network_hooks
