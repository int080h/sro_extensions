#include "menu/packet_inspector.hpp"
#include "menu/packet_decoder.hpp"
#include "menu/menu.hpp"

#include "hooks/net_hook.hpp"
#include "utils/client_config.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace ext_client::menu::packet_inspector {
  namespace {

    enum class direction_filter : int {
      both = 0,
      client_to_server,
      server_to_client,
    };

    char g_hex_editor[8192]{};
    char g_send_opcode[8] = "7007";
    char g_send_payload[256]{};
    char g_opcode_filter[32]{};
    std::uint32_t g_selected_id = 0;
    bool g_follow_latest = true;
    bool g_apply_override_all = false;
    bool g_show_decoder_fields = true;
    bool g_advanced_open = false;
    direction_filter g_direction = direction_filter::both;

    auto draw_toolbar() -> void;
    auto draw_packet_list(const std::vector<ext_client::hooks::net::entry>& entries, float height) -> void;
    auto draw_detail_panel(const ext_client::hooks::net::entry* selected) -> void;
    auto draw_advanced_drawer() -> void;

    auto matches_direction(const ext_client::hooks::net::entry& item) -> bool {
      switch (g_direction) {
        case direction_filter::client_to_server:
          return item.direction == ext_client::hooks::net::packet_direction::client_to_server;
        case direction_filter::server_to_client:
          return item.direction == ext_client::hooks::net::packet_direction::server_to_client;
        default:
          return true;
      }
    }

    auto matches_opcode_filter(const ext_client::hooks::net::entry& item) -> bool {
      if (g_opcode_filter[0] == '\0') {
        return true;
      }
      return ext_client::hooks::net::opcode_in_list(item.opcode, g_opcode_filter);
    }

    auto row_tint(const ext_client::hooks::net::entry& item) -> ImU32 {
      if (item.blocked) {
        return ImGui::GetColorU32(ImVec4(0.55f, 0.15f, 0.15f, 0.45f));
      }
      if (item.modified) {
        return ImGui::GetColorU32(ImVec4(0.55f, 0.40f, 0.10f, 0.40f));
      }
      if (item.direction == ext_client::hooks::net::packet_direction::client_to_server) {
        return ImGui::GetColorU32(ImVec4(0.10f, 0.35f, 0.18f, 0.30f));
      }
      return ImGui::GetColorU32(ImVec4(0.10f, 0.22f, 0.45f, 0.30f));
    }

    auto format_time_ms(std::uint64_t timestamp_ms) -> std::string {
      const auto total_sec = timestamp_ms / 1000;
      const auto ms = timestamp_ms % 1000;
      const auto sec = total_sec % 60;
      const auto min = (total_sec / 60) % 60;
      const auto hour = (total_sec / 3600) % 24;
      char buf[32]{};
      std::snprintf(buf, sizeof(buf), "%02llu:%02llu:%02llu.%03llu", hour, min, sec, ms);
      return buf;
    }

    auto preview_hex(const std::vector<std::uint8_t>& payload, std::size_t max_bytes = 8) -> std::string {
      std::string out;
      const auto count = std::min(payload.size(), max_bytes);
      out.reserve(count * 3);
      for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
          out.push_back(' ');
        }
        char pair[4]{};
        std::snprintf(pair, sizeof(pair), "%02X", payload[i]);
        out.append(pair);
      }
      if (payload.size() > max_bytes) {
        out.append(" ..");
      }
      return out;
    }

    auto load_entry_into_editor(const ext_client::hooks::net::entry& item) -> void {
      const auto text = ext_client::hooks::net::format_hex(item.payload);
      std::snprintf(g_hex_editor, sizeof(g_hex_editor), "%s", text.c_str());
    }

    auto find_entry_by_id(const std::vector<ext_client::hooks::net::entry>& entries, std::uint32_t id)
      -> const ext_client::hooks::net::entry* {
      for (const auto& item : entries) {
        if (item.id == id) {
          return &item;
        }
      }
      return nullptr;
    }

    auto draw_hex_dump(const ext_client::hooks::net::entry& item) -> void {
      ImGui::TextDisabled("Hex dump");
      if (ImGui::Button("Copy hex")) {
        ImGui::SetClipboardText(ext_client::hooks::net::format_hex(item.payload).c_str());
      }
      ImGui::SameLine();
      if (ImGui::Button("Copy C array")) {
        std::string array = "{ ";
        for (std::size_t i = 0; i < item.payload.size(); ++i) {
          if (i != 0) {
            array.append(", ");
          }
          char byte[8]{};
          std::snprintf(byte, sizeof(byte), "0x%02X", item.payload[i]);
          array.append(byte);
        }
        array.append(" }");
        ImGui::SetClipboardText(array.c_str());
      }

      if (ImGui::BeginTable("packet_hex_dump", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 180.0f))) {
        ImGui::TableSetupColumn("offset", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("hex", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ascii", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableHeadersRow();

        constexpr std::size_t k_row_bytes = 16;
        for (std::size_t row = 0; row < item.payload.size(); row += k_row_bytes) {
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%04zX", row);

          ImGui::TableSetColumnIndex(1);
          std::string hex;
          hex.reserve(k_row_bytes * 3);
          for (std::size_t i = 0; i < k_row_bytes && row + i < item.payload.size(); ++i) {
            if (i != 0) {
              hex.push_back(' ');
            }
            char pair[4]{};
            std::snprintf(pair, sizeof(pair), "%02X", item.payload[row + i]);
            hex.append(pair);
          }
          ImGui::TextUnformatted(hex.c_str());

          ImGui::TableSetColumnIndex(2);
          char ascii[18]{};
          for (std::size_t i = 0; i < k_row_bytes && row + i < item.payload.size(); ++i) {
            const auto ch = item.payload[row + i];
            ascii[i] = (ch >= 32 && ch < 127) ? static_cast<char>(ch) : '.';
          }
          ImGui::TextUnformatted(ascii);
        }

        ImGui::EndTable();
      }
    }

    auto draw_decoder_fields(const ext_client::hooks::net::entry& item) -> void {
      if (!g_show_decoder_fields) {
        return;
      }

      const auto decoded = packet_decoder::decode(item);
      const char* name = decoded.opcode_name ? decoded.opcode_name : packet_decoder::opcode_name(item.opcode);
      if (name) {
        ImGui::Text("Decoded: %s", name);
      } else {
        ImGui::TextDisabled("Decoded: Unknown 0x%04X", item.opcode);
      }

      if (decoded.fields.empty()) {
        ImGui::TextDisabled("No field breakdown.");
        return;
      }

      if (ImGui::BeginTable("packet_fields", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 140.0f))) {
        ImGui::TableSetupColumn("off", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& field : decoded.fields) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%04zX", field.offset);
          ImGui::TableSetColumnIndex(1);
          switch (field.type) {
            case packet_decoder::field_type::u8:
              ImGui::TextUnformatted("u8");
              break;
            case packet_decoder::field_type::u16:
              ImGui::TextUnformatted("u16");
              break;
            case packet_decoder::field_type::u24:
              ImGui::TextUnformatted("u24");
              break;
            case packet_decoder::field_type::u32:
              ImGui::TextUnformatted("u32");
              break;
            case packet_decoder::field_type::f32:
              ImGui::TextUnformatted("f32");
              break;
            default:
              ImGui::TextUnformatted("?");
              break;
          }
          ImGui::TableSetColumnIndex(2);
          ImGui::TextUnformatted(field.name);
          ImGui::TableSetColumnIndex(3);
          ImGui::TextUnformatted(field.formatted_value.c_str());
        }

        ImGui::EndTable();
      }
    }

    auto draw_toolbar() -> void {
      auto& log_cfg = ext_client::hooks::net::control_panel();
      bool changed = false;

      changed |= ImGui::Checkbox("Capture", &log_cfg.enabled);
      ImGui::SameLine();
      changed |= ImGui::Checkbox("Pause", &log_cfg.pause_capture);
      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
        ext_client::hooks::net::clear_log();
        g_selected_id = 0;
        g_hex_editor[0] = '\0';
      }
      ImGui::SameLine();
      ImGui::Checkbox("Follow latest", &g_follow_latest);
      ImGui::SameLine();
      ImGui::Checkbox("Decoder fields", &g_show_decoder_fields);

      if (changed) {
        ext_client::menu::ui::setting_changed(false);
      }

      ImGui::Spacing();
      if (ImGui::RadioButton("Both", g_direction == direction_filter::both)) {
        g_direction = direction_filter::both;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("C->S", g_direction == direction_filter::client_to_server)) {
        g_direction = direction_filter::client_to_server;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("S->C", g_direction == direction_filter::server_to_client)) {
        g_direction = direction_filter::server_to_client;
      }

      ImGui::SameLine(0.0f, 24.0f);
      changed = false;
      changed |= ImGui::Checkbox("Stream", &log_cfg.capture_stream);
      ImGui::SameLine();
      changed |= ImGui::Checkbox("CMsg", &log_cfg.capture_cmsg);
      ImGui::SameLine();
      changed |= ImGui::Checkbox("log C->S", &log_cfg.log_outgoing);
      ImGui::SameLine();
      changed |= ImGui::Checkbox("log S->C", &log_cfg.log_incoming);

      ImGui::SetNextItemWidth(180.0f);
      ImGui::InputTextWithHint("##opcode_filter", "Opcode filter (e.g. B070, 7001)", g_opcode_filter, sizeof(g_opcode_filter));
      if (changed) {
        ext_client::menu::ui::setting_changed(false);
      }

      ImGui::Separator();
    }

    auto draw_packet_list(const std::vector<ext_client::hooks::net::entry>& entries, float height) -> void {
      auto& log_cfg = ext_client::hooks::net::control_panel();

      std::vector<const ext_client::hooks::net::entry*> rows;
      rows.reserve(entries.size());
      for (const auto& item : entries) {
        if (!matches_direction(item)) {
          continue;
        }
        if (!matches_opcode_filter(item)) {
          continue;
        }
        if (item.layer == ext_client::hooks::net::packet_layer::cmsg && !log_cfg.capture_cmsg) {
          continue;
        }
        if (item.layer == ext_client::hooks::net::packet_layer::stream && !log_cfg.capture_stream) {
          continue;
        }
        rows.push_back(&item);
      }

      static std::size_t last_rows_count = 0;
      bool scroll_to_bottom = false;
      if (g_follow_latest && !entries.empty()) {
        g_selected_id = entries.back().id;
      }
      if (g_follow_latest && rows.size() != last_rows_count) {
        scroll_to_bottom = true;
      }
      last_rows_count = rows.size();

      const ext_client::hooks::net::entry* selected = find_entry_by_id(entries, g_selected_id);
      if (!selected && !rows.empty()) {
        selected = rows.back();
        g_selected_id = selected->id;
      }

      ImGui::TextDisabled("%zu shown / %zu captured", rows.size(), entries.size());

      const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                          ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortTristate;

      if (ImGui::BeginTable("packet_table", 7, table_flags, ImVec2(0.0f, height))) {
        ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed, 96.0f);
        ImGui::TableSetupColumn("dir", ImGuiTableColumnFlags_WidthFixed, 42.0f);
        ImGui::TableSetupColumn("layer", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("opcode", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("len", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("preview", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("flags", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (const auto* item : rows) {
          ImGui::TableNextRow();
          ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_tint(*item));

          ImGui::TableSetColumnIndex(0);
          const bool row_selected = item->id == g_selected_id;
          const auto time_label = format_time_ms(item->timestamp_ms);
          if (ImGui::Selectable(time_label.c_str(), row_selected, ImGuiSelectableFlags_SpanAllColumns)) {
            g_selected_id = item->id;
            g_follow_latest = false;
            load_entry_into_editor(*item);
          }

          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(item->direction == ext_client::hooks::net::packet_direction::client_to_server ? "C->S" : "S->C");

          ImGui::TableSetColumnIndex(2);
          ImGui::TextUnformatted(ext_client::hooks::net::format_layer(item->layer));

          ImGui::TableSetColumnIndex(3);
          const char* decoded_name = packet_decoder::opcode_name(item->opcode);
          if (decoded_name) {
            ImGui::Text("%s  %s", ext_client::hooks::net::format_opcode(item->opcode), decoded_name);
          } else {
            ImGui::TextUnformatted(ext_client::hooks::net::format_opcode(item->opcode));
          }

          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%zu", item->payload.size());

          ImGui::TableSetColumnIndex(5);
          ImGui::TextUnformatted(preview_hex(item->payload).c_str());

          ImGui::TableSetColumnIndex(6);
          if (item->blocked && item->modified) {
            ImGui::TextUnformatted("blk,mod");
          } else if (item->blocked) {
            ImGui::TextUnformatted("blocked");
          } else if (item->modified) {
            ImGui::TextUnformatted("modified");
          }
        }

        if (scroll_to_bottom) {
          ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
      }
    }

    auto draw_detail_panel(const ext_client::hooks::net::entry* selected) -> void {
      ImGui::TextDisabled("Packet detail");
      ImGui::Separator();

      if (!selected) {
        ImGui::TextWrapped("Select a packet row.");
        return;
      }

      const char* decoded_name = packet_decoder::opcode_name(selected->opcode);
      ImGui::Text("id %u  tick %u  time %s", selected->id, selected->tick, format_time_ms(selected->timestamp_ms).c_str());
      ImGui::Text("%s %s  opcode %s%s%s  len %zu",
                  ext_client::hooks::net::format_layer(selected->layer),
                  selected->direction == ext_client::hooks::net::packet_direction::client_to_server ? "C->S" : "S->C",
                  ext_client::hooks::net::format_opcode(selected->opcode),
                  decoded_name ? " (" : "",
                  decoded_name ? decoded_name : "",
                  decoded_name ? ")" : "",
                  selected->payload.size());
      if (selected->capture_point && selected->capture_point[0] != '\0') {
        ImGui::TextDisabled("capture: %s", selected->capture_point);
      }
      if (selected->blocked || selected->modified) {
        ImGui::Text("flags: %s%s", selected->blocked ? "blocked " : "", selected->modified ? "modified" : "");
      }

      draw_decoder_fields(*selected);
      draw_hex_dump(*selected);

      if (g_hex_editor[0] == '\0') {
        load_entry_into_editor(*selected);
      }

      ImGui::Spacing();
      ImGui::InputTextMultiline(
        "payload hex##edit", g_hex_editor, sizeof(g_hex_editor), ImVec2(-1.0f, 72.0f), ImGuiInputTextFlags_AllowTabInput);

      ImGui::Checkbox("apply to all matching opcode", &g_apply_override_all);
      if (ImGui::Button("Apply override")) {
        std::vector<std::uint8_t> bytes;
        if (ext_client::hooks::net::parse_hex(g_hex_editor, bytes)) {
          if (selected->direction == ext_client::hooks::net::packet_direction::client_to_server) {
            ext_client::hooks::net::set_outgoing_override(selected->opcode, bytes.data(), bytes.size(), g_apply_override_all);
          } else {
            ext_client::hooks::net::set_incoming_override(selected->opcode, bytes.data(), bytes.size(), g_apply_override_all);
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Filter opcode")) {
        std::snprintf(g_opcode_filter, sizeof(g_opcode_filter), "%04X", selected->opcode);
      }
      ImGui::SameLine();
      if (ImGui::Button("Block opcode")) {
        auto& cfg = ext_client::hooks::net::control_panel();
        cfg.block_opcode_mode = static_cast<int>(ext_client::hooks::net::opcode_block_mode::single);
        cfg.block_opcode = selected->opcode;
        ext_client::menu::ui::setting_changed(false);
      }
      ImGui::SameLine();
      if (ImGui::Button("Copy to send")) {
        std::snprintf(g_send_opcode, sizeof(g_send_opcode), "%04X", selected->opcode);
        const auto text = ext_client::hooks::net::format_hex(selected->payload);
        std::snprintf(g_send_payload, sizeof(g_send_payload), "%s", text.c_str());
      }
    }

    auto parse_opcode(const char* text, std::uint16_t& opcode) -> bool {
      if (!text || text[0] == '\0') {
        return false;
      }
      unsigned value = 0;
      if (std::sscanf(text, "%x", &value) != 1 || value > 0xFFFFu) {
        return false;
      }
      opcode = static_cast<std::uint16_t>(value);
      return true;
    }

    auto draw_advanced_drawer() -> void {
      g_advanced_open = ImGui::CollapsingHeader("Advanced (block / manual send)", g_advanced_open ? ImGuiTreeNodeFlags_DefaultOpen : 0);
      if (!g_advanced_open) {
        return;
      }

      auto& log_cfg = ext_client::hooks::net::control_panel();
      auto& hook_cfg = ext_client::hooks::net::control_panel();
      bool changed = false;

      changed |= ext_client::menu::ui::checkbox_setting("hook edits enabled", &hook_cfg.enabled, false);
      changed |= ext_client::menu::ui::checkbox_column(&log_cfg.block_outgoing, "block all C->S", &log_cfg.block_incoming, "block all S->C");

      int block_mode = log_cfg.block_opcode_mode;
      ImGui::SetNextItemWidth(140.0f);
      if (ImGui::Combo("opcode block", &block_mode, "off\0single opcode\0opcode list\0")) {
        log_cfg.block_opcode_mode = block_mode;
        changed = true;
      }

      if (log_cfg.block_opcode_mode == static_cast<int>(ext_client::hooks::net::opcode_block_mode::single)) {
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::InputScalar("opcode##block_single", ImGuiDataType_U16, &log_cfg.block_opcode, nullptr, nullptr, "%04X")) {
          changed = true;
        }
      } else if (log_cfg.block_opcode_mode == static_cast<int>(ext_client::hooks::net::opcode_block_mode::list)) {
        ext_client::menu::ui::set_full_width();
        if (ImGui::InputTextWithHint("opcodes##block_list", "6101, A101, 7007", log_cfg.block_opcode_list, sizeof(log_cfg.block_opcode_list))) {
          changed = true;
        }
      }

      if (changed) {
        ext_client::menu::ui::setting_changed(false);
      }

      if (ImGui::Button("Clear overrides")) {
        ext_client::hooks::net::clear_overrides();
      }

      ImGui::Separator();
      ImGui::TextDisabled("Manual send");
      ImGui::SetNextItemWidth(90.0f);
      ImGui::InputText("opcode##send", g_send_opcode, sizeof(g_send_opcode));
      ext_client::menu::ui::set_full_width();
      ImGui::InputText("payload hex##send", g_send_payload, sizeof(g_send_payload));
      if (ImGui::Button("Send packet")) {
        std::uint16_t opcode = 0;
        std::vector<std::uint8_t> payload;
        if (parse_opcode(g_send_opcode, opcode) && ext_client::hooks::net::parse_hex(g_send_payload, payload)) {
          ext_client::hooks::net::send(opcode, payload);
        }
      }

      ImGui::Separator();
      changed = false;
      changed |= ImGui::Checkbox("write packets.log", &log_cfg.log_to_file);
      int max_entries = log_cfg.max_entries;
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::InputInt("max entries", &max_entries)) {
        log_cfg.max_entries = std::max(64, max_entries);
        changed = true;
      }
      ext_client::menu::ui::set_full_width();
      if (ImGui::InputText("log file", log_cfg.file_path, sizeof(log_cfg.file_path))) {
        changed = true;
      }
      if (changed) {
        ext_client::menu::ui::setting_changed(false);
      }
    }

  } // namespace

  auto draw() -> void {
    draw_toolbar();

    ext_client::hooks::net::visit_entries([](const std::vector<ext_client::hooks::net::entry>& entries) {
      const ext_client::hooks::net::entry* selected = find_entry_by_id(entries, g_selected_id);
      if (!selected && !entries.empty()) {
        selected = &entries.back();
        g_selected_id = selected->id;
      }

      const float split_h = ext_client::menu::ui::avail_height(g_advanced_open ? 220.0f : 36.0f, 120.0f);
      const float list_h = split_h;

      if (!ImGui::BeginTable("packet_split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
        return;
      }

      ImGui::TableSetupColumn("list", ImGuiTableColumnFlags_WidthStretch, 0.55f);
      ImGui::TableSetupColumn("detail", ImGuiTableColumnFlags_WidthStretch, 0.45f);
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      if (ImGui::BeginChild("packet_list_pane", ImVec2(0.0f, list_h), ImGuiChildFlags_Border)) {
        draw_packet_list(entries, ImGui::GetContentRegionAvail().y);
        ImGui::EndChild();
      }

      ImGui::TableSetColumnIndex(1);
      if (ImGui::BeginChild("packet_detail_pane", ImVec2(0.0f, list_h), ImGuiChildFlags_Border)) {
        draw_detail_panel(selected);
        ImGui::EndChild();
      }

      ImGui::EndTable();
    });

    draw_advanced_drawer();
  }

} // namespace ext_client::menu::packet_inspector
