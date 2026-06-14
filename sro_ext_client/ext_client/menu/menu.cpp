#include "menu/menu.hpp"

#include "config/client_config.hpp"
#include "hooks/title_hook.hpp"
#include "hooks/version_check_hook.hpp"
#include "menu/config_panel.hpp"
#include "menu/menu_ui.hpp"
#include "menu/packet_inspector.hpp"
#include "menu/widget_inspector.hpp"
#include "sdk/cps_title.hpp"
#include "sdk/cps_version_check.hpp"
#include "utils/sro_color.hpp"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

namespace ext_client::menu {
  namespace {

    bool g_initialized = false;
    bool g_visible = false;
    int g_capture_input_frames = 0;

    auto draw_version_check_tab() -> void {
      auto& cfg = ext_client::hooks::version_check::control_panel();

      bool changed = false;

      changed |= ext_client::menu::ui::checkbox_column(&cfg.enabled, "enabled", &cfg.log_events, "log events");
      changed |=
        ext_client::menu::ui::checkbox_column(&cfg.bypass_gateway_connect, "bypass gateway", &cfg.skip_textdata_load, "skip textdata");
      changed |=
        ext_client::menu::ui::checkbox_column(&cfg.block_process_transition, "block transition", &cfg.skip_loading_splash, "skip splash");

      if (ImGui::Checkbox("force version result", &cfg.force_version_result)) {
        changed = true;
      }
      ImGui::SameLine(210.0f);
      ImGui::SetNextItemWidth(90.0f);
      if (ImGui::InputInt("result##vc", &cfg.forced_version_result)) {
        changed = true;
      }
      if (ImGui::Checkbox("loading overlay", &cfg.banner_overlay)) {
        changed = true;
      }

      if (changed) {
        ext_client::menu::ui::setting_changed(false);
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (auto* vc = cps_version_check::current()) {
        ImGui::Text("instance %p  net %d  load %s  active %s",
                    vc,
                    vc->net_state(),
                    vc->data_load_started() ? "yes" : "no",
                    cps_version_check::is_active() ? "yes" : "no");
      } else {
        ImGui::TextDisabled("instance (none)");
      }
      ImGui::Text("error %d  tag %d", cps_version_check::version_error_code(), cps_version_check::version_error_tag());
    }

    auto draw_login_tab() -> void {
      auto& cfg = ext_client::config::data().title;
      bool changed = false;

      // Checkboxes columns
      changed |= ext_client::menu::ui::checkbox_column(&cfg.enabled, "enabled##title", &cfg.log_events, "log events##title");
      changed |= ext_client::menu::ui::checkbox_column(&cfg.skip_on_create, "skip OnCreate", &cfg.auto_login_on_create, "auto login");
      changed |= ext_client::menu::ui::checkbox_column(
        &cfg.block_login_packets, "block login pkts", &cfg.block_process_transition, "block transition");

      if (ImGui::Checkbox("suppress captcha", &cfg.suppress_captcha)) {
        changed = true;
      }
      if (ImGui::Checkbox("hide channel list btn", &cfg.hide_channel_list_button)) {
        changed = true;
      }

      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::InputInt("logo y offset", &cfg.logo_y_offset, 4, 20)) {
        if (cfg.logo_y_offset < 0) {
          cfg.logo_y_offset = 0;
        }
        changed = true;
      }
      ImGui::SameLine();
      ImGui::TextDisabled("(px up, 0=default)");

      if (ImGui::Checkbox("override version text", &cfg.override_version_labels)) {
        changed = true;
      }

      if (ImGui::CollapsingHeader("Version label settings")) {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("data fmt", cfg.data_version_fmt, sizeof(cfg.data_version_fmt))) {
          changed = true;
        }

        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("exe fmt", cfg.exe_version_fmt, sizeof(cfg.exe_version_fmt))) {
          changed = true;
        }

