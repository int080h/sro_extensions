#include "features/combat_overlay/combat_overlay.hpp"

#include "config/client_config.hpp"

#include <imgui.h>

namespace ext_client::combat_overlay {

  auto render_imgui() -> void {
    const auto& cfg = ext_client::config::data().combat_overlay;
    if (!cfg.enabled || !cfg.show_dps) {
      return;
    }

    const auto view = get_dps_stats();

    ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (cfg.dps_window_x >= 0.0f && cfg.dps_window_y >= 0.0f) {
      ImGui::SetNextWindowPos(ImVec2(cfg.dps_window_x, cfg.dps_window_y), ImGuiCond_FirstUseEver);
    }

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    if (!ImGui::Begin("Combat DPS", nullptr, flags)) {
      ImGui::End();
      return;
    }

    if (!view.target_name.empty()) {
      ImGui::Text("Target: %ls", view.target_name.c_str());
    } else {
      ImGui::TextDisabled("Target: —");
    }

    ImGui::Separator();
    ImGui::Text("DPS (3s): %.0f", view.dps_rolling);
    ImGui::Text("DPS (total): %.0f", view.dps_total);
    ImGui::Text("Damage: %u", view.damage_total_self);
    ImGui::Text("Time: %.1fs", view.fight_duration_sec);

    if (view.hit_count > 0) {
      const double crit_pct = (static_cast<double>(view.crit_count) * 100.0) / static_cast<double>(view.hit_count);
      ImGui::Text("Crits: %u (%.0f%%)", view.crit_count, crit_pct);
    }

    if (cfg.show_party_dps && !view.party_rows.empty()) {
      ImGui::Separator();
      ImGui::TextDisabled("Party");
      for (const auto& row : view.party_rows) {
        ImGui::Text("%ls  %u", row.first.c_str(), row.second);
      }
    }

    if (ImGui::Button("Reset")) {
      reset_session();
    }

    ImGui::End();
  }

} // namespace ext_client::combat_overlay
