#include "features/combat_overlay/target_window_hp.hpp"

#include "config/client_config.hpp"
#include "sdk/cif_target_window.hpp"
#include "sdk/cgwnd.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <imgui.h>

#include <cstdio>

namespace ext_client::target_window_hp {
  namespace {

    using ext_client::offsets::field_at;

    auto hp_percent_from_gauge(const cgwnd* gauge) -> int {
      if (!gauge || !ext_client::msvc9::is_game_ptr(gauge)) {
        return -1;
      }
      if (!ext_client::msvc9::is_readable_ptr(gauge, ext_client::offsets::cif_target_window::gauge_fields::fill_ratio + sizeof(float))) {
        return -1;
      }

      float ratio = field_at<float>(gauge, ext_client::offsets::cif_target_window::gauge_fields::fill_ratio);
      if (ratio < 0.f) {
        ratio = 0.f;
      }
      if (ratio > 1.f) {
        ratio = 1.f;
      }
      return static_cast<int>(ratio * 100.f + 0.5f);
    }

    auto draw_outlined_text(ImDrawList* draw_list, ImVec2 pos, ImU32 color, const char* text) -> void {
      const ImU32 shadow = IM_COL32(0, 0, 0, 220);
      draw_list->AddText(ImVec2(pos.x + 1.f, pos.y + 1.f), shadow, text);
      draw_list->AddText(ImVec2(pos.x - 1.f, pos.y + 1.f), shadow, text);
      draw_list->AddText(ImVec2(pos.x + 1.f, pos.y - 1.f), shadow, text);
      draw_list->AddText(ImVec2(pos.x - 1.f, pos.y - 1.f), shadow, text);
      draw_list->AddText(pos, color, text);
    }

  } // namespace

  auto render_overlay() -> void {
    const auto& cfg = ext_client::config::data().combat_overlay;
    if (!cfg.enabled || !cfg.show_hp_percent) {
      return;
    }

    void* panel = cif_target_window::active();
    if (!panel || !cif_target_window::is_live_target_panel(panel)) {
      return;
    }

    cgwnd* gauge = cif_target_window::hp_gauge(panel);
    if (!gauge) {
      return;
    }

    const int percent = hp_percent_from_gauge(gauge);
    if (percent < 0) {
      return;
    }

    char text[16]{};
    snprintf(text, sizeof(text), "%d%%", percent);

    const cgwnd_bounds bar = gauge->get_bounds();
    if (bar.w <= 0 || bar.h <= 0) {
      return;
    }

    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const ImVec2 pos(static_cast<float>(bar.x) + (static_cast<float>(bar.w) - text_size.x) * 0.5f,
                     static_cast<float>(bar.y) + (static_cast<float>(bar.h) - text_size.y) * 0.5f);

    draw_outlined_text(ImGui::GetForegroundDrawList(), pos, IM_COL32(255, 255, 255, 255), text);
  }

} // namespace ext_client::target_window_hp
