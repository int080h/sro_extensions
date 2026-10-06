#include "pch.hpp"
#include "plugins/net_log/net_log_ui.hpp"
#include "plugins/net_log/net_log_internal.hpp"
#include "plugins/net_log/packet_doc_db.hpp"
#include "plugins/net_log/packet_parser.hpp"
#include "plugins/net_log/packet_session.hpp"

#include "core/config.hpp"
#include "utils/log.hpp"

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using ext_client::core::event::packet_layer;
using ext_client::utils::log_msg;

namespace ext_client::plugins::net_log {

  namespace {

    auto passes_filter(const log_entry &pkt, const ext_client::core::config::core_config::net_config_t &cfg) -> bool {
      if (cfg.filter_direction == 1 && pkt.direction != ext_client::packet_direction::server_to_client)
        return false;
      if (cfg.filter_direction == 2 && pkt.direction != ext_client::packet_direction::client_to_server)
        return false;

      if (cfg.filter_layer == 1 && pkt.layer != packet_layer::cmsg)
        return false;
      if (cfg.filter_layer == 2 && pkt.layer != packet_layer::stream)
        return false;

      if (cfg.filter_category > 0) {
        const auto target_cat = static_cast<opcode_category>(cfg.filter_category);
        if (pkt.category != target_cat)
          return false;
      }

      if (cfg.filter_search_text[0] != '\0') {
        char hex_code[16];
        std::snprintf(hex_code, sizeof(hex_code), "%04X", pkt.opcode);
        char hex_0x[16];
        std::snprintf(hex_0x, sizeof(hex_0x), "0x%04X", pkt.opcode);

        const char *name = opcode_display_name(pkt.opcode);
        bool matched = false;

        // Compare opcode hex
        if (std::strstr(hex_code, cfg.filter_search_text) != nullptr ||
            std::strstr(hex_0x, cfg.filter_search_text) != nullptr) {
          matched = true;
        } else if (name && std::strstr(name, cfg.filter_search_text) != nullptr) {
          matched = true;
        } else if (!pkt.payload.empty()) {
          // Substring search in ASCII payload text
          std::string payload_ascii;
          payload_ascii.reserve(pkt.payload.size());
          for (auto b : pkt.payload) {
            payload_ascii.push_back((b >= 32 && b < 127) ? static_cast<char>(std::tolower(b)) : ' ');
          }
          std::string query = cfg.filter_search_text;
          std::transform(query.begin(), query.end(), query.begin(),
                         [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
          if (payload_ascii.find(query) != std::string::npos) {
            matched = true;
          }
        }

        if (!matched) return false;
      }

      if (cfg.filter_opcode_mode == 1) {
        if (pkt.opcode != static_cast<std::uint16_t>(cfg.filter_opcode))
          return false;
      } else if (cfg.filter_opcode_mode == 2) {
        if (!opcode_in_list(pkt.opcode, cfg.filter_opcode_list))
          return false;
      } else if (cfg.filter_opcode_mode == 3) {
        const char *name = opcode_display_name(pkt.opcode);
        if (!name || !std::strstr(name, cfg.filter_opcode_name))
          return false;
      }

      if (cfg.filter_massive_only && !pkt.massive)
        return false;
      if (cfg.filter_blocked_only && !pkt.blocked)
        return false;
      if (cfg.filter_modified_only && !pkt.modified)
        return false;

      if (cfg.min_payload_size > 0 && pkt.payload_size < static_cast<std::uint16_t>(cfg.min_payload_size))
        return false;
      if (cfg.max_payload_size > 0 && pkt.payload_size > static_cast<std::uint16_t>(cfg.max_payload_size))
        return false;

      return true;
    }

    auto type_color(pkt::field_type ft) -> ImVec4 {
      switch (ft) {
      case pkt::field_type::u8:
        return ImVec4(0.45f, 0.85f, 0.45f, 1.0f);
      case pkt::field_type::u16:
        return ImVec4(0.45f, 0.85f, 0.65f, 1.0f);
      case pkt::field_type::u32:
        return ImVec4(0.45f, 0.85f, 0.85f, 1.0f);
      case pkt::field_type::u64:
        return ImVec4(0.45f, 0.70f, 1.00f, 1.0f);
      case pkt::field_type::i8:
      case pkt::field_type::i16:
      case pkt::field_type::i32:
      case pkt::field_type::i64:
        return ImVec4(0.85f, 0.60f, 0.60f, 1.0f);
      case pkt::field_type::f32:
        return ImVec4(0.90f, 0.85f, 0.35f, 1.0f);
      case pkt::field_type::bool_:
        return ImVec4(0.70f, 0.65f, 1.00f, 1.0f);
      case pkt::field_type::ascii:
        return ImVec4(1.00f, 0.75f, 0.30f, 1.0f);
      case pkt::field_type::loop:
        return ImVec4(0.75f, 0.50f, 1.00f, 1.0f);
      case pkt::field_type::branch:
        return ImVec4(1.00f, 0.50f, 0.50f, 1.0f);
      default:
        return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
      }
    }

    auto type_name(pkt::field_type ft) -> const char * {
      switch (ft) {
      case pkt::field_type::u8: return "u8";
      case pkt::field_type::u16: return "u16";
      case pkt::field_type::u32: return "u32";
      case pkt::field_type::u64: return "u64";
      case pkt::field_type::i8: return "i8";
      case pkt::field_type::i16: return "i16";
      case pkt::field_type::i32: return "i32";
      case pkt::field_type::i64: return "i64";
      case pkt::field_type::f32: return "f32";
      case pkt::field_type::bool_: return "bool";
      case pkt::field_type::ascii: return "ascii";
      case pkt::field_type::raw: return "raw";
      case pkt::field_type::loop: return "loop";
      case pkt::field_type::branch: return "branch";
      default: return "?";
      }
    }

    auto format_hex_line(const std::uint8_t *data, std::size_t len) -> std::pair<std::string, std::string> {
      std::string hex, ascii;
      hex.reserve(len * 3);
      ascii.reserve(len);
      for (std::size_t i = 0; i < len; ++i) {
        if (i > 0)
          hex.push_back(' ');
        char pair[4]{};
        std::snprintf(pair, sizeof(pair), "%02X", data[i]);
        hex.append(pair);
        ascii.push_back((data[i] >= 0x20 && data[i] < 0x7F) ? static_cast<char>(data[i]) : '.');
      }
      return {hex, ascii};
    }

    auto render_hex_ascii(const std::uint8_t *data, std::size_t len, std::size_t hex_pad = 0) -> void {
      auto [hex, ascii] = format_hex_line(data, len);
      if (hex_pad > hex.length()) {
        hex.append(hex_pad - hex.length(), ' ');
      }
      ImGui::TextUnformatted(hex.c_str());
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
      ImGui::TextUnformatted(ascii.c_str());
      ImGui::PopStyleColor();
    }

    auto copy_hex_clipboard(const std::vector<std::uint8_t> &data) -> void {
      if (data.empty()) return;
      std::string out;
      out.reserve(data.size() * 3);
      for (std::size_t i = 0; i < data.size(); ++i) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X ", data[i]);
        out.append(buf);
      }
      if (!out.empty()) out.pop_back();
      ImGui::SetClipboardText(out.c_str());
    }

    auto copy_cpp_array_clipboard(std::uint16_t opcode, const std::vector<std::uint8_t> &data) -> void {
      char header[256];
      std::snprintf(header, sizeof(header), "// Opcode: 0x%04X (%s), Size: %zu bytes\nconst std::uint8_t packet_0x%04X[] = {\n",
                    opcode, opcode_display_name(opcode), data.size(), opcode);
      std::string out = header;
      for (std::size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) out += "  ";
        char b[8];
        std::snprintf(b, sizeof(b), "0x%02X, ", data[i]);
        out += b;
        if (i % 16 == 15 || i + 1 == data.size()) out += "\n";
      }
      out += "};\n";
      ImGui::SetClipboardText(out.c_str());
    }