        if (ImGui::Checkbox("override color", &cfg.override_version_label_color)) {
          changed = true;
        }

        ImVec4 label_color = ImGui::ColorConvertU32ToFloat4(ext_client::utils::color::to_imgui(cfg.version_label_color));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::ColorEdit4("##label_color", &label_color.x, ImGuiColorEditFlags_NoInputs)) {
          cfg.version_label_color = ext_client::utils::color::from_imgui(ImGui::ColorConvertFloat4ToU32(label_color));
          changed = true;
        }

        if (ImGui::Checkbox("clip text (hover to expand)", &cfg.version_labels_clip)) {
          changed = true;
        }
        if (cfg.version_labels_clip) {
          ImGui::SetNextItemWidth(120.0f);
          if (ImGui::InputInt("clip width", &cfg.version_label_ellipsis_width, 4, 20)) {
            if (cfg.version_label_ellipsis_width < 16) {
              cfg.version_label_ellipsis_width = 16;
            }
            changed = true;
          }
        }

        ImGui::Text("captured %d/2", ext_client::hooks::title::captured_version_label_count());
        ImGui::SameLine();
        if (ImGui::Button("Apply labels")) {
          ext_client::hooks::title::apply_version_labels();
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply layout")) {
          ext_client::hooks::title::apply_version_label_layout();
        }
      }

      if (changed) {
        ext_client::menu::ui::setting_changed();
        ext_client::hooks::title::apply_from_control();
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (auto* title = cps_title::current()) {
        ImGui::Text("instance %p  phase %d  captcha %s  ch %d",
                    title,
                    title->login_phase(),
                    title->captcha_active() ? "yes" : "no",
                    cps_title::channel_index());
        if (ImGui::Button("DoLogin")) {
          title->trigger_login();
        }
      } else {
        ImGui::TextDisabled("CPSTitle not active");
      }
    }

    auto draw_widgets_tab() -> void {
      ext_client::menu::widget_inspector::draw();
    }

  } // namespace

  auto on_create(HWND hwnd, IDirect3DDevice9* device) -> void {
    if (g_initialized || !device) {
      return;
    }

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad);
    io.MouseDrawCursor = false;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 3.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);
    g_initialized = true;
  }

  auto shutdown() -> void {
    if (!g_initialized) {
      return;
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_initialized = false;
    g_visible = false;
  }

  auto render() -> void {
    if (!g_initialized || !g_visible) {
      return;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_capture_input_frames > 0) {
      ImGui::SetNextFrameWantCaptureMouse(true);
      ImGui::SetNextFrameWantCaptureKeyboard(true);
      if (g_capture_input_frames == 2) {
        ImGui::SetNextWindowFocus();
      }
      --g_capture_input_frames;
    }

    ImGui::SetNextWindowSize(ImVec2(720.0f, 560.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("sro_ext_client", &g_visible)) {
      if (ImGui::BeginTabBar("ext_client_tabs", ImGuiTabBarFlags_NoTooltip)) {
        if (ImGui::BeginTabItem("Config")) {
          ext_client::menu::config_panel::draw();
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("CPSVersionCheck")) {
          draw_version_check_tab();
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("CPSTitle")) {
          draw_login_tab();
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Widgets")) {
          draw_widgets_tab();
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Network")) {
          ext_client::menu::packet_inspector::draw();
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
    }
    ImGui::End();

    ext_client::menu::widget_inspector::draw_overlay();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
  }

  auto is_visible() -> bool {
    return g_visible;
  }
  auto set_visible(bool visible) -> void {
    if (g_visible == visible) {
      return;
    }

    g_visible = visible;
    if (!g_initialized) {
      return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.ClearEventsQueue();
    io.ClearInputKeys();
    io.ClearInputMouse();

    if (visible) {
      g_capture_input_frames = 2;
    } else {
      g_capture_input_frames = 0;
    }
  }

} // namespace ext_client::menu
