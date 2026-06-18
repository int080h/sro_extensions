#include "ext_client.hpp"

#include "menu/settings_graphics.hpp"

#include "menu/menu_ui.hpp"
#include "sdk/sworld.hpp"
#include "utils/offsets.hpp"
#include "utils/sight_range_math.hpp"

#include <imgui.h>

namespace ext_client::menu::settings_graphics {
  namespace {

    constexpr ImGuiTableFlags k_two_column_flags = ImGuiTableFlags_SizingStretchSame;

  } // namespace

  auto draw() -> void {
    ImGui::TextDisabled("Sight range override and D3D9 presentation flags.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("gfx_sight_card")) {
      auto& graphic = ext_client::config::data().graphic;
      bool changed = false;

      if (ImGui::Checkbox("Enable sight range override", &graphic.custom_sight_range)) {
        changed = true;
      }

      if (graphic.custom_sight_range) {
        ext_client::menu::ui::set_full_width();
        if (ImGui::SliderInt("Override percentage", &graphic.sight_range_percent, 50, 400, "%d%%")) {
          changed = true;
        }

        const auto preview = ext_client::utils::sight_range::compute_from_percent(graphic.sight_range_percent);
        ImGui::TextDisabled("apply range %.1f | cell %d | fade %.1f", preview.range, preview.cell_limit, preview.fade);
      } else {
        ImGui::TextDisabled("default: dropdown setting 5 auto-upgrades to level 6");
      }

      ext_client::menu::ui::set_full_width();
      if (ImGui::SliderInt("Sight range level", &graphic.sight_range_level, 0, 10)) {
        changed = true;
      }

      ImGui::Spacing();
      ImGui::Text("Active values");
      auto* world = sworld::instance();
      if (world && sworld::is_instance()) {
        const float active_range = world->sight_range();
        const std::int32_t active_cell = world->cell_limit();
        const float active_fade = ext_client::offsets::global_at<float>(ext_client::offsets::sworld::globals::details_fade_distance);
        const int current_opt = world->option(8);

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                           "option %d (index %d) | range %.1f | cell %d | fade %.1f",
                           current_opt + 1,
                           current_opt,
                           active_range,
                           active_cell,
                           active_fade);
      } else {
        ImGui::TextDisabled("world not loaded");
      }

      if (changed) {
        ext_client::menu::ui::setting_changed();
      }
      ext_client::menu::ui::section_card_end();
    }

    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("gfx_d3d_card")) {
      auto& graphic = ext_client::config::data().graphic;
      bool changed = false;

      ext_client::menu::ui::two_column_table("d3d_flags", k_two_column_flags, [&]() {
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("Force hardware VP", &graphic.d3d_force_hardware_vp)) {
          changed = true;
        }
        if (ImGui::Checkbox("Triple buffering", &graphic.d3d_triple_buffering)) {
          changed = true;
        }

        ImGui::TableNextColumn();
        if (ImGui::Checkbox("Force pure device", &graphic.d3d_force_pure_device)) {
          changed = true;
        }
        if (ImGui::Checkbox("Discard depth stencil", &graphic.d3d_discard_depth_stencil)) {
          changed = true;
        }
      });

      if (changed) {
        ext_client::menu::ui::setting_changed();
      }
      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_graphics
