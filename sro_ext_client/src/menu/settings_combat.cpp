#include "menu/settings_combat.hpp"

#include "config/client_config.hpp"
#include "features/combat_overlay/combat_overlay.hpp"
#include "menu/menu_ui.hpp"

#include <imgui.h>

namespace ext_client::menu::settings_combat {

  auto draw() -> void {
    ImGui::TextDisabled("Monster HP percent and DPS meter overlay.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("combat_overlay_card")) {
      auto& combat = ext_client::config::data().combat_overlay;
      bool changed = false;

      if (ImGui::Checkbox("Enable combat overlay", &combat.enabled)) {
        changed = true;
      }

      if (ImGui::Checkbox("Show monster HP % in target window", &combat.show_hp_percent)) {
        changed = true;
      }

      if (ImGui::Checkbox("Show DPS overlay (ImGui)", &combat.show_dps)) {
        changed = true;
      }

      if (ImGui::Checkbox("Show party DPS breakdown", &combat.show_party_dps)) {
        changed = true;
      }

      if (ImGui::Checkbox("Reset session on target death", &combat.reset_on_target_death)) {
        changed = true;
      }

      if (ImGui::Checkbox("Reset session on target change", &combat.reset_on_target_change)) {
        changed = true;
      }

      ext_client::menu::ui::set_full_width();
      if (ImGui::SliderInt("Combat timeout (sec)", &combat.combat_timeout_sec, 0, 60)) {
        changed = true;
      }

      ext_client::menu::ui::set_full_width();
      if (ImGui::SliderInt("SkillCastType::Attack byte", &combat.skill_cast_attack_type, -1, 16)) {
        changed = true;
      }
      ImGui::TextDisabled("HP %% works on common + special mob target windows. DPS uses B071 (-1) or B070+B071 (set Attack byte).");

      if (ImGui::Button("Reset DPS session now")) {
        combat_overlay::reset_session();
      }

      if (changed) {
        ext_client::config::mark_dirty();
      }

      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_combat
