#include "plugins/net_log_plugin.hpp"

#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "core/core_config.hpp"
#include "core/core_main.hpp"
#include "sdk/cmsg.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "utils/imgui_helpers.hpp"
#include "utils/log.hpp"

#include <Windows.h>
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::net_log {

  // =========================================================================
  // 1. Types & Enums
  // =========================================================================
  enum class handler_result : std::uint8_t {
    pass,
    consume,
  };

  enum class opcode_block_mode : std::uint8_t {
    off = 0,
    single = 1,
    list = 2,
  };

  using handler_fn = std::function<handler_result(packet_context& ctx)>;

  struct handler_token {
    std::uint32_t id = 0;
    auto valid() const -> bool { return id != 0; }
  };

  struct read_chunk {
    std::size_t offset;
    std::vector<std::uint8_t> bytes;
  };

  struct log_entry {
    std::uint32_t id = 0;
    std::uint32_t tick = 0;
    std::uint64_t timestamp_ms = 0;
    ext_client::packet_direction direction = ext_client::packet_direction::server_to_client;
    packet_layer layer = packet_layer::stream;
    std::uint16_t opcode = 0;
    std::vector<std::uint8_t> payload;
    std::uint16_t payload_size = 0;
    const char* capture_point = nullptr;
    bool blocked = false;
    bool modified = false;
    std::vector<read_chunk> reads;
  };

  struct override_rule {
    std::uint16_t opcode = 0;
    bool apply_all = false;
    std::vector<std::uint8_t> payload;
  };

  using control = ext_client::core::config::core_config::net_config_t;

  // =========================================================================
  // 2. Globals
  // =========================================================================
  namespace {

    // Handler registry
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

    // Log entries
    std::mutex g_log_mutex;
    std::vector<log_entry> g_entries;
    std::uint32_t g_next_log_id = 1;

    FILE* g_log_file = nullptr;
    char g_active_log_path[260]{};

    // Overrides
    std::mutex g_override_mutex;
    override_rule g_outgoing_override{};

    // =========================================================================
    // 3. Control Panel Helper
    // =========================================================================
    auto control_panel() -> control& {
      return ext_client::core::config::data().net;
    }

    auto should_capture(ext_client::packet_direction direction, packet_layer layer) -> bool {
      const auto& cfg = control_panel();
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

    // =========================================================================
    // 4. Opcode Utilities
    // =========================================================================
    auto format_opcode(std::uint16_t opcode) -> const char* {
      thread_local char buffer[16]{};
      std::snprintf(buffer, sizeof(buffer), "0x%04X", opcode);
      return buffer;
    }

    auto format_layer(packet_layer layer) -> const char* {
      return layer == packet_layer::cmsg ? "CMsg" : "stream";
    }

    auto format_hex(const std::vector<std::uint8_t>& bytes, std::size_t bytes_per_row = 16) -> std::string {
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

    auto format_hex_line(const std::uint8_t* data, std::size_t len) -> std::pair<std::string, std::string> {
      std::string hex, ascii;
      hex.reserve(len * 3);
      ascii.reserve(len);
      for (std::size_t i = 0; i < len; ++i) {
        if (i > 0) hex.push_back(' ');
        char pair[4]{};
        std::snprintf(pair, sizeof(pair), "%02X", data[i]);
        hex.append(pair);
        ascii.push_back((data[i] >= 0x20 && data[i] < 0x7F) ? static_cast<char>(data[i]) : '.');
      }
      return {hex, ascii};
    }

    auto render_hex_ascii(const std::uint8_t* data, std::size_t len, std::size_t hex_pad = 0) -> void {
      auto [hex, ascii] = format_hex_line(data, len);
      if (hex_pad > hex.length()) {
        hex.append(hex_pad - hex.length(), ' ');
      }
      ImGui::TextUnformatted(hex.c_str());
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
      ImGui::TextUnformatted(ascii.c_str());
      ImGui::PopStyleColor();
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

    // =========================================================================
    // 5. Handler Dispatch
    // =========================================================================
    auto invoke_chain(const std::vector<handler_entry>& entries, packet_context& ctx) -> handler_result {
      for (const auto& entry : entries) {
        if (!entry.wildcard && entry.opcode != ctx.opcode) {
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

    auto dispatch_packet(packet_context& ctx) -> handler_result {
      std::lock_guard lock(g_handler_mutex);
      const auto& handlers = ctx.direction == ext_client::packet_direction::server_to_client
        ? g_incoming_handlers : g_outgoing_handlers;
      return invoke_chain(handlers, ctx);
    }

    // =========================================================================
    // 6. Log File & Entry Recording
    // =========================================================================
    auto close_log_file() -> void {
      if (g_log_file) {
        std::fclose(g_log_file);
        g_log_file = nullptr;
      }
      g_active_log_path[0] = '\0';
    }

    auto append_file(const log_entry& item) -> void {
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
                   item.direction == ext_client::packet_direction::client_to_server ? "C->S" : "S->C",
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

    auto push_entry(ext_client::packet_direction direction,
                    packet_layer layer,
                    std::uint16_t opcode,
                    std::vector<std::uint8_t> payload,
                    bool blocked,
                    bool modified,
                    const char* capture_point) -> void {
      const auto& cfg = control_panel();
      log_entry item{};
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
        if (g_entries.size() > static_cast<std::size_t>(cfg.max_entries)) {
          const auto overflow = g_entries.size() - static_cast<std::size_t>(cfg.max_entries);
          g_entries.erase(g_entries.begin(), g_entries.begin() + static_cast<std::ptrdiff_t>(overflow));
        }
      }
    }

    auto extract_payload(const packet_context& ctx) -> std::vector<std::uint8_t> {
      if (ctx.layer == packet_layer::stream && ctx.stream_msg) {
        return ctx.stream_msg->extract_payload();
      }
      if (ctx.layer == packet_layer::cmsg && ctx.cmsg_msg) {
        return ctx.cmsg_msg->extract_payload();
      }
      return {};
    }

    auto record_packet(packet_context& ctx, bool blocked, bool modified) -> void {
      if (!should_capture(ctx.direction, ctx.layer)) {
        return;
      }
      push_entry(ctx.direction,
                 ctx.layer,
                 ctx.opcode,
                 extract_payload(ctx),
                 blocked,
                 modified,
                 ctx.capture_point);
    }

    // =========================================================================
    // 7. Plugin Helpers
    // =========================================================================
    auto debug_log(const char* fmt, ...) -> void {
      if (control_panel().log_events) {
        va_list args;
        va_start(args, fmt);
        char buf[512];
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        log_msg("%s", buf);
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

    auto capture_packet(packet_context& ctx, bool blocked, bool modified) -> void {
      const auto dir = ctx.direction == ext_client::packet_direction::client_to_server ? "C->S" : "S->C";
      record_packet(ctx, blocked, modified);
      if (ctx.layer == packet_layer::stream) {
        const auto size = ctx.stream_msg ? ctx.stream_msg->payload_size() : 0;
        debug_log("[net] stream %s opcode=%s len=%zu%s%s",
                  dir, format_opcode(ctx.opcode), size,
                  blocked ? " blocked" : "",
                  modified ? " modified" : "");
      } else {
        const auto size = ctx.cmsg_msg ? ctx.cmsg_msg->body_size() : 0;
        if (ctx.direction == ext_client::packet_direction::server_to_client) {
          debug_log("[net] CMsg %s opcode=%s len=%zu encrypted=%s",
                    dir, format_opcode(ctx.opcode), size,
                    ctx.cmsg_msg && ctx.cmsg_msg->is_encrypted() ? "yes" : "no");
        } else {
          debug_log("[net] CMsg %s opcode=%s len=%zu%s%s",
                    dir, format_opcode(ctx.opcode), size,
                    blocked ? " blocked" : "",
                    modified ? " modified" : "");
        }
      }
    }

    auto should_block(ext_client::packet_direction direction, std::uint16_t opcode) -> bool {
      const auto& cfg = control_panel();
      if (direction == ext_client::packet_direction::client_to_server && cfg.block_outgoing) {
        return true;
      }
      if (direction == ext_client::packet_direction::server_to_client && cfg.block_incoming) {
        return true;
      }
      return should_block_opcode(opcode);
    }

    auto process_outgoing_stream(packet_context& ctx, bool& blocked, bool& modified) -> void {
      if (!ctx.stream_msg) {
        return;
      }
      if (dispatch_packet(ctx) == handler_result::consume) {
        blocked = true;
      }
      if (control_panel().enabled) {
        std::lock_guard lock(g_override_mutex);
        if (control_panel().edit_outgoing) {
          modified = apply_override(ctx.stream_msg, g_outgoing_override) || modified;
        }
      }
      blocked = blocked || should_block(ext_client::packet_direction::client_to_server, ctx.opcode);
    }

    auto clear_log() -> void {
      std::lock_guard lock(g_log_mutex);
      g_entries.clear();
    }

  } // namespace

  // =========================================================================
  // 4. Event Handler Implementation
  // =========================================================================
  auto handle_packet(void* raw_ctx) -> void {
    auto* ctx = static_cast<packet_context*>(raw_ctx);
    if (!ctx) {
      return;
    }

    // Outgoing stream: apply override + dispatch + block check
    if (ctx->direction == ext_client::packet_direction::client_to_server && ctx->layer == packet_layer::stream) {
      bool blocked = false;
      bool modified = false;
      process_outgoing_stream(*ctx, blocked, modified);
      capture_packet(*ctx, blocked, modified);
      if (blocked) {
        ctx->blocked = true;
      }
      return;
    }

    // Dispatch to registered handlers
    if (dispatch_packet(*ctx) == handler_result::consume) {
      debug_log("[net_log_plugin] consumed by handler opcode=%s", format_opcode(ctx->opcode));
      capture_packet(*ctx, true, false);
      ctx->blocked = true;
      ctx->result = ctx->direction == ext_client::packet_direction::server_to_client ? 1 : 0;
      return;
    }

    // Block check
    if (control_panel().enabled && should_block(ctx->direction, ctx->opcode)) {
      debug_log("[net_log_plugin] dropped opcode=%s", format_opcode(ctx->opcode));
      capture_packet(*ctx, true, false);
      ctx->blocked = true;
      ctx->result = ctx->direction == ext_client::packet_direction::server_to_client ? 1 : 0;
      return;
    }

    // Normal capture
    capture_packet(*ctx, false, false);
  }

  auto handle_menu(void* raw_ctx) -> void {
    auto* ctx = static_cast<menu_draw_context*>(raw_ctx);
    if (!ctx || !ctx->menu_visible)
      return;

    if (ImGui::BeginTabItem("Network Logs")) {
      auto& net_cfg = ext_client::core::config::data().net;

      // --- Settings (collapsible) ---
      if (ImGui::CollapsingHeader("Settings")) {
        ext_client::utils::imgui_helpers::checkbox_dirty("Enable", &net_cfg.enabled);
        ImGui::SameLine();
        ext_client::utils::imgui_helpers::checkbox_dirty("Pause", &net_cfg.pause_capture);
        ImGui::SameLine();
        ext_client::utils::imgui_helpers::checkbox_dirty("Incoming", &net_cfg.log_incoming);
        ImGui::SameLine();
        ext_client::utils::imgui_helpers::checkbox_dirty("Outgoing", &net_cfg.log_outgoing);

        ImGui::Spacing();

        ext_client::utils::imgui_helpers::checkbox_dirty("Filter Opcode", &net_cfg.filter_enabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputInt("##filter_opcode", &net_cfg.filter_opcode, 0, 0)) {
          if (net_cfg.filter_opcode < 0) net_cfg.filter_opcode = 0;
          if (net_cfg.filter_opcode > 0xFFFF) net_cfg.filter_opcode = 0xFFFF;
          ext_client::core::config::mark_dirty();
        }
        ImGui::SameLine();
        const char* dir_items[] = {"All", "Incoming", "Outgoing"};
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::Combo("Direction", &net_cfg.filter_direction, dir_items, 3)) {
          ext_client::core::config::mark_dirty();
        }

        ImGui::Spacing();

        if (ImGui::Button("Clear Logs")) {
          clear_log();
        }
        ImGui::SameLine();
        ext_client::utils::imgui_helpers::checkbox_dirty("Show Raw Hex", &net_cfg.show_raw_hex);
        ImGui::SameLine();
        ext_client::utils::imgui_helpers::checkbox_dirty("Show Parsed", &net_cfg.show_parsed);
      }

      ImGui::Spacing();

      // --- Fetch data ---
      std::vector<log_entry> packets;
      {
        std::lock_guard lock(g_log_mutex);
        packets = g_entries;
      }

      // Apply filters — natural order (oldest first), stable selection
      std::vector<int> filtered_indices;
      for (int i = 0; i < static_cast<int>(packets.size()); ++i) {
        const auto& pkt = packets[static_cast<std::size_t>(i)];
        if (net_cfg.filter_enabled && net_cfg.filter_opcode != 0) {
          if (pkt.opcode != static_cast<std::uint16_t>(net_cfg.filter_opcode)) {
            continue;
          }
        }
        if (net_cfg.filter_direction == 1 && pkt.direction != ext_client::packet_direction::server_to_client) {
          continue;
        }
        if (net_cfg.filter_direction == 2 && pkt.direction != ext_client::packet_direction::client_to_server) {
          continue;
        }
        filtered_indices.push_back(i);
      }

      static int s_selected = -1;

      // --- Top pane: packet list ---
      ImGui::TextColored(ImVec4(0.35f, 0.72f, 0.92f, 1.0f), "Packets (%d)", static_cast<int>(filtered_indices.size()));
      ImGui::Separator();

      float top_height = ImGui::GetContentRegionAvail().y * 0.45f;
      if (top_height < 100.0f) top_height = 100.0f;

      ImGui::BeginChild("packet_list", ImVec2(0, top_height), true);
      if (ImGui::BeginTable("packets_table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Tick", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Dir", ImGuiTableColumnFlags_WidthFixed, 45.0f);
        ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Opcode", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableHeadersRow();

        for (int vis_idx = 0; vis_idx < static_cast<int>(filtered_indices.size()); ++vis_idx) {
          const auto& pkt = packets[static_cast<std::size_t>(filtered_indices[static_cast<std::size_t>(vis_idx)])];
          ImGui::TableNextRow();

          // Highlight selected row
          if (s_selected == vis_idx) {
            ImU32 row_bg = ImGui::GetColorU32(ImGuiCol_Header);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg);
          }

          // Full-row invisible selectable for click detection
          char label[32];
          std::snprintf(label, sizeof(label), "##row_%d", vis_idx);
          ImGui::TableSetColumnIndex(0);
          if (ImGui::Selectable(label, s_selected == vis_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
            s_selected = vis_idx;
          }
          ImGui::SameLine();
          ImGui::Text("%u", pkt.tick);

          ImGui::TableSetColumnIndex(1);
          ImGui::TextColored(pkt.direction == ext_client::packet_direction::client_to_server ? ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : ImVec4(0.4f, 1.0f, 0.6f, 1.0f),
                             pkt.direction == ext_client::packet_direction::client_to_server ? "C->S" : "S->C");
          ImGui::TableSetColumnIndex(2);
          ImGui::TextUnformatted(format_layer(pkt.layer));
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("0x%04X", pkt.opcode);
          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%u", pkt.payload_size);
        }
        ImGui::EndTable();
      }
      ImGui::EndChild();

      ImGui::Spacing();

      // --- Bottom pane: detail view (split: raw hex | parsed) ---
      float bottom_height = ImGui::GetContentRegionAvail().y;
      if (bottom_height < 80.0f) bottom_height = 80.0f;

      if (s_selected >= 0 && s_selected < static_cast<int>(filtered_indices.size())) {
        const auto& pkt = packets[static_cast<std::size_t>(filtered_indices[static_cast<std::size_t>(s_selected)])];

        float half_width = ImGui::GetContentRegionAvail().x * 0.5f - 4.0f;

        // Left: raw hex
        if (net_cfg.show_raw_hex) {
          ImGui::BeginChild("raw_hex", ImVec2(half_width, bottom_height), true);
          ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Raw Hex (0x%04X)", pkt.opcode);
          ImGui::Separator();
          if (pkt.payload.empty()) {
            ImGui::TextDisabled("(empty)");
          } else {
            const std::size_t row_size = 16;
            for (std::size_t offset = 0; offset < pkt.payload.size(); offset += row_size) {
              const auto remaining = pkt.payload.size() - offset;
              const auto len = remaining < row_size ? remaining : row_size;
              render_hex_ascii(pkt.payload.data() + offset, len);
            }
          }
          ImGui::EndChild();
          ImGui::SameLine();
        }

        // Right: auto-parse (edxloader style)
        if (net_cfg.show_parsed) {
          float parse_width = net_cfg.show_raw_hex ? half_width : ImGui::GetContentRegionAvail().x;
          ImGui::BeginChild("auto_parse", ImVec2(parse_width, bottom_height), true);
          ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "Auto-Parse [%s][%04X]",
                             pkt.direction == ext_client::packet_direction::client_to_server ? "C -> S" : "S -> C",
                             pkt.opcode);
          ImGui::Separator();
          if (!pkt.reads.empty()) {
            for (const auto& chunk : pkt.reads) {
              render_hex_ascii(chunk.bytes.data(), chunk.bytes.size(), 48);
            }
          } else {
            ImGui::TextDisabled("(no parsed data — incoming CMsg only)");
          }
          ImGui::EndChild();
        }
      } else {
        ImGui::TextDisabled("Select a packet to view details");
      }

      ImGui::EndTabItem();
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("net_log", "Network Logger", "Captures, logs, blocks, and overrides client network packets.");

    ADD_PLUGIN_EVENT("net_log", event_type::on_packet, handle_packet);
    ADD_PLUGIN_EVENT("net_log", event_type::on_menu, handle_menu);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::net_log
