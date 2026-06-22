#pragma once

#include "core/core_config.hpp"

#include <imgui.h>

namespace ext_client::utils::imgui_helpers {

  inline auto section_header(const char* text) -> void {
    ImGui::TextColored(ImVec4(0.35f, 0.72f, 0.92f, 1.0f), "%s", text);
    ImGui::Separator();
    ImGui::Spacing();
  }

  inline auto checkbox_dirty(const char* label, bool* value) -> bool {
    const bool changed = ImGui::Checkbox(label, value);
    if (changed) {
      ext_client::core::config::mark_dirty();
    }
    return changed;
  }

  inline auto slider_int_dirty(const char* label, int* value, int v_min, int v_max) -> bool {
    const bool changed = ImGui::SliderInt(label, value, v_min, v_max);
    if (changed) {
      ext_client::core::config::mark_dirty();
    }
    return changed;
  }

  inline auto input_text_dirty(const char* label, char* buf, std::size_t buf_size) -> bool {
    const bool changed = ImGui::InputText(label, buf, buf_size);
    if (changed) {
      ext_client::core::config::mark_dirty();
    }
    return changed;
  }

  inline auto drag_float2_dirty(const char* label, float* values, float speed = 0.5f, float v_min = -500.0f, float v_max = 500.0f, const char* fmt = "%.1f") -> bool {
    const bool changed = ImGui::DragFloat2(label, values, speed, v_min, v_max, fmt);
    if (changed) {
      ext_client::core::config::mark_dirty();
    }
    return changed;
  }

} // namespace ext_client::utils::imgui_helpers
