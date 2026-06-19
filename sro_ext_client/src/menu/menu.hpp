#pragma once

#include "utils/vectorf.h"

#include <imgui.h>

#include <cstdint>
#include <functional>

namespace ext_client::menu {

  class menu_controller {
  public:
    static auto get() -> menu_controller&;

    menu_controller(const menu_controller&) = delete;
    menu_controller& operator=(const menu_controller&) = delete;
    menu_controller(menu_controller&&) = delete;
    menu_controller& operator=(menu_controller&&) = delete;

    auto is_visible() const -> bool;
    auto set_visible(bool visible) -> void;
    auto toggle() -> void;

    auto wants_capture_mouse() const -> bool;
    auto wants_capture_keyboard() const -> bool;

    auto draw_shell() -> void;
    auto sync_capture_state() -> void;

  private:
    menu_controller() = default;

    bool m_visible = false;
    int m_capture_input_frames = 0;
    bool m_wants_capture_mouse = false;
    bool m_wants_capture_keyboard = false;
  };

  class menu_shell {
  public:
    static auto get() -> menu_shell&;

    menu_shell(const menu_shell&) = delete;
    menu_shell& operator=(const menu_shell&) = delete;
    menu_shell(menu_shell&&) = delete;
    menu_shell& operator=(menu_shell&&) = delete;

    auto on_shown() -> void;
    auto draw(bool* visible) -> void;

  private:
    menu_shell() = default;

    auto draw_menu_bar(bool* visible) -> void;
    auto draw_settings_window() -> void;

    struct window_state {
      bool settings = false;
      bool interface_mgr = true;
      bool combat = false;
      bool network = false;
      bool debug = false;
      bool widgets = false;
    };

    window_state m_windows{};
    int m_settings_page = 0;
  };

} // namespace ext_client::menu

namespace ext_client::menu::theme {

  constexpr float k_sidebar_width = 150.0f;
  constexpr float k_settings_subnav_width = 130.0f;
  constexpr float k_status_bar_height = 24.0f;
  constexpr float k_content_padding = 12.0f;

  auto apply_style() -> void;

} // namespace ext_client::menu::theme

namespace ext_client::menu::ui {

  auto checkbox_column(bool* left, const char* left_label, bool* right, const char* right_label) -> bool;
  auto setting_changed(bool sync_runtime = true) -> void;
  auto checkbox_setting(const char* label, bool* value, bool sync_runtime = true) -> bool;

  auto section_header(const char* title, bool default_open = true) -> bool;
  auto live_state_header(const char* title) -> bool;
  auto section_card_begin(const char* id) -> bool;
  auto section_card_end() -> void;
  auto set_full_width() -> void;
  auto offset_input2(const char* label, vector2f& value) -> bool;
  auto sro_color_edit(const char* label, std::uint32_t& sro_color) -> bool;

  auto apply_button(const char* label) -> bool;
  auto int_input_setting(const char* label, int* value, bool sync_runtime = true) -> bool;
  auto int_slider_setting(const char* label, int* value, int min_v, int max_v, const char* fmt = "%d", bool sync_runtime = true) -> bool;
  auto text_input_setting(const char* label, char* buffer, std::size_t buffer_size, bool sync_runtime = true) -> bool;

  auto two_column_table(const char* id, int imgui_table_flags, const std::function<void()>& draw_cells) -> void;
  auto checkbox_pair_table(const char* id, bool* left, const char* left_label, bool* right, const char* right_label) -> bool;

  auto avail_width(float min_w = 1.0f) -> float;
  auto avail_height(float reserve = 0.0f, float min_h = 80.0f) -> float;
  auto constrain_windows_to_viewport() -> void;
  auto tool_window_begin(const char* title, bool* open, ImVec2 default_size, ImGuiWindowFlags extra_flags = 0) -> bool;
  auto tool_window_end() -> void;

} // namespace ext_client::menu::ui
