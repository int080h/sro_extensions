#include "menu/packet_inspector.hpp"

#include "hooks/packet_hook.hpp"
#include "sdk/net_manager.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "menu/menu_ui.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace ext_client::menu::packet_inspector {
  namespace {

    char g_hex_editor[8192]{};
    char g_send_opcode[8] = "7007";
    char g_send_payload[256]{};
    std::uint32_t g_selected_id = 0;
    bool g_follow_latest = true;
    bool g_apply_override_all = false;

    struct view_filter {
      bool show_cmsg = true;
      bool show_stream = true;
      bool show_outgoing = true;
      bool show_incoming = true;
      bool hide_blocked = false;
      bool hide_modified = false;
      bool only_blocked = false;
      bool only_modified = false;
      int min_length = 0;
      int max_length = -1;
      char opcodes[128]{};
      char payload_hex[64]{};
    };

    view_filter g_view{};

    auto draw_selected_editor(const ext_client::net_manager::entry* selected) -> void;

    struct table_row {
      std::size_t index = 0;
      const ext_client::net_manager::entry* entry = nullptr;
    };

    auto load_entry_into_editor(const ext_client::net_manager::entry& item) -> void {
      const auto text = ext_client::net_manager::format_hex(item.payload);
      std::snprintf(g_hex_editor, sizeof(g_hex_editor), "%s", text.c_str());
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

    auto payload_contains_hex(const std::vector<std::uint8_t>& payload, const char* needle) -> bool {
      if (!needle || needle[0] == '\0') {
        return true;
      }

      std::string haystack;
      haystack.reserve(payload.size() * 2);
      for (const auto byte : payload) {
        char pair[3]{};
        std::snprintf(pair, sizeof(pair), "%02X", byte);
        haystack.append(pair);
      }

      std::string pattern;
      pattern.reserve(std::strlen(needle));
      for (const char* c = needle; *c != '\0'; ++c) {
        if (std::isspace(static_cast<unsigned char>(*c))) {
          continue;
        }
        pattern.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(*c))));
      }

      if (pattern.empty()) {
        return true;
      }

      return haystack.find(pattern) != std::string::npos;
    }

    auto matches_view_filter(const ext_client::net_manager::entry& item, const view_filter& filter) -> bool {
      if (item.layer == ext_client::net_manager::packet_layer::cmsg && !filter.show_cmsg) {
        return false;
      }
      if (item.layer == ext_client::net_manager::packet_layer::stream && !filter.show_stream) {
        return false;
      }
      if (item.direction == packet_direction::client_to_server && !filter.show_outgoing) {
        return false;
      }
      if (item.direction == packet_direction::server_to_client && !filter.show_incoming) {
        return false;
      }
      if (filter.hide_blocked && item.blocked) {
        return false;
      }
      if (filter.hide_modified && item.modified) {
        return false;
      }
      if (filter.only_blocked && !item.blocked) {
        return false;
      }
      if (filter.only_modified && !item.modified) {
        return false;
      }

      const auto len = static_cast<int>(item.payload.size());
      if (len < filter.min_length) {
        return false;
      }
      if (filter.max_length >= 0 && len > filter.max_length) {
        return false;
      }

      if (filter.opcodes[0] != '\0' && !ext_client::net_manager::opcode_in_list(item.opcode, filter.opcodes)) {
        return false;
      }

      if (!payload_contains_hex(item.payload, filter.payload_hex)) {
        return false;
      }

      return true;
    }

    auto build_filtered_rows(const std::vector<ext_client::net_manager::entry>& entries) -> std::vector<table_row> {
      std::vector<table_row> rows;
      rows.reserve(entries.size());
      for (std::size_t i = 0; i < entries.size(); ++i) {
        if (!matches_view_filter(entries[i], g_view)) {
          continue;
        }
        rows.push_back({i, &entries[i]});
      }
      return rows;
    }

    auto find_entry_by_id(const std::vector<ext_client::net_manager::entry>& entries, std::uint32_t id)
      -> const ext_client::net_manager::entry* {
      for (const auto& item : entries) {
        if (item.id == id) {
          return &item;
        }
      }
      return nullptr;
    }

    auto draw_capture_panel() -> void {
      if (!ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      auto& log_cfg = ext_client::net_manager::control_panel();

      ext_client::menu::ui::checkbox_column(&log_cfg.enabled, "capture enabled", &log_cfg.pause_capture, "pause capture");
      ext_client::menu::ui::checkbox_column(&log_cfg.log_outgoing, "log C->S", &log_cfg.log_incoming, "log S->C");
      ext_client::menu::ui::checkbox_column(&log_cfg.capture_cmsg, "layer CMsg", &log_cfg.capture_stream, "layer stream");
      ext_client::menu::ui::checkbox_column(&log_cfg.log_to_file, "write packets.log", &g_follow_latest, "follow latest");

      int max_entries = static_cast<int>(log_cfg.max_entries);
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::InputInt("max entries", &max_entries)) {
        log_cfg.max_entries = static_cast<std::size_t>(std::max(64, max_entries));
      }
      ImGui::SetNextItemWidth(-1.0f);
      ImGui::InputText("log file", log_cfg.file_path, sizeof(log_cfg.file_path));

      if (ImGui::Button("Clear log")) {
        ext_client::net_manager::clear_log();
        g_selected_id = 0;
        g_hex_editor[0] = '\0';
      }
    }

    auto draw_block_panel() -> void {
      if (!ImGui::CollapsingHeader("Block / intercept")) {
        return;
      }

      auto& log_cfg = ext_client::net_manager::control_panel();
      auto& hook_cfg = ext_client::hooks::packet::control_panel();

      ImGui::Checkbox("hook edits enabled", &hook_cfg.enabled);
      ext_client::menu::ui::checkbox_column(&log_cfg.block_outgoing, "block all C->S", &log_cfg.block_incoming, "block all S->C");

      int block_mode = log_cfg.block_opcode_mode;
      ImGui::SetNextItemWidth(140.0f);
      if (ImGui::Combo("opcode block", &block_mode, "off\0single opcode\0opcode list\0")) {
        log_cfg.block_opcode_mode = block_mode;
      }

      if (log_cfg.block_opcode_mode == static_cast<int>(ext_client::net_manager::opcode_block_mode::single)) {
        ImGui::SetNextItemWidth(90.0f);
        ImGui::InputScalar("opcode##block_single", ImGuiDataType_U16, &log_cfg.block_opcode, nullptr, nullptr, "%04X");
      } else if (log_cfg.block_opcode_mode == static_cast<int>(ext_client::net_manager::opcode_block_mode::list)) {
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("opcodes##block_list", "6101, A101, 7007", log_cfg.block_opcode_list, sizeof(log_cfg.block_opcode_list));
      }

      if (ImGui::Button("Clear overrides")) {
        ext_client::hooks::packet::clear_overrides();
      }
    }

    auto draw_view_filter_panel() -> void {
      if (!ImGui::CollapsingHeader("Display filter", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      ext_client::menu::ui::checkbox_column(&g_view.show_cmsg, "show CMsg", &g_view.show_stream, "show stream");
      ext_client::menu::ui::checkbox_column(&g_view.show_outgoing, "show C->S", &g_view.show_incoming, "show S->C");
      ext_client::menu::ui::checkbox_column(&g_view.hide_blocked, "hide blocked", &g_view.hide_modified, "hide modified");
      ext_client::menu::ui::checkbox_column(&g_view.only_blocked, "only blocked", &g_view.only_modified, "only modified");

      ImGui::SetNextItemWidth(90.0f);
      ImGui::InputInt("min len", &g_view.min_length);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(90.0f);
      ImGui::InputInt("max len", &g_view.max_length);
      if (g_view.min_length < 0) {
        g_view.min_length = 0;
      }

      ImGui::SetNextItemWidth(-1.0f);
      ImGui::InputTextWithHint("opcodes##view", "empty = all; e.g. 6101, A101, 0FF2", g_view.opcodes, sizeof(g_view.opcodes));
      ImGui::SetNextItemWidth(-1.0f);
      ImGui::InputTextWithHint("payload hex##view", "substring search in payload bytes", g_view.payload_hex, sizeof(g_view.payload_hex));

      if (ImGui::Button("Reset filters")) {
        g_view = {};
      }
    }

    auto row_background_color(const ext_client::net_manager::entry& item) -> ImU32 {
      if (item.blocked) {
        return ImGui::GetColorU32(ImVec4(0.45f, 0.15f, 0.15f, 0.35f));
      }
      if (item.modified) {
        return ImGui::GetColorU32(ImVec4(0.45f, 0.35f, 0.10f, 0.35f));
      }
      if (item.layer == ext_client::net_manager::packet_layer::cmsg) {
        return ImGui::GetColorU32(ImVec4(0.12f, 0.22f, 0.40f, 0.25f));
      }
      return ImGui::GetColorU32(ImVec4(0.12f, 0.30f, 0.18f, 0.20f));
    }

    auto sort_rows(std::vector<table_row>& rows, const ImGuiTableSortSpecs* specs) -> void {
      if (!specs || specs->SpecsCount == 0) {
        return;
      }

      const auto spec = specs->Specs[0];
      const bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;

      auto compare = [&](const table_row& a, const table_row& b) -> bool {
        const auto& lhs = *a.entry;
        const auto& rhs = *b.entry;
        int cmp = 0;

        switch (spec.ColumnIndex) {
          case 0:
            cmp = static_cast<int>(lhs.id) - static_cast<int>(rhs.id);
            break;
          case 1:
            cmp = static_cast<int>(lhs.layer) - static_cast<int>(rhs.layer);
            break;
          case 2:
            cmp = static_cast<int>(lhs.direction) - static_cast<int>(rhs.direction);
            break;
          case 3:
            cmp = static_cast<int>(lhs.opcode) - static_cast<int>(rhs.opcode);
            break;
          case 4:
            cmp = static_cast<int>(lhs.payload.size()) - static_cast<int>(rhs.payload.size());
            break;
          default:
            cmp = static_cast<int>(lhs.id) - static_cast<int>(rhs.id);
            break;
        }

        if (cmp == 0) {
          cmp = static_cast<int>(lhs.id) - static_cast<int>(rhs.id);
        }
        return ascending ? cmp < 0 : cmp > 0;
      };

      std::stable_sort(rows.begin(), rows.end(), compare);
    }

    auto draw_packet_table() -> void {
      const auto entries = ext_client::net_manager::snapshot();
      auto rows = build_filtered_rows(entries);

      static std::size_t last_rows_count = 0;
      static bool last_follow_latest = false;
      bool scroll_to_bottom = false;

      if (g_follow_latest && !entries.empty()) {
        g_selected_id = entries.back().id;
      }

      if (g_follow_latest) {
        if (!last_follow_latest || rows.size() != last_rows_count) {
          scroll_to_bottom = true;
        }
      }
      last_follow_latest = g_follow_latest;
      last_rows_count = rows.size();

      const ext_client::net_manager::entry* selected = find_entry_by_id(entries, g_selected_id);
      if (!selected && !rows.empty()) {
        selected = rows.back().entry;
        g_selected_id = selected->id;
      }

      ImGui::Separator();
      ImGui::Text("Packets  %zu shown / %zu captured", rows.size(), entries.size());

      const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                          ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortTristate;

      if (ImGui::BeginTable("packet_table", 6, table_flags, ImVec2(0.0f, 200.0f))) {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 48.0f);
        ImGui::TableSetupColumn("layer", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("dir", ImGuiTableColumnFlags_WidthFixed, 42.0f);
        ImGui::TableSetupColumn("opcode", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("len", ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("flags", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs(); specs && specs->SpecsDirty) {
          sort_rows(rows, specs);
          specs->SpecsDirty = false;
        }

        for (const auto& row : rows) {
          const auto& item = *row.entry;
          ImGui::TableNextRow();
          ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_background_color(item));

          ImGui::TableSetColumnIndex(0);
          const bool row_selected = item.id == g_selected_id;
          const auto id_label = std::to_string(item.id);
          if (ImGui::Selectable(id_label.c_str(), row_selected, ImGuiSelectableFlags_SpanAllColumns)) {
            g_selected_id = item.id;
            g_follow_latest = false;
            load_entry_into_editor(item);
          }

          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(ext_client::net_manager::format_layer(item.layer));

          ImGui::TableSetColumnIndex(2);
          ImGui::TextUnformatted(item.direction == packet_direction::client_to_server ? "C->S" : "S->C");

          ImGui::TableSetColumnIndex(3);
          ImGui::TextUnformatted(ext_client::net_manager::format_opcode(item.opcode).c_str());

          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%zu", item.payload.size());

          ImGui::TableSetColumnIndex(5);
          if (item.blocked && item.modified) {
            ImGui::TextUnformatted("blocked, modified");
          } else if (item.blocked) {
            ImGui::TextUnformatted("blocked");
          } else if (item.modified) {
            ImGui::TextUnformatted("modified");
          }
        }

        if (scroll_to_bottom) {
          ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
      }

      draw_selected_editor(selected);
    }

    auto draw_selected_editor(const ext_client::net_manager::entry* selected) -> void {
      if (!ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      if (!selected) {
        ImGui::TextDisabled("Select a packet row to inspect or edit payload bytes.");
        return;
      }

      ImGui::Text("%s %s  opcode %s  len %zu  id %u  tick %u",
                  ext_client::net_manager::format_layer(selected->layer),
                  selected->direction == packet_direction::client_to_server ? "C->S" : "S->C",
                  ext_client::net_manager::format_opcode(selected->opcode).c_str(),
                  selected->payload.size(),
                  selected->id,
                  selected->tick);

      if (selected->blocked || selected->modified) {
        ImGui::Text("flags: %s%s", selected->blocked ? "blocked " : "", selected->modified ? "modified" : "");
      }

      ImGui::InputTextMultiline(
        "payload hex", g_hex_editor, sizeof(g_hex_editor), ImVec2(-1.0f, 120.0f), ImGuiInputTextFlags_AllowTabInput);

      ImGui::Checkbox("apply to all matching opcode", &g_apply_override_all);

      if (ImGui::Button("Apply payload override")) {
        std::vector<std::uint8_t> bytes;
        if (ext_client::net_manager::parse_hex(g_hex_editor, bytes)) {
          if (selected->direction == packet_direction::client_to_server) {
            ext_client::hooks::packet::set_outgoing_override(selected->opcode, bytes.data(), bytes.size(), g_apply_override_all);
          } else {
            ext_client::hooks::packet::set_incoming_override(selected->opcode, bytes.data(), bytes.size(), g_apply_override_all);
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Filter opcode")) {
        std::snprintf(g_view.opcodes, sizeof(g_view.opcodes), "%04X", selected->opcode);
      }
      ImGui::SameLine();
      if (ImGui::Button("Block opcode")) {
        auto& cfg = ext_client::net_manager::control_panel();
        cfg.block_opcode_mode = static_cast<int>(ext_client::net_manager::opcode_block_mode::single);
        cfg.block_opcode = selected->opcode;
      }
      ImGui::SameLine();
      if (ImGui::Button("Copy to send")) {
        std::snprintf(g_send_opcode, sizeof(g_send_opcode), "%04X", selected->opcode);
        const auto text = ext_client::net_manager::format_hex(selected->payload);
        std::snprintf(g_send_payload, sizeof(g_send_payload), "%s", text.c_str());
      }
    }

    auto draw_manual_send() -> void {
      if (!ImGui::CollapsingHeader("Manual send")) {
        return;
      }

      ImGui::SetNextItemWidth(90.0f);
      ImGui::InputText("opcode", g_send_opcode, sizeof(g_send_opcode));
      ImGui::SetNextItemWidth(-1.0f);
      ImGui::InputText("payload hex", g_send_payload, sizeof(g_send_payload));

      if (ImGui::Button("Send packet")) {
        std::uint16_t opcode = 0;
        std::vector<std::uint8_t> payload;
        if (parse_opcode(g_send_opcode, opcode) && ext_client::net_manager::parse_hex(g_send_payload, payload)) {
          ext_client::net_manager::send(opcode, payload);
        }
      }
    }

  } // namespace

  auto draw() -> void {
    draw_capture_panel();
    draw_block_panel();
    draw_view_filter_panel();
    draw_packet_table();
    draw_manual_send();
  }

} // namespace ext_client::menu::packet_inspector
