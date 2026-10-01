#pragma once

#include "core/core_config.hpp"

#include <cstdarg>
#include <cstddef>
#include <utility>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>

namespace ext_client::render::menu {

  class menu_builder {
  public:
    auto section(const char *title) -> void { ImGui::SeparatorText(title); }

    auto spacing() -> void { ImGui::Spacing(); }

    auto same_line(float offset_from_start_x = 0.0f, float spacing_w = -1.0f) -> void {
      ImGui::SameLine(offset_from_start_x, spacing_w);
    }

    auto set_next_item_width(float width) -> void { ImGui::SetNextItemWidth(width); }

    auto checkbox(const char *label, bool *value) -> bool {
      return dirty_widget([&]() { return ImGui::Checkbox(label, value); });
    }

    auto slider_int(const char *label, int *value, int v_min, int v_max) -> bool {
      return dirty_widget([&]() { return ImGui::SliderInt(label, value, v_min, v_max); });
    }

    auto input_int(const char *label, int *value, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
        -> bool {
      return dirty_widget([&]() { return ImGui::InputInt(label, value, step, step_fast, flags); });
    }

    auto input_text(const char *label, char *buf, std::size_t buf_size, ImGuiInputTextFlags flags = 0) -> bool {
      return dirty_widget([&]() { return ImGui::InputText(label, buf, buf_size, flags); });
    }

    auto combo(const char *label, int *current_item, const char *const items[], int item_count,
               int popup_max_height_in_items = -1) -> bool {
      return dirty_widget(
          [&]() { return ImGui::Combo(label, current_item, items, item_count, popup_max_height_in_items); });
    }

    auto drag_float2(const char *label, float values[2], float speed = 0.5f, float v_min = 0.0f, float v_max = 0.0f,
                     const char *fmt = "%.1f") -> bool {
      return dirty_widget([&]() { return ImGui::DragFloat2(label, values, speed, v_min, v_max, fmt); });
    }

    auto button(const char *label) -> bool { return ImGui::Button(label); }

    auto collapsing_header(const char *label, ImGuiTreeNodeFlags flags = 0) -> bool {
      return ImGui::CollapsingHeader(label, flags);
    }

    auto text(const char *fmt, ...) -> void {
      va_list args;
      va_start(args, fmt);
      ImGui::TextV(fmt, args);
      va_end(args);
    }

    auto text_wrapped(const char *fmt, ...) -> void {
      va_list args;
      va_start(args, fmt);
      ImGui::TextWrappedV(fmt, args);
      va_end(args);
    }

    auto any_changed() const -> bool { return m_any_changed; }

    auto reset_changed() -> void { m_any_changed = false; }

    auto note_dirty() -> void {
      ext_client::core::config::mark_dirty();
      m_any_changed = true;
    }

  private:
    bool m_any_changed = false;

    template <typename Fn> auto dirty_widget(Fn &&fn) -> bool {
      const bool changed = std::forward<Fn>(fn)();
      if (changed) {
        ext_client::core::config::mark_dirty();
        m_any_changed = true;
      }
      return changed;
    }
  };

} // namespace ext_client::render::menu
