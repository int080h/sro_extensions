#include "ext_client.hpp"

#include "menu/settings_advanced.hpp"

#include "menu/menu_ui.hpp"

#include <imgui.h>

namespace ext_client::menu::settings_advanced {
  namespace {

    constexpr ImGuiTableFlags k_two_column_flags = ImGuiTableFlags_SizingStretchSame;

    auto draw_checkbox_pair(const char* left_label, bool* left, const char* right_label, bool* right) -> void {
      ImGui::TableNextColumn();
      ext_client::menu::ui::checkbox_setting(left_label, left);
      ImGui::TableNextColumn();
      ext_client::menu::ui::checkbox_setting(right_label, right);
    }

    auto draw_packet_defaults() -> void {
      if (!ext_client::menu::ui::section_header("Packet hook defaults")) {
        return;
      }

      auto& packet = ext_client::config::data().packet;
      bool changed = false;

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "pkt_main", &packet.enabled, "Hook enabled", &packet.log_events, "Log events");

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "pkt_out", &packet.edit_outgoing, "Edit outgoing", &packet.edit_outgoing_apply_all, "Apply all outgoing");

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "pkt_in", &packet.edit_incoming, "Edit incoming", &packet.edit_incoming_apply_all, "Apply all incoming");

      ImGui::SetNextItemWidth(100.0f);
      if (ImGui::InputScalar("Outgoing opcode", ImGuiDataType_U16, &packet.edit_outgoing_opcode, nullptr, nullptr, "%04X")) {
        changed = true;
      }
      ImGui::SetNextItemWidth(100.0f);
      if (ImGui::InputScalar("Incoming opcode", ImGuiDataType_U16, &packet.edit_incoming_opcode, nullptr, nullptr, "%04X")) {
        changed = true;
      }

      if (changed) {
        ext_client::menu::ui::setting_changed();
      }
    }

    auto draw_widget_defaults() -> void {
      if (!ext_client::menu::ui::section_header("Widget inspector defaults")) {
        return;
      }

      auto& widgets = ext_client::config::data().widgets;

      ext_client::menu::ui::two_column_table("widget_defaults_grid", k_two_column_flags, [&]() {
        draw_checkbox_pair("Static texts only", &widgets.static_only, "Auto refresh list", &widgets.auto_refresh);
        ImGui::TableNextColumn();
        ext_client::menu::ui::checkbox_setting("Show alarm slot debug", &widgets.show_alarm_debug);
      });

      ImGui::TextDisabled("Widget walk follows game child lists + res maps (VC2005); no depth cap.");
    }

  } // namespace

  auto draw() -> void {
    ImGui::TextDisabled("Packet hook INI defaults and widget inspector preferences.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("advanced_card")) {
      draw_packet_defaults();
      ImGui::Spacing();
      draw_widget_defaults();
      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_advanced
