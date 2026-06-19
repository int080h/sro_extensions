#include "hooks/net_hook.hpp"

#include "utils/client_config.hpp"
#include "sdk/cmsg.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "utils/log.hpp"
#include "utils/hooks.hpp"
#include "utils/offsets.hpp"

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::net {
  namespace {

    // -------------------------------------------------------------------------
    // Thread-local state (declared early — used by net_msg)
    // -------------------------------------------------------------------------

    thread_local bool programmatic_send = false;

    // -------------------------------------------------------------------------
    // net_msg — RAII builder (absorbed from sdk/net_msg.cpp)
    // -------------------------------------------------------------------------

    class net_msg {
    public:
      explicit net_msg(std::uint16_t opcode) : opcode_(opcode) {
        msg_ = reinterpret_cast<cmsg_stream_buffer*>(storage_);
        cmsg_stream_buffer::construct(msg_, opcode_);
      }

      ~net_msg() {
        if (msg_) {
          msg_->destroy();
          msg_ = nullptr;
        }
      }

      net_msg(const net_msg&) = delete;
      net_msg& operator=(const net_msg&) = delete;

      auto write_bytes(const void* src, std::size_t size) -> net_msg& {
        if (!msg_ || !src || size == 0) {
          return *this;
        }
        ensure_writing();
        msg_->write_bytes(src, size);
        return *this;
      }

      auto write_u8(std::uint8_t value) -> net_msg& { return write_bytes(&value, sizeof(value)); }
      auto write_u16(std::uint16_t value) -> net_msg& { return write_bytes(&value, sizeof(value)); }
      auto write_u32(std::uint32_t value) -> net_msg& { return write_bytes(&value, sizeof(value)); }

      auto write_string(const std::string& value) -> net_msg& {
        if (!msg_) {
          return *this;
        }
        ensure_writing();
        msg_->write_string(value);
        return *this;
      }

      auto send() -> void {
        if (!msg_) {
          return;
        }
        if (writing_) {
          msg_->toggle_after();
          writing_ = false;
        }
        programmatic_send = true;
        cmsg::send_from_buffer(msg_);
        programmatic_send = false;
        msg_->destroy();
        msg_ = nullptr;
      }

    private:
      auto ensure_writing() -> void {
        if (!msg_ || writing_) {
          return;
        }
        msg_->toggle_before();
        writing_ = true;
      }

      alignas(cmsg_stream_buffer) std::uint8_t storage_[sizeof(cmsg_stream_buffer)]{};
      cmsg_stream_buffer* msg_ = nullptr;
      std::uint16_t opcode_ = 0;
      bool writing_ = false;
    };

    // -------------------------------------------------------------------------
    // Hook objects
    // -------------------------------------------------------------------------

    make_hook<convention_type::thiscall_t, cmsg_stream_buffer*, cmsg_stream_buffer*, std::uint16_t, char*, int, int> g_from_wire;
    make_hook<convention_type::thiscall_t, int, void*, cmsg_stream_buffer*> g_stream_dispatch;
    make_hook<convention_type::cdecl_t, void*, cmsg_stream_buffer*> g_send_from_buffer;
    make_hook<convention_type::thiscall_t, int, void*, cmsg*> g_cmsg_dispatch;
    make_hook<convention_type::stdcall_t, int, int, int, cmsg*> g_send_pool_buffer;

    hook_group g_hooks;

    // -------------------------------------------------------------------------
    // Override state
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Handler storage (absorbed from net_manager.cpp)
    // -------------------------------------------------------------------------

    struct handler_entry {
      std::uint32_t id = 0;
      std::uint16_t opcode = 0;
      bool wildcard = false;
      handler_fn callback;
    };

    std::mutex g_handler_mutex;
    std::uint32_t g_next_handler_id = 1;
    std::vector<handler_entry> g_incoming_handlers;
    std::vector<handler_entry> g_outgoing_handlers;
    bool g_has_incoming_handlers = false;
    bool g_has_outgoing_handlers = false;

    auto invoke_chain(const std::vector<handler_entry>& entries, std::uint16_t opcode, packet_view& ctx) -> handler_result {
      for (const auto& entry : entries) {
        if (!entry.wildcard && entry.opcode != opcode) {
          continue;
        }
        if (!entry.callback) {
          continue;
        }
        const auto result = entry.callback(ctx);
        if (result == handler_result::consume) {
          return handler_result::consume;
        }
      }
      return handler_result::pass;
    }

    auto add_handler(std::vector<handler_entry>& list, bool& has_handlers, std::uint16_t opcode, bool wildcard, handler_fn handler) -> handler_token {
      std::lock_guard lock(g_handler_mutex);
      const handler_token token{g_next_handler_id++};
      list.push_back(handler_entry{token.id, opcode, wildcard, std::move(handler)});
      has_handlers = true;
      return token;
    }

    auto remove_from(std::vector<handler_entry>& list, bool& has_handlers, std::uint32_t id) -> void {
      list.erase(std::remove_if(list.begin(), list.end(), [id](const handler_entry& entry) { return entry.id == id; }), list.end());
      if (list.empty()) {
        has_handlers = false;
      }
    }

    // -------------------------------------------------------------------------
    // Log storage (absorbed from net_manager.cpp)
    // -------------------------------------------------------------------------

    std::mutex g_log_mutex;
    std::vector<entry> g_entries;
    std::uint32_t g_next_log_id = 1;

    FILE* g_log_file = nullptr;
    char g_active_log_path[260]{};

    // Deferred file I/O queue (Phase 5 — flush from render thread)
    std::mutex g_file_queue_mutex;
    std::vector<entry> g_file_queue;
    bool g_file_queue_empty = true;

    auto close_log_file() -> void {
      if (g_log_file) {
        std::fclose(g_log_file);
        g_log_file = nullptr;
      }
      g_active_log_path[0] = '\0';
    }

    auto append_file(const entry& item) -> void {
      const auto& cfg = control_panel();
      if (!cfg.log_to_file || cfg.file_path[0] == '\0') {
        close_log_file();
        return;
      }

      if (!g_log_file || std::strcmp(g_active_log_path, cfg.file_path) != 0) {
        close_log_file();
        if (fopen_s(&g_log_file, cfg.file_path, "a") != 0 || !g_log_file) {
          return;
        }
        std::strncpy(g_active_log_path, cfg.file_path, sizeof(g_active_log_path) - 1);
        g_active_log_path[sizeof(g_active_log_path) - 1] = '\0';
      }

      std::fprintf(g_log_file,
                   "%s %s 0x%04X  len=%zu",
                   item.direction == packet_direction::client_to_server ? "C->S" : "S->C",
                   format_layer(item.layer),
                   item.opcode,
                   item.payload.size());

      if (item.blocked) {
        std::fputs(" [blocked]", g_log_file);
      }
      if (item.modified) {
        std::fputs(" [modified]", g_log_file);
      }

      if (!item.payload.empty()) {
        std::fputs("  ", g_log_file);
        for (const auto byte : item.payload) {
          std::fprintf(g_log_file, "%02X", byte);
        }
      }

      std::fputc('\n', g_log_file);
    }

    auto should_capture(packet_direction direction, packet_layer layer) -> bool {
      const auto& cfg = control_panel();
      if (!cfg.enabled || cfg.pause_capture) {
        return false;
      }
      if (direction == packet_direction::client_to_server && !cfg.log_outgoing) {
        return false;
      }
      if (direction == packet_direction::server_to_client && !cfg.log_incoming) {
        return false;
      }
      if (layer == packet_layer::cmsg && !cfg.capture_cmsg) {
        return false;
      }
      if (layer == packet_layer::stream && !cfg.capture_stream) {
        return false;
      }
      return true;
    }

    // Phase 4: push_entry is unconditional — should_capture() guards in record_*.
    auto push_entry(packet_direction direction,
                    packet_layer layer,
                    std::uint16_t opcode,
                    std::vector<std::uint8_t> payload,
                    bool blocked,
                    bool modified,
                    const char* capture_point) -> void {
      const auto& cfg = control_panel();

      entry item{};
      item.id = g_next_log_id++;
      item.tick = GetTickCount();
      item.timestamp_ms = static_cast<std::uint64_t>(GetTickCount());
      item.direction = direction;
      item.layer = layer;
      item.opcode = opcode;
      item.payload = std::move(payload);
      item.capture_point = capture_point;
      item.blocked = blocked;
      item.modified = modified;

      // Phase 5: defer file I/O to render thread
      if (cfg.log_to_file) {
        std::lock_guard fq_lock(g_file_queue_mutex);
        g_file_queue.push_back(item);
        g_file_queue_empty = false;
      }

      {
        std::lock_guard lock(g_log_mutex);
        g_entries.push_back(std::move(item));
        if (g_entries.size() > static_cast<std::size_t>(cfg.max_entries > 0 ? cfg.max_entries : 0)) {
          const auto overflow = g_entries.size() - static_cast<std::size_t>(cfg.max_entries > 0 ? cfg.max_entries : 0);
          g_entries.erase(g_entries.begin(), g_entries.begin() + static_cast<std::ptrdiff_t>(overflow));
        }
      }
    }

    auto resolve_stream_opcode(cmsg_stream_buffer* msg, std::uint16_t override_opcode) -> std::uint16_t {
      if (override_opcode != 0) {
        return override_opcode;
      }
      return msg->opcode();
    }

    auto resolve_cmsg_opcode(cmsg* pkt, std::uint16_t override_opcode) -> std::uint16_t {
      if (override_opcode != 0) {
        return override_opcode;
      }
      const auto opcode = pkt->opcode();
      return opcode != 0 ? opcode : pkt->header_opcode();
    }

    auto parse_opcode_token(const char*& cursor, std::uint16_t& opcode) -> bool {
      while (*cursor != '\0' && (std::isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',')) {
        ++cursor;
      }
      if (*cursor == '\0') {
        return false;
      }

      char* end = nullptr;
      const unsigned long value = std::strtoul(cursor, &end, 16);
      if (end == cursor || value > 0xFFFFu) {
        return false;
      }

      opcode = static_cast<std::uint16_t>(value);
      cursor = end;
      return true;
    }

    // -------------------------------------------------------------------------
    // Hook helpers
    // -------------------------------------------------------------------------

    auto should_block(packet_direction direction, std::uint16_t opcode) -> bool {
      const auto& cfg = control_panel();
      if (direction == packet_direction::client_to_server && cfg.block_outgoing) {
        return true;
      }
      if (direction == packet_direction::server_to_client && cfg.block_incoming) {
        return true;
      }
      return should_block_opcode(opcode);
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

    auto capture_stream(packet_direction direction, cmsg_stream_buffer* msg, bool blocked, bool modified, const char* capture_point) -> void {
      if (!msg) {
        return;
      }

      const auto opcode = msg->opcode();
      const auto size = msg->payload_size();
      const auto dir = direction == packet_direction::client_to_server ? "C->S" : "S->C";

      if (direction == packet_direction::client_to_server) {
        record_outgoing(msg, blocked, modified, opcode, capture_point);
      } else {
        record_incoming(msg, blocked, modified, opcode, capture_point);
      }

      debug_log("[net] stream %s opcode=%s len=%zu%s%s",
                dir,
                format_opcode(opcode),
                size,
                blocked ? " blocked" : "",
                modified ? " modified" : "");
    }

    auto capture_cmsg(packet_direction direction, cmsg* msg, bool blocked, bool modified, const char* capture_point) -> void {
      if (!msg) {
        return;
      }

      const auto opcode = msg->opcode();
      const auto size = msg->body_size();
      const auto dir = direction == packet_direction::client_to_server ? "C->S" : "S->C";

      if (direction == packet_direction::client_to_server) {
        record_cmsg_outgoing(msg, blocked, modified, opcode, capture_point);
      } else {
        record_cmsg_incoming(msg, blocked, modified, opcode, capture_point);
      }

      if (direction == packet_direction::server_to_client) {
        debug_log("[net] CMsg %s opcode=%s len=%zu encrypted=%s",
                  dir,
                  format_opcode(opcode),
                  size,
                  msg->is_encrypted() ? "yes" : "no");
      } else {
        debug_log("[net] CMsg %s opcode=%s len=%zu%s%s",
                  dir,
                  format_opcode(opcode),
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

      if (!programmatic_send && dispatch_outgoing(msg) == handler_result::consume) {
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

    // -------------------------------------------------------------------------
    // Detours — stream layer
    // -------------------------------------------------------------------------

    auto __cdecl send_from_buffer_detour(cmsg_stream_buffer* msg) -> void* {
      bool blocked = false;
      bool modified = false;
      process_outgoing_stream(msg, blocked, modified);
      capture_stream(packet_direction::client_to_server, msg, blocked, modified, "send_from_buffer");

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

      capture_stream(packet_direction::server_to_client, msg, false, modified, "from_wire");
      return msg;
    }

    auto __fastcall stream_dispatch_detour(void* self, void* /*edx*/, cmsg_stream_buffer* msg) -> int {
      if (msg) {
        const auto opcode = msg->opcode();
        take_incoming_modified(msg);

        if (dispatch_incoming(msg) == handler_result::consume) {
          debug_log("[net] consumed by handler opcode=%s", format_opcode(opcode));
          return 1;
        }

        if (control_panel().enabled && should_block(packet_direction::server_to_client, opcode)) {
          debug_log("[net] dropped dispatch opcode=%s", format_opcode(opcode));
          return 1;
        }
      }

      return g_stream_dispatch.call_original(self, msg);
    }

    // -------------------------------------------------------------------------
    // Detours — CMsg layer
    // -------------------------------------------------------------------------

    auto __fastcall cmsg_dispatch_detour(void* session, void* /*edx*/, cmsg* msg) -> int {
      capture_cmsg(packet_direction::server_to_client, msg, false, false, "cmsg_dispatch");
      return g_cmsg_dispatch.call_original(session, msg);
    }

    auto __stdcall send_pool_buffer_detour(int socket, int session, cmsg* msg) -> int {
      if (msg) {
        capture_cmsg(packet_direction::client_to_server, msg, false, false, "send_pool_buffer");
      }
      return g_send_pool_buffer.call_original(socket, session, msg);
    }

  } // namespace

  // -------------------------------------------------------------------------
  // Config
  // -------------------------------------------------------------------------

  auto control_panel() -> control& {
    return ext_client::config::data().net;
  }

  // -------------------------------------------------------------------------
  // Handler registration
  // -------------------------------------------------------------------------

  auto on_incoming(std::uint16_t opcode, handler_fn handler) -> handler_token {
    return add_handler(g_incoming_handlers, g_has_incoming_handlers, opcode, false, std::move(handler));
  }

  auto on_incoming_any(handler_fn handler) -> handler_token {
    return add_handler(g_incoming_handlers, g_has_incoming_handlers, 0, true, std::move(handler));
  }

  auto on_outgoing(std::uint16_t opcode, handler_fn handler) -> handler_token {
    return add_handler(g_outgoing_handlers, g_has_outgoing_handlers, opcode, false, std::move(handler));
  }

  auto on_outgoing_any(handler_fn handler) -> handler_token {
    return add_handler(g_outgoing_handlers, g_has_outgoing_handlers, 0, true, std::move(handler));
  }

  auto remove(handler_token token) -> void {
    if (!token.valid()) {
      return;
    }
    std::lock_guard lock(g_handler_mutex);
    remove_from(g_incoming_handlers, g_has_incoming_handlers, token.id);
    remove_from(g_outgoing_handlers, g_has_outgoing_handlers, token.id);
  }

  auto clear_handlers() -> void {
    std::lock_guard lock(g_handler_mutex);
    g_incoming_handlers.clear();
    g_outgoing_handlers.clear();
    g_has_incoming_handlers = false;
    g_has_outgoing_handlers = false;
  }

  auto dispatch_incoming(cmsg_stream_buffer* msg) -> handler_result {
    if (!msg || !g_has_incoming_handlers) {
      return handler_result::pass;
    }

    packet_view ctx{};
    ctx.msg = msg;
    ctx.direction = packet_direction::server_to_client;
    ctx.opcode = msg->opcode();

    std::lock_guard lock(g_handler_mutex);
    return invoke_chain(g_incoming_handlers, ctx.opcode, ctx);
  }

  auto dispatch_outgoing(cmsg_stream_buffer* msg) -> handler_result {
    if (!msg || !g_has_outgoing_handlers) {
      return handler_result::pass;
    }

    packet_view ctx{};
    ctx.msg = msg;
    ctx.direction = packet_direction::client_to_server;
    ctx.opcode = msg->opcode();

    std::lock_guard lock(g_handler_mutex);
    return invoke_chain(g_outgoing_handlers, ctx.opcode, ctx);
  }

  // -------------------------------------------------------------------------
  // Send
  // -------------------------------------------------------------------------

  auto send(std::uint16_t opcode) -> void {
    net_msg msg(opcode);
    msg.send();
  }

  auto send(std::uint16_t opcode, const std::uint8_t* data, std::size_t size) -> void {
    net_msg msg(opcode);
    if (data && size > 0) {
      msg.write_bytes(data, size);
    }
    msg.send();
  }

  auto send(std::uint16_t opcode, const std::vector<std::uint8_t>& payload) -> void {
    send(opcode, payload.data(), payload.size());
  }

  // -------------------------------------------------------------------------
  // Recording
  // -------------------------------------------------------------------------

  constexpr std::size_t k_max_payload_capture = 4096;

  auto truncate_payload(std::vector<std::uint8_t> payload) -> std::vector<std::uint8_t> {
    if (payload.size() > k_max_payload_capture) {
      payload.resize(k_max_payload_capture);
    }
    return payload;
  }

  auto record_outgoing(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!msg || !should_capture(packet_direction::client_to_server, packet_layer::stream)) {
      return;
    }
    push_entry(packet_direction::client_to_server,
               packet_layer::stream,
               resolve_stream_opcode(msg, opcode),
               truncate_payload(msg->extract_payload()),
               blocked,
               modified,
               capture_point);
  }

  auto record_incoming(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!msg || !should_capture(packet_direction::server_to_client, packet_layer::stream)) {
      return;
    }
    push_entry(packet_direction::server_to_client,
               packet_layer::stream,
               resolve_stream_opcode(msg, opcode),
               truncate_payload(msg->extract_payload()),
               blocked,
               modified,
               capture_point);
  }

  auto record_cmsg_outgoing(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!pkt || !should_capture(packet_direction::client_to_server, packet_layer::cmsg)) {
      return;
    }
    push_entry(packet_direction::client_to_server,
               packet_layer::cmsg,
               resolve_cmsg_opcode(pkt, opcode),
               truncate_payload(pkt->extract_payload()),
               blocked,
               modified,
               capture_point);
  }

  auto record_cmsg_incoming(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!pkt || !should_capture(packet_direction::server_to_client, packet_layer::cmsg)) {
      return;
    }
    push_entry(packet_direction::server_to_client,
               packet_layer::cmsg,
               resolve_cmsg_opcode(pkt, opcode),
               truncate_payload(pkt->extract_payload()),
               blocked,
               modified,
               capture_point);
  }

  // -------------------------------------------------------------------------
  // Log management
  // -------------------------------------------------------------------------

  auto clear_log() -> void {
    std::lock_guard lock(g_log_mutex);
    g_entries.clear();
  }

  auto count() -> std::size_t {
    std::lock_guard lock(g_log_mutex);
    return g_entries.size();
  }

  auto visit_entries(const std::function<void(const std::vector<entry>&)>& visitor) -> void {
    std::lock_guard lock(g_log_mutex);
    visitor(g_entries);
  }

  auto snapshot() -> std::vector<entry> {
    std::vector<entry> copy;
    visit_entries([&copy](const std::vector<entry>& entries) { copy = entries; });
    return copy;
  }

  auto flush_log() -> void {
    if (g_file_queue_empty) {
      return;
    }
    std::vector<entry> queue;
    {
      std::lock_guard fq_lock(g_file_queue_mutex);
      queue.swap(g_file_queue);
      g_file_queue_empty = true;
    }
    for (const auto& item : queue) {
      append_file(item);
    }
    if (g_log_file) {
      std::fflush(g_log_file);
    }
  }

  // -------------------------------------------------------------------------
  // Codec
  // -------------------------------------------------------------------------

  auto format_opcode(std::uint16_t opcode) -> const char* {
    thread_local char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "0x%04X", opcode);
    return buffer;
  }

  auto format_layer(packet_layer layer) -> const char* {
    return layer == packet_layer::cmsg ? "CMsg" : "stream";
  }

  auto format_hex(const std::vector<std::uint8_t>& bytes, std::size_t bytes_per_row) -> std::string {
    if (bytes.empty()) {
      return {};
    }

    std::string out;
    out.reserve(bytes.size() * 3);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      if (i != 0) {
        if (i % bytes_per_row == 0) {
          out.push_back('\n');
        } else {
          out.push_back(' ');
        }
      }
      char pair[4]{};
      std::snprintf(pair, sizeof(pair), "%02X", bytes[i]);
      out.append(pair);
    }
    return out;
  }

  auto parse_hex(const char* text, std::vector<std::uint8_t>& out) -> bool {
    out.clear();
    if (!text) {
      return true;
    }

    int value = -1;
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
      const char ch = *cursor;
      if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t' || ch == ',') {
        continue;
      }

      int nibble = -1;
      if (ch >= '0' && ch <= '9') {
        nibble = ch - '0';
      } else if (ch >= 'a' && ch <= 'f') {
        nibble = 10 + (ch - 'a');
      } else if (ch >= 'A' && ch <= 'F') {
        nibble = 10 + (ch - 'A');
      } else {
        return false;
      }

      if (value < 0) {
        value = nibble;
      } else {
        out.push_back(static_cast<std::uint8_t>((value << 4) | nibble));
        value = -1;
      }
    }

    return value < 0;
  }

  auto parse_opcode_list(const char* text, std::vector<std::uint16_t>& out) -> bool {
    out.clear();
    if (!text || text[0] == '\0') {
      return true;
    }

    const char* cursor = text;
    const char* start = text;
    std::uint16_t opcode = 0;
    while (parse_opcode_token(cursor, opcode)) {
      out.push_back(opcode);
    }

    while (*cursor != '\0' && (std::isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',')) {
      ++cursor;
    }

    if (out.empty()) {
      return start[0] == '\0';
    }
    return *cursor == '\0';
  }

  auto opcode_in_list(std::uint16_t opcode, const char* list_text) -> bool {
    if (!list_text || list_text[0] == '\0') {
      return false;
    }

    const char* cursor = list_text;
    std::uint16_t parsed = 0;
    while (parse_opcode_token(cursor, parsed)) {
      if (parsed == opcode) {
        return true;
      }
    }
    return false;
  }

  // -------------------------------------------------------------------------
  // Block logic
  // -------------------------------------------------------------------------

  auto should_block_opcode(std::uint16_t opcode) -> bool {
    const auto& cfg = control_panel();
    switch (static_cast<opcode_block_mode>(cfg.block_opcode_mode)) {
      case opcode_block_mode::single:
        return opcode == cfg.block_opcode;
      case opcode_block_mode::list:
        return opcode_in_list(opcode, cfg.block_opcode_list);
      default:
        return false;
    }
  }

  // -------------------------------------------------------------------------
  // Overrides
  // -------------------------------------------------------------------------

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

  // -------------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------------

  auto register_handlers() -> void {
    on_incoming_any([](packet_view& ctx) -> handler_result {
      if (ctx.opcode >= 0x5000 && ctx.opcode <= 0x50FF) {
        log_msg("[net] auth S->C opcode=0x%04X len=%zu", ctx.opcode, ctx.msg->payload_size());
      }
      return handler_result::pass;
    });
  }

  auto shutdown() -> void {
    clear_handlers();
    std::lock_guard lock(g_log_mutex);
    close_log_file();
  }

  // -------------------------------------------------------------------------
  // Hook install / uninstall
  // -------------------------------------------------------------------------

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    if (!g_hooks.install(
          g_from_wire, ext_client::offsets::cmsg_stream_buffer::functions::from_wire, from_wire_detour, "net_hook", "from_wire") ||
        !g_hooks.install(g_stream_dispatch,
                         ext_client::offsets::cmsg_stream_buffer::functions::dispatch_to_process,
                         stream_dispatch_detour,
                         "net_hook",
                         "stream_dispatch") ||
        !g_hooks.install(g_send_from_buffer,
                         ext_client::offsets::cmsg::functions::send_from_buffer,
                         send_from_buffer_detour,
                         "net_hook",
                         "send_from_buffer") ||
        !g_hooks.install(
          g_cmsg_dispatch, ext_client::offsets::cmsg::functions::dispatch_handler, cmsg_dispatch_detour, "net_hook", "cmsg_dispatch") ||
        !g_hooks.install(g_send_pool_buffer,
                         ext_client::offsets::cnet_engine::functions::send_pool_buffer,
                         send_pool_buffer_detour,
                         "net_hook",
                         "send_pool_buffer")) {
      return false;
    }

    log_msg("[net_hook] installed (5 hooks)");
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

} // namespace ext_client::hooks::net
