#include "pch.hpp"
#include "plugins/net_log/net_log_capture.hpp"

#include "core/core_config.hpp"
#include "sdk/net/cmsg.hpp"
#include "sdk/net/cmsg_stream_buffer.hpp"
#include "sdk/net/msg_define.hpp"
#include "utils/log.hpp"

#include <Windows.h>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <utility>
#include <deque>

using ext_client::core::event::packet_context;
using ext_client::core::event::packet_layer;
using ext_client::utils::log_msg;

namespace ext_client::plugins::net_log {

  std::mutex g_override_mutex;
  override_rule g_outgoing_override{};
  override_rule g_incoming_override{};

  std::mutex g_log_mutex;
  log_entry g_ring[k_log_ring_capacity]{};
  std::size_t g_ring_head = 0;
  std::size_t g_ring_count = 0;
  std::uint32_t g_next_log_id = 1;
  std::uint64_t g_log_revision = 1;
  std::uint64_t g_log_epoch = 1;
  namespace {
    std::mutex g_file_queue_mutex;
    struct file_record {
      log_entry entry;
      std::string path;
    };
    std::deque<file_record> g_file_queue;
    std::size_t g_file_dropped = 0;
  } // namespace

  FILE *g_log_file = nullptr;
  char g_active_log_path[260]{};
  std::size_t g_unflushed_writes = 0;