    auto copy_ascii_clipboard(const std::vector<std::uint8_t> &data) -> void {
      std::string out;
      out.reserve(data.size());
      for (auto b : data) {
        if (b >= 32 && b < 127) out.push_back(static_cast<char>(b));
      }
      ImGui::SetClipboardText(out.c_str());
    }

    auto ensure_parsed(log_entry &pkt) -> void {
      if (pkt.has_parsed) {
        return;
      }
      if (!pkt.payload.empty()) {
        pkt.parsed = parse_packet(pkt.opcode, pkt.payload.data(), pkt.payload.size());
      }
      pkt.has_parsed = true;
    }

    auto entries_to_session_packets(const std::vector<log_entry> &entries) -> std::vector<session_packet> {
      std::vector<session_packet> out;
      out.reserve(entries.size());
      for (const auto &e : entries) {
        session_packet p{};
        p.id = e.id;
        p.tick = e.tick;
        p.timestamp_ms = e.timestamp_ms;
        p.direction = static_cast<std::uint8_t>(e.direction == ext_client::packet_direction::client_to_server ? 1 : 0);
        p.layer = static_cast<std::uint8_t>(e.layer == packet_layer::stream ? 1 : 0);
        p.opcode = e.opcode;
        p.payload_size = e.payload_size;
        p.has_wire_header = e.has_wire_header;
        p.header_size_raw = e.header_size_raw;
        p.header_payload_size = e.header_payload_size;
        p.massive = e.massive;
        p.security_count = e.security_count;
        p.security_crc = e.security_crc;
        p.blocked = e.blocked;
        p.modified = e.modified;
        p.capture_point = e.capture_point ? e.capture_point : "";
        p.payload = e.payload;
        out.push_back(std::move(p));
      }
      return out;
    }

