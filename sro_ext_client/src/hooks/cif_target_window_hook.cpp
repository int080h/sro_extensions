#include "hooks/cif_target_window_hook.hpp"

#include "utils/client_config.hpp"
#include "sdk/cif_target_window.hpp"
#include "sdk/cgwnd.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <imgui.h>

#include <cstdio>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks {
  namespace {

    make_hook<convention_type::thiscall_t, int, void*, void*> g_populate_target;
    make_hook<convention_type::thiscall_t, void, void*, void*> g_populate_special_mob;
    make_hook<convention_type::thiscall_t, unsigned int, void*> g_update_special_mob;
    hook_group g_hooks;

    auto target_id_active(void* target_id) -> bool {
      return target_id != nullptr && reinterpret_cast<std::uintptr_t>(target_id) != 0;
    }

    auto __fastcall populate_target_detour(void* self, void* /*edx*/, void* target_id) -> int {
      const int result = g_populate_target.call_original(self, target_id);
      if (!target_id_active(target_id)) {
        cif_target_window::clear_active_panel();
      }
      return result;
    }

    auto __fastcall populate_special_mob_detour(void* self, void* /*edx*/, void* target_id) -> void {
      g_populate_special_mob.call_original(self, target_id);
      if (target_id_active(target_id)) {
        cif_target_window::note_active_panel(self);
      } else {
        cif_target_window::clear_active_panel();
      }
    }

    auto __fastcall update_special_mob_detour(void* self) -> unsigned int {
      const unsigned int result = g_update_special_mob.call_original(self);
      if (cif_target_window::is_live_target_panel(self)) {
        cif_target_window::note_active_panel(self);
      }
      return result;
    }

  } // namespace

  auto cif_target_window_hook::install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace ext_client::offsets::cif_target_window::functions;
    if (!g_hooks.install(g_populate_target,
                         populate_target,
                         &populate_target_detour,
                         "cif_target_window_hook",
                         "populate_target")) {
      return false;
    }

    if (!g_hooks.install(g_populate_special_mob,
                         populate_special_mob,
                         &populate_special_mob_detour,
                         "cif_target_window_hook",
                         "populate_special_mob")) {
      g_hooks.uninstall();
      return false;
    }

    if (!g_hooks.install(g_update_special_mob,
                         on_update_special_mob,
                         &update_special_mob_detour,
                         "cif_target_window_hook",
                         "update_special_mob")) {
      g_hooks.uninstall();
      return false;
    }

    log_msg("[cif_target_window_hook] Hook installed successfully");
    return true;
  }

  auto cif_target_window_hook::uninstall() -> void {
    g_hooks.uninstall();
    cif_target_window::clear_active_panel();
  }

  auto cif_target_window_hook::is_installed() -> bool {
    return g_hooks.is_installed();
  }

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

  auto cif_target_window_hook::render_hp_overlay() -> void {
    const auto& cfg = ext_client::config::data().target_window;
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

} // namespace ext_client::hooks
