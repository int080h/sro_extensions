#include "menu/settings_panels.hpp"

#include "menu/settings_interface.hpp"
#include "menu/settings_version_check.hpp"
#include "menu/settings_login.hpp"
#include "menu/settings_graphics.hpp"
#include "menu/settings_welcome.hpp"
#include "menu/settings_advanced.hpp"
#include "menu/settings_app.hpp"
#include "menu/menu_theme.hpp"

#include <imgui.h>

namespace ext_client::menu::settings_panels {
  namespace {

    struct nav_item {
      settings_page id;
      const char* label;
    };

    constexpr nav_item k_nav[] = {
      {settings_page::version_check, "Version Check"},
      {settings_page::login, "Login"},
      {settings_page::graphics, "Graphics"},
      {settings_page::welcome, "Welcome"},
      {settings_page::advanced, "Advanced"},
      {settings_page::app, "App"},
    };

  } // namespace

  auto draw_sub_nav(settings_page& current) -> void {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
    for (const auto& item : k_nav) {
      const bool selected = current == item.id;
      if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_TabSelected));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_TabSelected));
      }
      if (ImGui::Button(item.label, ImVec2(-1.0f, 0.0f))) {
        current = item.id;
      }
      if (selected) {
        ImGui::PopStyleColor(2);
      }
    }
    ImGui::PopStyleVar();
  }

  auto draw_page(settings_page current) -> void {
    switch (current) {
    case settings_page::iface:
      settings_interface::draw();
      break;
    case settings_page::version_check:
      settings_version_check::draw();
      break;
    case settings_page::login:
      settings_login::draw();
      break;
    case settings_page::graphics:
      settings_graphics::draw();
      break;
    case settings_page::welcome:
      settings_welcome::draw();
      break;
    case settings_page::advanced:
      settings_advanced::draw();
      break;
    case settings_page::app:
      settings_app::draw();
      break;
    default:
      break;
    }
  }

} // namespace ext_client::menu::settings_panels
