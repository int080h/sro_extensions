#include "sdk/net_manager.hpp"

#include "sdk/net_msg.hpp"
#include "sdk/cmsg.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace ext_client::net_manager {

  namespace detail {
    thread_local bool programmatic_send = false;
  } // namespace detail

  namespace {

    struct handler_entry {
      std::uint32_t id = 0;
      std::uint16_t opcode = 0;
      bool wildcard = false;
      handler_fn callback;
    };

    std::mutex g_mutex;
    std::uint32_t g_next_id = 1;
    std::vector<handler_entry> g_incoming;
    std::vector<handler_entry> g_outgoing;

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

    auto add_handler(std::vector<handler_entry>& list, std::uint16_t opcode, bool wildcard, handler_fn handler) -> handler_token {
      std::lock_guard lock(g_mutex);
      const handler_token token{g_next_id++};
      list.push_back(handler_entry{token.id, opcode, wildcard, std::move(handler)});
      return token;
    }

    auto remove_from(std::vector<handler_entry>& list, std::uint32_t id) -> void {
      list.erase(std::remove_if(list.begin(), list.end(), [id](const handler_entry& entry) { return entry.id == id; }), list.end());
    }

    std::mutex g_log_mutex;
    std::vector<entry> g_entries;
    std::uint32_t g_next_log_id = 1;

    FILE* g_log_file = nullptr;
    char g_active_log_path[260]{};

    auto cfg() -> control& {
      return control_panel();
    }

    auto close_log_file() -> void {
      if (g_log_file) {
        std::fclose(g_log_file);
        g_log_file = nullptr;
      }
      g_active_log_path[0] = '\0';
    }

    auto append_file(const entry& item) -> void {
      const auto& control = cfg();
      if (!control.log_to_file || control.file_path[0] == '\0') {
        close_log_file();
        return;
      }

      if (!g_log_file || std::strcmp(g_active_log_path, control.file_path) != 0) {
        close_log_file();
        if (fopen_s(&g_log_file, control.file_path, "a") != 0 || !g_log_file) {
          return;
        }
        std::strncpy(g_active_log_path, control.file_path, sizeof(g_active_log_path) - 1);
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
      std::fflush(g_log_file);
    }

    auto push_entry(packet_direction direction,
                    packet_layer layer,
                    std::uint16_t opcode,
                    std::vector<std::uint8_t> payload,
                    bool blocked,
                    bool modified,
                    const char* capture_point) -> void {
      const auto& control = cfg();
      if (!control.enabled || control.pause_capture) {
        return;
      }

      if (direction == packet_direction::client_to_server && !control.log_outgoing) {
        return;
      }
      if (direction == packet_direction::server_to_client && !control.log_incoming) {
        return;
      }
      if (layer == packet_layer::cmsg && !control.capture_cmsg) {
        return;
      }
      if (layer == packet_layer::stream && !control.capture_stream) {
        return;
      }

      entry item{};
      item.id = g_next_log_id++;
      item.tick = GetTickCount();
      item.timestamp_ms = static_cast<std::uint64_t>(GetTickCount());
      item.direction = direction;
      item.layer = layer;
      item.opcode = opcode;
      item.payload = std::move(payload);
      item.payload_size = static_cast<std::uint16_t>(item.payload.size());
      item.capture_point = capture_point;
      item.blocked = blocked;
      item.modified = modified;

      append_file(item);

      {
        std::lock_guard lock(g_log_mutex);
        g_entries.push_back(std::move(item));
        if (g_entries.size() > control.max_entries) {
          const auto overflow = g_entries.size() - control.max_entries;
          g_entries.erase(g_entries.begin(), g_entries.begin() + static_cast<std::ptrdiff_t>(overflow));
        }
      }

      append_file(item);
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

    // Example: log every incoming 0x5000-series auth packet without blocking the game.
    auto register_example_handlers() -> void {
      on_incoming_any([](packet_view& ctx) -> handler_result {
        if (ctx.opcode >= 0x5000 && ctx.opcode <= 0x50FF) {
          log_msg("[net_manager] auth S->C opcode=0x%04X len=%zu", ctx.opcode, ctx.msg->payload_size());
        }
        return handler_result::pass;
      });
    }

  } // namespace

  auto on_incoming(std::uint16_t opcode, handler_fn handler) -> handler_token {
    return add_handler(g_incoming, opcode, false, std::move(handler));
  }

  auto on_incoming_any(handler_fn handler) -> handler_token {
    return add_handler(g_incoming, 0, true, std::move(handler));
  }

  auto on_outgoing(std::uint16_t opcode, handler_fn handler) -> handler_token {
    return add_handler(g_outgoing, opcode, false, std::move(handler));
  }

  auto on_outgoing_any(handler_fn handler) -> handler_token {
    return add_handler(g_outgoing, 0, true, std::move(handler));
  }

  auto remove(handler_token token) -> void {
    if (!token.valid()) {
      return;
    }
    std::lock_guard lock(g_mutex);
    remove_from(g_incoming, token.id);
    remove_from(g_outgoing, token.id);
  }

  auto clear_handlers() -> void {
    std::lock_guard lock(g_mutex);
    g_incoming.clear();
    g_outgoing.clear();
  }

  auto shutdown() -> void {
    clear_handlers();
    std::lock_guard lock(g_log_mutex);
    close_log_file();
  }

  auto dispatch_incoming(cmsg_stream_buffer* msg) -> handler_result {
    if (!msg) {
      return handler_result::pass;
    }

    packet_view ctx{};
    ctx.msg = msg;
    ctx.direction = packet_direction::server_to_client;
    ctx.opcode = msg->opcode();

    std::lock_guard lock(g_mutex);
    return invoke_chain(g_incoming, ctx.opcode, ctx);
  }

  auto dispatch_outgoing(cmsg_stream_buffer* msg) -> handler_result {
    if (!msg) {
      return handler_result::pass;
    }

    packet_view ctx{};
    ctx.msg = msg;
    ctx.direction = packet_direction::client_to_server;
    ctx.opcode = msg->opcode();

    std::lock_guard lock(g_mutex);
    return invoke_chain(g_outgoing, ctx.opcode, ctx);
  }

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

  auto control_panel() -> control& {
    return ext_client::config::data().net_manager;
  }

  auto clear_log() -> void {
    std::lock_guard lock(g_log_mutex);
    g_entries.clear();
  }

  auto snapshot() -> std::vector<entry> {
    std::lock_guard lock(g_log_mutex);
    return g_entries;
  }

  auto count() -> std::size_t {
    std::lock_guard lock(g_log_mutex);
    return g_entries.size();
  }

  auto should_block_opcode(std::uint16_t opcode) -> bool {
    const auto& control = cfg();
    switch (static_cast<opcode_block_mode>(control.block_opcode_mode)) {
      case opcode_block_mode::single:
        return opcode == control.block_opcode;
      case opcode_block_mode::list:
        return opcode_in_list(opcode, control.block_opcode_list);
      default:
        return false;
    }
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

  auto record_outgoing(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!msg) {
      return;
    }
    push_entry(packet_direction::client_to_server,
               packet_layer::stream,
               resolve_stream_opcode(msg, opcode),
               msg->extract_payload(),
               blocked,
               modified,
               capture_point);
  }

  auto record_incoming(cmsg_stream_buffer* msg, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!msg) {
      return;
    }
    push_entry(packet_direction::server_to_client,
               packet_layer::stream,
               resolve_stream_opcode(msg, opcode),
               msg->extract_payload(),
               blocked,
               modified,
               capture_point);
  }

  auto record_cmsg_outgoing(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!pkt) {
      return;
    }
    push_entry(packet_direction::client_to_server,
               packet_layer::cmsg,
               resolve_cmsg_opcode(pkt, opcode),
               pkt->extract_payload(),
               blocked,
               modified,
               capture_point);
  }

  auto record_cmsg_incoming(cmsg* pkt, bool blocked, bool modified, std::uint16_t opcode, const char* capture_point) -> void {
    if (!pkt) {
      return;
    }
    push_entry(packet_direction::server_to_client,
               packet_layer::cmsg,
               resolve_cmsg_opcode(pkt, opcode),
               pkt->extract_payload(),
               blocked,
               modified,
               capture_point);
  }

  auto format_opcode(std::uint16_t opcode) -> std::string {
    char buffer[16]{};
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

  auto register_handlers() -> void {
    register_example_handlers();
  }

} // namespace ext_client::net_manager
