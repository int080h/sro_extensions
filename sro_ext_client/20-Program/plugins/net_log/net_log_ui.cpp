#include "pch.hpp"
#include "plugins/net_log/net_log_ui.hpp"
#include "plugins/net_log/net_log_internal.hpp"
#include "plugins/net_log/packet_parser.hpp"
#include "plugins/net_log/packet_session.hpp"

#include "core/core_config.hpp"
#include "utils/log.hpp"

#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <optional>

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
        return ImVec4(0.5f, 0.8f, 0.5f, 1.0f);
      case pkt::field_type::u16:
        return ImVec4(0.5f, 0.8f, 0.6f, 1.0f);
      case pkt::field_type::u32:
        return ImVec4(0.5f, 0.8f, 0.7f, 1.0f);
      case pkt::field_type::u64:
        return ImVec4(0.5f, 0.8f, 0.8f, 1.0f);
      case pkt::field_type::i8:
        return ImVec4(0.8f, 0.5f, 0.5f, 1.0f);
      case pkt::field_type::i16:
        return ImVec4(0.8f, 0.5f, 0.6f, 1.0f);
      case pkt::field_type::i32:
        return ImVec4(0.8f, 0.5f, 0.7f, 1.0f);
      case pkt::field_type::i64:
        return ImVec4(0.8f, 0.5f, 0.8f, 1.0f);
      case pkt::field_type::f32:
        return ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
      case pkt::field_type::bool_:
        return ImVec4(0.6f, 0.6f, 1.0f, 1.0f);
      case pkt::field_type::ascii:
        return ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
      case pkt::field_type::loop:
        return ImVec4(0.7f, 0.5f, 1.0f, 1.0f);
      case pkt::field_type::branch:
        return ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
      default:
        return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
      }
    }

    auto type_name(pkt::field_type ft) -> const char * {
      switch (ft) {
      case pkt::field_type::u8:
        return "u8";
      case pkt::field_type::u16:
        return "u16";
      case pkt::field_type::u32:
        return "u32";
      case pkt::field_type::u64:
        return "u64";
      case pkt::field_type::i8:
        return "i8";
      case pkt::field_type::i16:
        return "i16";
      case pkt::field_type::i32:
        return "i32";
      case pkt::field_type::i64:
        return "i64";
      case pkt::field_type::f32:
        return "f32";
      case pkt::field_type::bool_:
        return "bool";
      case pkt::field_type::ascii:
        return "ascii";
      case pkt::field_type::raw:
        return "raw";
      case pkt::field_type::loop:
        return "loop";
      case pkt::field_type::branch:
        return "branch";
      default:
        return "?";
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
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
      ImGui::TextUnformatted(ascii.c_str());
      ImGui::PopStyleColor();
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

    if (ui.collapsing_header("Settings")) {
      ui.checkbox("Enable", &net_cfg.enabled);
      ui.same_line();
      ui.checkbox("Pause", &net_cfg.pause_capture);
      ui.same_line();
      ui.checkbox("Incoming", &net_cfg.log_incoming);
      ui.same_line();
      ui.checkbox("Outgoing", &net_cfg.log_outgoing);
      ui.same_line();
      ui.checkbox("CMsg", &net_cfg.capture_cmsg);
      ui.same_line();
      ui.checkbox("Stream", &net_cfg.capture_stream);

      ui.spacing();

      if (ui.button("Clear")) {
        clear_log();
      }
      ui.same_line();
      ui.checkbox("Auto-scroll", &net_cfg.auto_scroll);
      ui.same_line();
      ui.checkbox("Raw Hex", &net_cfg.show_raw_hex);
      ui.same_line();
      ui.checkbox("Parsed", &net_cfg.show_parsed);
      ui.same_line();
      ui.checkbox("Stats Bar", &net_cfg.show_stats_bar);

      ui.spacing();

      ui.set_next_item_width(200.0f);
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

    if (ui.collapsing_header("Filters")) {
      const char *dir_items[] = {"All", "S->C", "C->S"};
      ui.set_next_item_width(80.0f);
      ui.combo("Direction##filter", &net_cfg.filter_direction, dir_items, 3);
      ui.same_line();

      const char *layer_items[] = {"All", "CMsg", "Stream"};
      ui.set_next_item_width(80.0f);
      ui.combo("Layer##filter", &net_cfg.filter_layer, layer_items, 3);
      ui.same_line();

      const char *opmode_items[] = {"Off", "Single", "List", "Name"};
      ui.set_next_item_width(70.0f);
      ui.combo("Opcode##filter", &net_cfg.filter_opcode_mode, opmode_items, 4);
      ui.same_line();

      if (net_cfg.filter_opcode_mode == 1) {
        ui.set_next_item_width(80.0f);
        if (ui.input_int("##filter_opcode_single", &net_cfg.filter_opcode, 0, 0)) {
          if (net_cfg.filter_opcode < 0)
            net_cfg.filter_opcode = 0;
          if (net_cfg.filter_opcode > 0xFFFF)
            net_cfg.filter_opcode = 0xFFFF;
        }
      } else if (net_cfg.filter_opcode_mode == 2) {
        ui.set_next_item_width(150.0f);
        ui.input_text("##filter_opcode_list", net_cfg.filter_opcode_list, sizeof(net_cfg.filter_opcode_list));
      } else if (net_cfg.filter_opcode_mode == 3) {
        ui.set_next_item_width(120.0f);
        ui.input_text("##filter_opcode_name", net_cfg.filter_opcode_name, sizeof(net_cfg.filter_opcode_name));
      }

      ui.spacing();

      ui.checkbox("Massive", &net_cfg.filter_massive_only);
      ui.same_line();
      ui.checkbox("Blocked", &net_cfg.filter_blocked_only);
      ui.same_line();
      ui.checkbox("Modified", &net_cfg.filter_modified_only);
      ui.same_line();

      ui.set_next_item_width(60.0f);
      ui.input_int("Min Size##filter", &net_cfg.min_payload_size, 0, 0);
      ui.same_line();
      ui.set_next_item_width(60.0f);
      ui.input_int("Max Size##filter", &net_cfg.max_payload_size, 0, 0);
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
      ImGui::Text("Bytes: %zu", total_bytes);
      ImGui::Separator();
    }

    float total_height = ImGui::GetContentRegionAvail().y;
    static float s_split_ratio = 0.45f;
    float top_height = total_height * s_split_ratio;
    if (top_height < 80.0f)
      top_height = 80.0f;
    float bottom_height = total_height - top_height - 4.0f;
    if (bottom_height < 60.0f)
      bottom_height = 60.0f;

    ImGui::BeginChild("packet_list", ImVec2(0, top_height), true);
    if (ImGui::BeginTable("packets_table", 6,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders |
                              ImGuiTableFlags_Sortable)) {
      ImGui::TableSetupColumn("Tick", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 70.0f);
      ImGui::TableSetupColumn("Dir", ImGuiTableColumnFlags_WidthFixed, 45.0f);
      ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_WidthFixed, 55.0f);
      ImGui::TableSetupColumn("Opcode", ImGuiTableColumnFlags_WidthFixed, 200.0f);
      ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 50.0f);
      ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 60.0f);
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
            row_bg = IM_COL32(40, 30, 80, 60);
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
          ImGui::TextUnformatted(format_layer(pkt.layer));

          ImGui::TableSetColumnIndex(3);
          const char *opname = opcode_display_name(pkt.opcode);
          ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "%s", opname);

          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%u", pkt.payload_size);

          ImGui::TableSetColumnIndex(5);
          if (pkt.massive) {
            ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "M");
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

    ImGui::InvisibleButton("splitter", ImVec2(-1, 4.0f));
    if (ImGui::IsItemActive()) {
      s_split_ratio += ImGui::GetIO().MouseDelta.y / total_height;
      if (s_split_ratio < 0.1f)
        s_split_ratio = 0.1f;
      if (s_split_ratio > 0.9f)
        s_split_ratio = 0.9f;
    }

    ImGui::BeginChild("detail_pane", ImVec2(0, bottom_height), true);
    const auto selected = std::find_if(filtered_indices.begin(), filtered_indices.end(), [&](int index) {
      return packets[static_cast<std::size_t>(index)].id == selected_id;
    });
    if (selected != filtered_indices.end()) {
      auto &pkt = packets[static_cast<std::size_t>(*selected)];

      ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "[%s] %s (0x%04X)  size=%u",
                         pkt.direction == ext_client::packet_direction::client_to_server ? "C->S" : "S->C",
                         opcode_display_name(pkt.opcode), pkt.opcode, pkt.payload_size);
      if (pkt.has_wire_header) {
        ImGui::SameLine();
        ImGui::Text("  hdr: size=0x%04X payload=%u massive=%s sec=%02X crc=%02X", pkt.header_size_raw,
                    pkt.header_payload_size, pkt.massive ? "yes" : "no", pkt.security_count, pkt.security_crc);
      }
      if (pkt.blocked) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), " [BLOCKED]");
      }
      if (pkt.modified) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), " [MODIFIED]");
      }
      ImGui::Separator();

      float half_width = ImGui::GetContentRegionAvail().x * 0.5f - 4.0f;

      if (net_cfg.show_raw_hex) {
        ImGui::BeginChild("raw_hex", ImVec2(half_width, 0), true);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Raw Hex");
        ImGui::Separator();
        if (pkt.payload.empty()) {
          ImGui::TextDisabled("(empty)");
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
              render_hex_ascii(pkt.payload.data() + offset, len);
            }
          }
        }
        ImGui::EndChild();
        ImGui::SameLine();
      }

      if (net_cfg.show_parsed) {
        ensure_parsed(pkt);
        float parse_width = net_cfg.show_raw_hex ? half_width : ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("parsed_fields", ImVec2(parse_width, 0), true);
        ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "Parsed Fields");
        ImGui::Separator();

        if (pkt.parsed.fields.empty()) {
          if (!pkt.parsed.error.empty()) {
            ImGui::TextDisabled("%s", pkt.parsed.error.c_str());
          } else {
            ImGui::TextDisabled("(no parsed data)");
          }
        } else {
          for (const auto &pf : pkt.parsed.fields) {
            std::string indent_str(static_cast<std::size_t>(pf.indent) * 2, ' ');
            ImGui::TextColored(type_color(pf.type), "%s%s", indent_str.c_str(), type_name(pf.type));
            ImGui::SameLine();
            ImGui::Text(" %s", pf.name);
            ImGui::SameLine();
            ImGui::TextDisabled(" @%04X", static_cast<unsigned>(pf.offset));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), " = %s", pf.value.c_str());
          }
          if (!pkt.parsed.error.empty()) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Note: %s (%zu/%zu bytes)", pkt.parsed.error.c_str(),
                               pkt.parsed.bytes_consumed, pkt.payload.size());
          }
        }
        ImGui::EndChild();
      }
    } else {
      ImGui::TextDisabled("Select a packet to view details");
    }
    ImGui::EndChild();
  }

} // namespace ext_client::plugins::net_log