  auto should_capture(ext_client::packet_direction direction, packet_layer layer) -> bool {
    const auto settings = ext_client::core::config::runtime();
    const auto &cfg = settings->net;
    if (!cfg.enabled || cfg.pause_capture) {
      return false;
    }
    if (direction == ext_client::packet_direction::client_to_server && !cfg.log_outgoing) {
      return false;
    }
    if (direction == ext_client::packet_direction::server_to_client && !cfg.log_incoming) {
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

  auto format_opcode(std::uint16_t opcode) -> const char * {
    thread_local char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "0x%04X", opcode);
    return buffer;
  }

  auto opcode_display_name(std::uint16_t opcode) -> const char * {
    if (const char *name = ext_client::net::msg::name(opcode)) {
      return name;
    }
    return format_opcode(opcode);
  }

  auto format_layer(packet_layer layer) -> const char * {
    switch (layer) {
    case packet_layer::cmsg:
      return "CMsg";
    case packet_layer::stream:
      return "stream";
    default:
      return "unknown";
    }
  }

  auto parse_opcode_token(const char *&cursor, std::uint16_t &opcode) -> bool {
    while (*cursor != '\0' && (std::isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',')) {
      ++cursor;
    }
    if (*cursor == '\0') {
      return false;
    }
    char *end = nullptr;
    const unsigned long value = std::strtoul(cursor, &end, 16);
    if (end == cursor || value > 0xFFFFu) {
      return false;
    }
    opcode = static_cast<std::uint16_t>(value);
    cursor = end;
    return true;
  }

  auto parse_opcode_list(const char *text, std::vector<std::uint16_t> &out) -> bool {
    out.clear();
    if (!text || text[0] == '\0') {
      return true;
    }
    const char *cursor = text;
    const char *start = text;
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

  auto opcode_in_list(std::uint16_t opcode, const char *list_text) -> bool {
    if (!list_text || list_text[0] == '\0') {
      return false;
    }
    const char *cursor = list_text;
    std::uint16_t parsed = 0;
    while (parse_opcode_token(cursor, parsed)) {
      if (parsed == opcode) {
        return true;
      }
    }
    return false;
  }

  auto should_block_opcode(std::uint16_t opcode) -> bool {
    const auto settings = ext_client::core::config::runtime();
    const auto &cfg = settings->net;
    switch (static_cast<opcode_block_mode>(cfg.block_opcode_mode)) {
    case opcode_block_mode::single:
      return opcode == cfg.block_opcode;
    case opcode_block_mode::list:
      return opcode_in_list(opcode, cfg.block_opcode_list);
    default:
      return false;
    }
  }

  auto parse_hex(const char *text, std::vector<std::uint8_t> &out) -> bool {
    out.clear();
    if (!text) {
      return true;
    }
    int value = -1;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
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

  auto flush_log_file() -> void {
    if (g_log_file && g_unflushed_writes > 0) {
      std::fflush(g_log_file);
      g_unflushed_writes = 0;
    }
  }

  auto close_log_file() -> void {
    if (g_log_file) {
      flush_log_file();
      std::fclose(g_log_file);
      g_log_file = nullptr;
    }
    g_active_log_path[0] = '\0';
    g_unflushed_writes = 0;
  }

  auto append_file_record(const file_record &record) -> void {
    const auto &item = record.entry;
    const char *file_path = record.path.c_str();
    if (!g_log_file || std::strcmp(g_active_log_path, file_path) != 0) {
      close_log_file();
      if (fopen_s(&g_log_file, file_path, "a") != 0 || !g_log_file) {
        return;
      }
      std::strncpy(g_active_log_path, file_path, sizeof(g_active_log_path) - 1);
      g_active_log_path[sizeof(g_active_log_path) - 1] = '\0';
    }
    std::fprintf(g_log_file, "%s %s 0x%04X  len=%u",
                 item.direction == ext_client::packet_direction::client_to_server ? "C->S" : "S->C",
                 format_layer(item.layer), item.opcode, item.payload_size);
    if (item.has_wire_header) {
      std::fprintf(g_log_file, " header_size=0x%04X payload=%u massive=%s sec=%02X crc=%02X", item.header_size_raw,
                   item.header_payload_size, item.massive ? "yes" : "no", item.security_count, item.security_crc);
    }
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
    ++g_unflushed_writes;
    if (g_unflushed_writes >= k_file_flush_every) {
      flush_log_file();
    }
  }

  auto append_file(const log_entry &item) -> void {
    const auto settings = ext_client::core::config::runtime();
    const auto &cfg = settings->net;
    if (!cfg.log_to_file || cfg.file_path[0] == '\0')
      return;
    std::lock_guard lock(g_file_queue_mutex);
    if (g_file_queue.size() >= 1024) {
      ++g_file_dropped;
      return;
    }
    g_file_queue.push_back(file_record{item, cfg.file_path});
  }

  auto flush_pending_file() -> void {
    std::deque<file_record> pending;
    std::size_t dropped;
    {
      std::lock_guard lock(g_file_queue_mutex);
      pending.swap(g_file_queue);
      dropped = std::exchange(g_file_dropped, 0);
    }
    for (const auto &record : pending)
      append_file_record(record);
    flush_log_file();
    if (!ext_client::core::config::runtime()->net.log_to_file)
      close_log_file();
    if (dropped)
      log_msg("[net_log] file queue dropped %zu records", dropped);
  }

  auto handle_shutdown() -> void {
    flush_pending_file();
    close_log_file();
  }

  auto ring_keep_limit() -> std::size_t {
    const auto settings = ext_client::core::config::runtime();
    const auto &cfg = settings->net;
    if (cfg.max_entries > 0 && static_cast<std::size_t>(cfg.max_entries) < k_log_ring_capacity) {
      return static_cast<std::size_t>(cfg.max_entries);
    }
    return k_log_ring_capacity;
  }

  auto push_entry(ext_client::packet_direction direction, packet_layer layer, std::uint16_t opcode,
                  std::vector<std::uint8_t> payload, const packet_context &ctx, bool blocked, bool modified,
                  const char *capture_point) -> void {
    log_entry item{};
    item.tick = GetTickCount();
    item.timestamp_ms = static_cast<std::uint64_t>(GetTickCount());
    item.direction = direction;
    item.layer = layer;
    item.opcode = opcode;

    const std::size_t full_size = ctx.layer == packet_layer::stream && ctx.stream_msg
                                      ? ctx.stream_msg->get_payload_size()
                                      : (ctx.cmsg_msg ? ctx.cmsg_msg->body_size() : payload.size());
    item.payload_size = static_cast<std::uint16_t>(full_size > 0xFFFFu ? 0xFFFFu : full_size);
    if (payload.size() > k_max_payload_store) {
      payload.resize(k_max_payload_store);
    }
    item.payload = std::move(payload);

    if (layer == packet_layer::cmsg && ctx.cmsg_msg) {
      item.has_wire_header = true;
      item.header_size_raw = ctx.cmsg_msg->header_size_raw();
      item.header_payload_size = static_cast<std::uint16_t>(ctx.cmsg_msg->body_size());
      item.massive = ctx.cmsg_msg->is_massive();
      item.security_count = ctx.cmsg_msg->security_count();
      item.security_crc = ctx.cmsg_msg->security_crc();
    }
    item.capture_point = capture_point;
    item.blocked = blocked;
    item.modified = modified;
    item.has_parsed = false;

    {
      std::lock_guard lock(g_log_mutex);
      item.id = g_next_log_id++;
      append_file(item);
      ++g_log_revision;
      g_ring[g_ring_head] = std::move(item);
      g_ring_head = (g_ring_head + 1) % k_log_ring_capacity;
      if (g_ring_count < k_log_ring_capacity) {
        ++g_ring_count;
      }
      const std::size_t keep = ring_keep_limit();
      if (g_ring_count > keep) {
        g_ring_count = keep;
      }
    }
  }

  auto extract_payload(const packet_context &ctx) -> std::vector<std::uint8_t> {
    if (ctx.layer == packet_layer::stream && ctx.stream_msg) {
      return ctx.stream_msg->extract_payload(k_max_payload_store);
    }
    if (ctx.layer == packet_layer::cmsg && ctx.cmsg_msg) {
      return ctx.cmsg_msg->extract_payload(k_max_payload_store);
    }
    return {};
  }

  auto record_packet(packet_context &ctx, bool blocked, bool modified) -> void {
    if (!should_capture(ctx.direction, ctx.layer)) {
      return;
    }
    push_entry(ctx.direction, ctx.layer, ctx.opcode, extract_payload(ctx), ctx, blocked, modified, ctx.capture_point);
  }

  auto debug_log(const char *fmt, ...) -> void {
    if (ext_client::core::config::runtime()->net.log_events) {
      va_list args;
      va_start(args, fmt);
      char buf[512];
      std::vsnprintf(buf, sizeof(buf), fmt, args);
      va_end(args);
      log_msg("%s", buf);
    }
  }

  auto apply_override(cmsg_stream_buffer *msg, const override_rule &rule) -> bool {
    if (!msg || rule.payload.empty()) {
      return false;
    }
    const auto opcode = msg->get_opcode();
    if (!rule.apply_all && opcode != rule.opcode) {
      return false;
    }
    return msg->replace_payload(rule.payload.data(), rule.payload.size());
  }

  auto apply_incoming_override(cmsg *msg, const override_rule &rule) -> bool {
    if (!msg || rule.payload.empty()) {
      return false;
    }
    const auto opcode = msg->opcode();
    if (!rule.apply_all && opcode != rule.opcode) {
      return false;
    }
    return msg->write_payload(rule.payload.data(), static_cast<int>(rule.payload.size())) > 0;
  }

  auto capture_packet(packet_context &ctx, bool blocked, bool modified) -> void {
    const auto dir = ctx.direction == ext_client::packet_direction::client_to_server ? "C->S" : "S->C";
    record_packet(ctx, blocked, modified);
    if (ctx.layer == packet_layer::stream) {
      const auto size = ctx.stream_msg ? ctx.stream_msg->get_payload_size() : 0;
      debug_log("[net] stream %s opcode=%s len=%zu%s%s", dir, format_opcode(ctx.opcode), size,
                blocked ? " blocked" : "", modified ? " modified" : "");
    } else {
      const auto size = ctx.cmsg_msg ? ctx.cmsg_msg->body_size() : 0;
      if (ctx.direction == ext_client::packet_direction::server_to_client) {
        debug_log("[net] CMsg %s opcode=%s len=%zu massive=%s sec=%02X crc=%02X", dir, format_opcode(ctx.opcode), size,
                  ctx.cmsg_msg && ctx.cmsg_msg->is_massive() ? "yes" : "no",
                  ctx.cmsg_msg ? ctx.cmsg_msg->security_count() : 0, ctx.cmsg_msg ? ctx.cmsg_msg->security_crc() : 0);
      } else {
        debug_log("[net] CMsg %s opcode=%s len=%zu massive=%s sec=%02X crc=%02X%s%s", dir, format_opcode(ctx.opcode),
                  size, ctx.cmsg_msg && ctx.cmsg_msg->is_massive() ? "yes" : "no",
                  ctx.cmsg_msg ? ctx.cmsg_msg->security_count() : 0, ctx.cmsg_msg ? ctx.cmsg_msg->security_crc() : 0,
                  blocked ? " blocked" : "", modified ? " modified" : "");
      }
    }
  }

  auto should_block(ext_client::packet_direction direction, std::uint16_t opcode) -> bool {
    const auto settings = ext_client::core::config::runtime();
    const auto &cfg = settings->net;
    if (direction == ext_client::packet_direction::client_to_server && cfg.block_outgoing) {
      return true;
    }
    if (direction == ext_client::packet_direction::server_to_client && cfg.block_incoming) {
      return true;
    }
    return should_block_opcode(opcode);
  }

  auto process_incoming_cmsg(packet_context &ctx, bool &blocked, bool &modified) -> void {
    if (!ctx.cmsg_msg) {
      return;
    }
    const auto settings = ext_client::core::config::runtime();
    if (settings->net.enabled && settings->net.edit_incoming) {
      std::lock_guard lock(g_override_mutex);
      modified = apply_incoming_override(ctx.cmsg_msg, g_incoming_override) || modified;
    }
    blocked = blocked || should_block(ext_client::packet_direction::server_to_client, ctx.opcode);
  }

  auto process_outgoing_stream(packet_context &ctx, bool &blocked, bool &modified) -> void {
    if (!ctx.stream_msg) {
      return;
    }
    const auto settings = ext_client::core::config::runtime();
    if (settings->net.enabled && settings->net.edit_outgoing) {
      std::lock_guard lock(g_override_mutex);
      modified = apply_override(ctx.stream_msg, g_outgoing_override) || modified;
    }
    blocked = blocked || should_block(ext_client::packet_direction::client_to_server, ctx.opcode);
  }

  auto clear_log() -> void {
    std::lock_guard lock(g_log_mutex);
    ++g_log_revision;
    ++g_log_epoch;
    g_ring_head = 0;
    g_ring_count = 0;
    for (auto &entry : g_ring) {
      entry = log_entry{};
    }
  }

  auto copy_log_entries(std::vector<log_entry> &out) -> void {
    std::lock_guard lock(g_log_mutex);
    out.clear();
    out.reserve(g_ring_count);
    if (g_ring_count == 0) {
      return;
    }
    const std::size_t start = (g_ring_head + k_log_ring_capacity - g_ring_count) % k_log_ring_capacity;
    for (std::size_t i = 0; i < g_ring_count; ++i) {
      out.push_back(g_ring[(start + i) % k_log_ring_capacity]);
    }
  }

  auto refresh_log_entries(std::vector<log_entry> &out, std::uint64_t &revision, std::uint64_t &epoch) -> bool {
    std::lock_guard lock(g_log_mutex);
    if (revision == g_log_revision)
      return false;
    if (epoch != g_log_epoch)
      out.clear();
    std::vector<log_entry> next;
    next.reserve(g_ring_count);
    const auto start = (g_ring_head + k_log_ring_capacity - g_ring_count) % k_log_ring_capacity;
    std::size_t old = 0;
    for (std::size_t i = 0; i < g_ring_count; ++i) {
      const auto &stored = g_ring[(start + i) % k_log_ring_capacity];
      while (old < out.size() && out[old].id != stored.id)
        ++old;
      if (old < out.size())
        next.push_back(std::move(out[old++]));
      else
        next.push_back(stored);
    }
    out.swap(next);
    revision = g_log_revision;
    epoch = g_log_epoch;
    return true;
  }

  auto replace_log_entries(std::vector<log_entry> entries) -> void {
    std::lock_guard lock(g_log_mutex);
    ++g_log_revision;
    ++g_log_epoch;
    g_ring_head = 0;
    g_ring_count = 0;
    for (auto &entry : g_ring) {
      entry = log_entry{};
    }
    const std::size_t keep = ring_keep_limit();
    const std::size_t begin = entries.size() > keep ? entries.size() - keep : 0;
    for (std::size_t i = begin; i < entries.size(); ++i) {
      g_ring[g_ring_head] = std::move(entries[i]);
      g_ring_head = (g_ring_head + 1) % k_log_ring_capacity;
      if (g_ring_count < k_log_ring_capacity) {
        ++g_ring_count;
      }
    }
    if (g_ring_count > keep) {
      g_ring_count = keep;
    }
    if (g_ring_count > 0) {
      const std::size_t last = (g_ring_head + k_log_ring_capacity - 1) % k_log_ring_capacity;
      g_next_log_id = g_ring[last].id + 1;
    }
  }

  auto handle_packet(packet_context &ctx) -> void {
    if (!ext_client::core::config::runtime()->net.enabled) {
      return;
    }

    if (ctx.direction == ext_client::packet_direction::client_to_server && ctx.layer == packet_layer::stream) {
      bool blocked = false;
      bool modified = false;
      process_outgoing_stream(ctx, blocked, modified);
      capture_packet(ctx, blocked, modified);
      if (blocked) {
        ctx.blocked = true;
      }
      return;
    }

    if (ctx.direction == ext_client::packet_direction::server_to_client && ctx.layer == packet_layer::cmsg) {
      bool blocked = false;
      bool modified = false;
      process_incoming_cmsg(ctx, blocked, modified);
      capture_packet(ctx, blocked, modified);
      if (blocked) {
        ctx.blocked = true;
        ctx.result = 1;
      }
    }
  }

} // namespace ext_client::plugins::net_log