    auto session_packets_to_entries(const std::vector<session_packet> &packets) -> std::vector<log_entry> {
      std::vector<log_entry> out;
      out.reserve(packets.size());
      for (const auto &p : packets) {
        log_entry e{};
        e.id = p.id;
        e.tick = p.tick;
        e.timestamp_ms = p.timestamp_ms;
        e.direction = p.direction ? ext_client::packet_direction::client_to_server
                                  : ext_client::packet_direction::server_to_client;
        e.layer = p.layer ? packet_layer::stream : packet_layer::cmsg;
        e.opcode = p.opcode;
        e.category = classify_opcode(p.opcode);
        e.payload = p.payload;
        e.payload_size = p.payload_size;
        e.has_wire_header = p.has_wire_header;
        e.header_size_raw = p.header_size_raw;
        e.header_payload_size = p.header_payload_size;
        e.massive = p.massive;
        e.security_count = p.security_count;
        e.security_crc = p.security_crc;
        e.blocked = p.blocked;
        e.modified = p.modified;
        e.capture_point = nullptr;
        e.has_parsed = false;
        out.push_back(std::move(e));
      }
      return out;
    }
  } // namespace

  auto handle_menu(ext_client::render::menu::menu_builder &ui) -> void {
    auto &net_cfg = ext_client::core::config::data().net;

    if (ui.collapsing_header("Capture & Settings")) {
      ui.checkbox("Enable Capture", &net_cfg.enabled);
      ui.same_line();
      ui.checkbox("Pause", &net_cfg.pause_capture);
      ui.same_line();
      ui.checkbox("Incoming (S->C)", &net_cfg.log_incoming);
      ui.same_line();
      ui.checkbox("Outgoing (C->S)", &net_cfg.log_outgoing);
      ui.same_line();
      ui.checkbox("CMsg Layer", &net_cfg.capture_cmsg);
      ui.same_line();
      ui.checkbox("Stream Layer", &net_cfg.capture_stream);

      ui.spacing();

      if (ui.button("Clear Log")) {
        clear_log();
      }
      ui.same_line();
      ui.checkbox("Auto-scroll", &net_cfg.auto_scroll);
      ui.same_line();
      ui.checkbox("Stats Bar", &net_cfg.show_stats_bar);
      ui.same_line();
      ui.checkbox("Timestamps", &net_cfg.show_timestamps);

      ui.spacing();

      ui.set_next_item_width(220.0f);
      ui.input_text("##session_path", net_cfg.session_path, sizeof(net_cfg.session_path));
      ui.same_line();
      if (ui.button("Save Session")) {
        std::vector<log_entry> snapshot;
        copy_log_entries(snapshot);
        auto sp = entries_to_session_packets(snapshot);
        if (!save_session(net_cfg.session_path, sp)) {
          log_msg("[net_log] Failed to save session to %s", net_cfg.session_path);
        }
      }
      ui.same_line();
      if (ui.button("Load Session")) {
        std::vector<session_packet> sp;
        if (load_session(net_cfg.session_path, sp)) {
          replace_log_entries(session_packets_to_entries(sp));
        } else {
          log_msg("[net_log] Failed to load session from %s", net_cfg.session_path);
        }
      }
    }

    ui.spacing();

    if (ui.collapsing_header("Protocol Filters")) {
      const char *category_items[] = {
        "All Categories",
        "Handshake & System",
        "Authentication & Login",
        "Character State",
        "Entity Spawn & World",
        "Movement & Position",
        "Chat & Messaging",
        "Combat & Skills",
        "Inventory & Storage",
        "Social & Guild",
        "Stall & Trade",
        "Environment & Weather",
        "Other"
      };
      ui.set_next_item_width(170.0f);
      ui.combo("Category##filter", &net_cfg.filter_category, category_items, 13);
      ui.same_line();

      const char *dir_items[] = {"All Dir", "S->C", "C->S"};
      ui.set_next_item_width(80.0f);
      ui.combo("Direction##filter", &net_cfg.filter_direction, dir_items, 3);
      ui.same_line();

      const char *layer_items[] = {"All Layers", "CMsg", "Stream"};
      ui.set_next_item_width(85.0f);
      ui.combo("Layer##filter", &net_cfg.filter_layer, layer_items, 3);
      ui.same_line();

      ui.set_next_item_width(180.0f);
      ui.input_text("Search##filter", net_cfg.filter_search_text, sizeof(net_cfg.filter_search_text));

      ui.spacing();

      const char *opmode_items[] = {"Opcode: Off", "Single Opcode", "Opcode List", "Name Filter"};
      ui.set_next_item_width(115.0f);
      ui.combo("##opmode_filter", &net_cfg.filter_opcode_mode, opmode_items, 4);
      ui.same_line();

      if (net_cfg.filter_opcode_mode == 1) {
        ui.set_next_item_width(90.0f);
        if (ui.input_int("##filter_opcode_single", &net_cfg.filter_opcode, 0, 0)) {
          if (net_cfg.filter_opcode < 0) net_cfg.filter_opcode = 0;
          if (net_cfg.filter_opcode > 0xFFFF) net_cfg.filter_opcode = 0xFFFF;
        }
        ui.same_line();
      } else if (net_cfg.filter_opcode_mode == 2) {
        ui.set_next_item_width(160.0f);
        ui.input_text("##filter_opcode_list", net_cfg.filter_opcode_list, sizeof(net_cfg.filter_opcode_list));
        ui.same_line();
      } else if (net_cfg.filter_opcode_mode == 3) {
        ui.set_next_item_width(140.0f);
        ui.input_text("##filter_opcode_name", net_cfg.filter_opcode_name, sizeof(net_cfg.filter_opcode_name));
        ui.same_line();
      }

      ui.checkbox("Massive Only", &net_cfg.filter_massive_only);
      ui.same_line();
      ui.checkbox("Blocked Only", &net_cfg.filter_blocked_only);
      ui.same_line();
      ui.checkbox("Modified Only", &net_cfg.filter_modified_only);
      ui.same_line();

      ui.set_next_item_width(55.0f);
      ui.input_int("Min Sz", &net_cfg.min_payload_size, 0, 0);
      ui.same_line();
      ui.set_next_item_width(55.0f);
      ui.input_int("Max Sz", &net_cfg.max_payload_size, 0, 0);
    }

    ui.spacing();

    static std::vector<log_entry> packets;
    static std::uint64_t snapshot_revision = 0;
    static std::uint64_t snapshot_epoch = 0;
    static std::uint32_t selected_id = 0;
    const auto old_epoch = snapshot_epoch;
    const bool capture_changed = refresh_log_entries(packets, snapshot_revision, snapshot_epoch);
    if (old_epoch != snapshot_epoch)
      selected_id = 0;

    const auto view_key = std::make_tuple(
        net_cfg.filter_direction, net_cfg.filter_layer, net_cfg.filter_opcode_mode, net_cfg.filter_opcode,
        net_cfg.filter_enabled, std::string(net_cfg.filter_opcode_list), std::string(net_cfg.filter_opcode_name),
        net_cfg.filter_category, std::string(net_cfg.filter_search_text),
        net_cfg.filter_massive_only, net_cfg.filter_blocked_only, net_cfg.filter_modified_only,
        net_cfg.min_payload_size, net_cfg.max_payload_size, net_cfg.sort_column, net_cfg.sort_ascending);

    static std::optional<decltype(view_key)> previous_view_key;
    static std::vector<int> filtered_indices;
    if (capture_changed || !previous_view_key || *previous_view_key != view_key) {
      previous_view_key.emplace(view_key);
      filtered_indices.clear();
      filtered_indices.reserve(packets.size());
      for (int i = 0; i < static_cast<int>(packets.size()); ++i) {
        const auto &pkt = packets[static_cast<std::size_t>(i)];
        if (net_cfg.filter_enabled && net_cfg.filter_opcode != 0 && net_cfg.filter_opcode_mode == 0 &&
            pkt.opcode != static_cast<std::uint16_t>(net_cfg.filter_opcode))
          continue;
        if (passes_filter(pkt, net_cfg))
          filtered_indices.push_back(i);
      }
      std::stable_sort(filtered_indices.begin(), filtered_indices.end(), [&](int a, int b) {
        return packet_less(packets[static_cast<std::size_t>(a)], packets[static_cast<std::size_t>(b)],
                           net_cfg.sort_column, net_cfg.sort_ascending);
      });
    }

    if (net_cfg.show_stats_bar) {
      int count_c2s = 0, count_s2c = 0, count_massive = 0, count_blocked = 0, count_modified = 0;
      std::size_t total_bytes = 0;
      for (int idx : filtered_indices) {
        const auto &pkt = packets[static_cast<std::size_t>(idx)];
        if (pkt.direction == ext_client::packet_direction::client_to_server)
          count_c2s++;
        else
          count_s2c++;
        if (pkt.massive)
          count_massive++;
        if (pkt.blocked)
          count_blocked++;
        if (pkt.modified)
          count_modified++;
        total_bytes += pkt.payload.size();
      }
      ImGui::TextColored(ImVec4(0.35f, 0.72f, 0.92f, 1.0f), "Packets: %d", static_cast<int>(filtered_indices.size()));
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "C->S: %d", count_c2s);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "S->C: %d", count_s2c);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "Massive: %d", count_massive);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Blocked: %d", count_blocked);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Modified: %d", count_modified);
      ImGui::SameLine();
      ImGui::TextDisabled("Total Data: %.1f KB", total_bytes / 1024.0f);
      ImGui::Separator();
    }

    float total_height = ImGui::GetContentRegionAvail().y;
    static float s_split_ratio = 0.48f;
    float top_height = total_height * s_split_ratio;
    if (top_height < 80.0f) top_height = 80.0f;
    float bottom_height = total_height - top_height - 6.0f;
    if (bottom_height < 80.0f) bottom_height = 80.0f;

    ImGui::BeginChild("packet_list", ImVec2(0, top_height), true);
    if (ImGui::BeginTable("packets_table", 7,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders |
                              ImGuiTableFlags_Sortable)) {
      ImGui::TableSetupColumn("Tick", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 65.0f);
      ImGui::TableSetupColumn("Dir", ImGuiTableColumnFlags_WidthFixed, 42.0f);
      ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed, 60.0f);
      ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_WidthFixed, 50.0f);
      ImGui::TableSetupColumn("Opcode", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 48.0f);
      ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 55.0f);
      ImGui::TableHeadersRow();

      ImGuiTableSortSpecs *specs = ImGui::TableGetSortSpecs();
      if (specs && specs->SpecsCount > 0) {
        const auto &spec = specs->Specs[0];
        const auto column = static_cast<int>(spec.ColumnIndex);
        const bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;
        if (net_cfg.sort_column != column || net_cfg.sort_ascending != ascending) {
          net_cfg.sort_column = column;
          net_cfg.sort_ascending = ascending;
          ui.note_dirty();
        }
        specs->SpecsDirty = false;
      }

      ImGuiListClipper clipper;
      clipper.Begin(static_cast<int>(filtered_indices.size()));
      while (clipper.Step()) {
        for (int vis_idx = clipper.DisplayStart; vis_idx < clipper.DisplayEnd; ++vis_idx) {
          const auto &pkt = packets[static_cast<std::size_t>(filtered_indices[static_cast<std::size_t>(vis_idx)])];
          ImGui::TableNextRow();

          ImU32 row_bg = 0;
          if (selected_id == pkt.id) {
            row_bg = ImGui::GetColorU32(ImGuiCol_Header);
          } else if (pkt.blocked) {
            row_bg = IM_COL32(80, 30, 30, 80);
          } else if (pkt.modified) {
            row_bg = IM_COL32(80, 60, 20, 80);
          } else if (pkt.massive) {
            row_bg = IM_COL32(50, 30, 85, 70);
          }
          if (row_bg)
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg);

          char label[32];
          std::snprintf(label, sizeof(label), "##row_%d", vis_idx);
          ImGui::TableSetColumnIndex(0);
          if (ImGui::Selectable(label, selected_id == pkt.id,
                                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
            selected_id = pkt.id;
          }

          // Right-click context menu on row
          if (ImGui::BeginPopupContextItem(label)) {
            char op_hex_buf[16];
            std::snprintf(op_hex_buf, sizeof(op_hex_buf), "0x%04X", pkt.opcode);
            if (ImGui::MenuItem("Copy Opcode (Hex)")) {
              ImGui::SetClipboardText(op_hex_buf);
            }
            if (ImGui::MenuItem("Copy Opcode Name")) {
              ImGui::SetClipboardText(opcode_display_name(pkt.opcode));
            }
            if (ImGui::MenuItem("Copy Payload (Hex)")) {
              copy_hex_clipboard(pkt.payload);
            }
            if (ImGui::MenuItem("Copy Payload (C++ Array)")) {
              copy_cpp_array_clipboard(pkt.opcode, pkt.payload);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Filter by this Opcode")) {
              net_cfg.filter_opcode_mode = 1;
              net_cfg.filter_opcode = pkt.opcode;
              ui.note_dirty();
            }
            if (ImGui::MenuItem("Block this Opcode")) {
              net_cfg.block_opcode_mode = 1;
              net_cfg.block_opcode = pkt.opcode;
              ui.note_dirty();
            }
            ImGui::EndPopup();
          }

          ImGui::SameLine();
          if (net_cfg.show_timestamps)
            ImGui::Text("%u", pkt.tick);
          else
            ImGui::TextDisabled("-");

          ImGui::TableSetColumnIndex(1);
          ImGui::TextColored(pkt.direction == ext_client::packet_direction::client_to_server
                                 ? ImVec4(0.4f, 0.8f, 1.0f, 1.0f)
                                 : ImVec4(0.4f, 1.0f, 0.6f, 1.0f),
                             pkt.direction == ext_client::packet_direction::client_to_server ? "C->S" : "S->C");

          ImGui::TableSetColumnIndex(2);
          ImGui::TextColored(ImVec4(0.8f, 0.7f, 1.0f, 1.0f), "%s", opcode_category_badge(pkt.category));

          ImGui::TableSetColumnIndex(3);
          ImGui::TextUnformatted(format_layer(pkt.layer));

          ImGui::TableSetColumnIndex(4);
          const char *opname = opcode_display_name(pkt.opcode);
          ImGui::TextColored(ImVec4(0.85f, 0.88f, 1.0f, 1.0f), "0x%04X %s", pkt.opcode, opname);

          ImGui::TableSetColumnIndex(5);
          ImGui::Text("%u", pkt.payload_size);

          ImGui::TableSetColumnIndex(6);
          if (pkt.massive) {
            ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "M");
            ImGui::SameLine();
          }
          if (pkt.has_wire_header && pkt.security_crc != 0) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "C");
            ImGui::SameLine();
          }
          if (pkt.blocked) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "B");
            ImGui::SameLine();
          }
          if (pkt.modified) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "*");
          }
        }
      } // clipped rows

      if (net_cfg.auto_scroll && capture_changed && !filtered_indices.empty()) {
        ImGui::SetScrollY(ImGui::GetCursorPosY());
      }

      ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::InvisibleButton("splitter", ImVec2(-1, 5.0f));
    if (ImGui::IsItemActive()) {
      s_split_ratio += ImGui::GetIO().MouseDelta.y / total_height;
      if (s_split_ratio < 0.15f) s_split_ratio = 0.15f;
      if (s_split_ratio > 0.85f) s_split_ratio = 0.85f;
    }

    ImGui::BeginChild("detail_pane", ImVec2(0, bottom_height), true);
    const auto selected = std::find_if(filtered_indices.begin(), filtered_indices.end(), [&](int index) {
      return packets[static_cast<std::size_t>(index)].id == selected_id;
    });

    if (selected != filtered_indices.end()) {
      auto &pkt = packets[static_cast<std::size_t>(*selected)];
      ensure_parsed(pkt);

      // Packet Header Summary Banner
      ImGui::TextColored(pkt.direction == ext_client::packet_direction::client_to_server
                             ? ImVec4(0.4f, 0.8f, 1.0f, 1.0f)
                             : ImVec4(0.4f, 1.0f, 0.6f, 1.0f),
                         "[%s]", pkt.direction == ext_client::packet_direction::client_to_server ? "C -> S" : "S -> C");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.45f, 1.0f), "0x%04X %s", pkt.opcode, opcode_display_name(pkt.opcode));
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.75f, 0.65f, 1.0f, 1.0f), "[%s]", opcode_category_name(pkt.category));
      ImGui::SameLine();
      ImGui::TextDisabled("| Size: %u bytes", pkt.payload_size);

      if (pkt.has_wire_header) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.85f, 0.85f, 1.0f),
                           "| Wire: Raw=0x%04X Payload=%u Massive=%s SecCount=0x%02X SecCRC=0x%02X",
                           pkt.header_size_raw, pkt.header_payload_size,
                           pkt.massive ? "YES (bit 15)" : "NO",
                           pkt.security_count, pkt.security_crc);
      }
      if (pkt.blocked) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[BLOCKED]");
      }
      if (pkt.modified) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "[MODIFIED]");
      }

      ImGui::Separator();

      if (ImGui::BeginTabBar("PacketDetailTabs")) {
        // TAB 1: Parsed Field Inspector
        if (ImGui::BeginTabItem("Parsed Fields")) {
          if (pkt.parsed.doc_summary) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.90f, 0.70f, 1.0f));
            ImGui::TextWrapped("Doc: %s", pkt.parsed.doc_summary);
            ImGui::PopStyleColor();
            ImGui::Separator();
          }

          if (pkt.parsed.parser_method) {
            ImGui::TextDisabled("Parser: %s (%zu/%zu bytes decoded)",
                                pkt.parsed.parser_method, pkt.parsed.bytes_consumed, pkt.payload.size());
          }

          static char field_filter[64]{};
          ImGui::SetNextItemWidth(200.0f);
          ImGui::InputTextWithHint("##field_filter", "Filter fields...", field_filter, sizeof(field_filter));
          ImGui::Separator();

          if (pkt.parsed.fields.empty()) {
            if (!pkt.parsed.error.empty()) {
              ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Parse Error: %s", pkt.parsed.error.c_str());
            } else {
              ImGui::TextDisabled("(no fields decoded)");
            }
          } else {
            if (ImGui::BeginTable("fields_tree_table", 5,
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
              ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 55.0f);
              ImGui::TableSetupColumn("Field Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
              ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 55.0f);
              ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
              ImGui::TableSetupColumn("Semantic Description", ImGuiTableColumnFlags_WidthStretch);
              ImGui::TableHeadersRow();

              for (const auto &pf : pkt.parsed.fields) {
                if (field_filter[0] != '\0') {
                  if (pf.name.find(field_filter) == std::string::npos &&
                      pf.value.find(field_filter) == std::string::npos &&
                      pf.description.find(field_filter) == std::string::npos) {
                    continue;
                  }
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("+0x%04X", static_cast<unsigned>(pf.offset));

                ImGui::TableSetColumnIndex(1);
                std::string indent_str(static_cast<std::size_t>(pf.indent) * 2, ' ');
                ImGui::TextUnformatted((indent_str + pf.name).c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(type_color(pf.type), "%s", type_name(pf.type));

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(pf.value.c_str());

                ImGui::TableSetColumnIndex(4);
                if (!pf.description.empty()) {
                  ImGui::TextColored(ImVec4(0.45f, 0.95f, 0.70f, 1.0f), "%s", pf.description.c_str());
                } else {
                  ImGui::TextDisabled("-");
                }
              }
              ImGui::EndTable();
            }
          }
          ImGui::EndTabItem();
        }

        // TAB 2: Hex & ASCII Dump
        if (ImGui::BeginTabItem("Hex & ASCII")) {
          if (ImGui::Button("Copy Hex")) {
            copy_hex_clipboard(pkt.payload);
          }
          ImGui::SameLine();
          if (ImGui::Button("Copy C++ Array")) {
            copy_cpp_array_clipboard(pkt.opcode, pkt.payload);
          }
          ImGui::SameLine();
          if (ImGui::Button("Copy ASCII")) {
            copy_ascii_clipboard(pkt.payload);
          }

          ImGui::Separator();

          if (pkt.payload.empty()) {
            ImGui::TextDisabled("(payload is empty)");
          } else {
            const std::size_t row_size = 16;
            ImGuiListClipper hex_clipper;
            hex_clipper.Begin(static_cast<int>((pkt.payload.size() + row_size - 1) / row_size));
            while (hex_clipper.Step()) {
              for (int row = hex_clipper.DisplayStart; row < hex_clipper.DisplayEnd; ++row) {
                const std::size_t offset = static_cast<std::size_t>(row) * row_size;
                const auto remaining = pkt.payload.size() - offset;
                const auto len = remaining < row_size ? remaining : row_size;
                ImGui::TextDisabled("%04X:", static_cast<unsigned>(offset));
                ImGui::SameLine();
                render_hex_ascii(pkt.payload.data() + offset, len, 48);
              }
            }
          }
          ImGui::EndTabItem();
        }

        // TAB 3: Protocol Reference Documentation
        if (ImGui::BeginTabItem("Protocol Reference")) {
          ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "Opcode: 0x%04X (%u)", pkt.opcode, pkt.opcode);
          ImGui::Text("Message Name: %s", opcode_display_name(pkt.opcode));
          ImGui::Text("Category: %s", opcode_category_name(pkt.category));
          ImGui::Text("Direction: %s", pkt.direction == ext_client::packet_direction::client_to_server
                                           ? "Client -> Server (C->S)"
                                           : "Server -> Client (S->C)");
          ImGui::Separator();
          ImGui::TextWrapped("%s", opcode_summary_doc(pkt.opcode));
          ImGui::Separator();
          ImGui::TextDisabled("Documented in tools/silkroad-docs-main/docs (Silkroad Online Network Protocol).");
          ImGui::EndTabItem();
        }

        // TAB 4: Injection & Overrides
        if (ImGui::BeginTabItem("Injection & Override")) {
          ImGui::Text("Active Packet: 0x%04X (%zu bytes)", pkt.opcode, pkt.payload.size());
          ImGui::Separator();

          if (ImGui::Button("Load into Outgoing Override (C->S)")) {
            std::lock_guard lock(g_override_mutex);
            g_outgoing_override.opcode = pkt.opcode;
            g_outgoing_override.apply_all = false;
            g_outgoing_override.payload = pkt.payload;
            net_cfg.edit_outgoing = true;
            net_cfg.edit_outgoing_opcode = pkt.opcode;
            ui.note_dirty();
            log_msg("[net_log] Loaded 0x%04X into outgoing override buffer", pkt.opcode);
          }
          ImGui::SameLine();
          if (ImGui::Button("Load into Incoming Override (S->C)")) {
            std::lock_guard lock(g_override_mutex);
            g_incoming_override.opcode = pkt.opcode;
            g_incoming_override.apply_all = false;
            g_incoming_override.payload = pkt.payload;
            net_cfg.edit_incoming = true;
            net_cfg.edit_incoming_opcode = pkt.opcode;
            ui.note_dirty();
            log_msg("[net_log] Loaded 0x%04X into incoming override buffer", pkt.opcode);
          }

          ImGui::Spacing();
          ImGui::Text("Outgoing Override Status: %s (Target: 0x%04X, Buffer: %zu bytes)",
                      net_cfg.edit_outgoing ? "ACTIVE" : "OFF",
                      g_outgoing_override.opcode, g_outgoing_override.payload.size());
          ImGui::Text("Incoming Override Status: %s (Target: 0x%04X, Buffer: %zu bytes)",
                      net_cfg.edit_incoming ? "ACTIVE" : "OFF",
                      g_incoming_override.opcode, g_incoming_override.payload.size());

          if (ImGui::Button("Clear All Overrides")) {
            std::lock_guard lock(g_override_mutex);
            g_outgoing_override.payload.clear();
            g_incoming_override.payload.clear();
            net_cfg.edit_outgoing = false;
            net_cfg.edit_incoming = false;
            ui.note_dirty();
          }
          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }
    } else {
      ImGui::TextDisabled("Select a packet from the table above to inspect parsed fields, hex dump, protocol docs, and overrides.");
    }

    ImGui::EndChild();
  }
} // namespace ext_client::plugins::net_log
