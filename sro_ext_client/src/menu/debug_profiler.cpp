#include "menu/debug_profiler.hpp"

#include <imgui.h>

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace ext_client::menu::debug_profiler {
  namespace {

    struct phase_data {
      bool enabled = true;
      double last_us = 0.0;
      double max_us = 0.0;
      double avg_us = 0.0;

      static constexpr std::size_t k_history = 120;
      std::array<double, k_history> history{};
      std::size_t history_pos = 0;
      bool history_full = false;

      auto record(double us) -> void {
        last_us = us;
        max_us = std::max(max_us, us);
        history[history_pos] = us;
        history_pos = (history_pos + 1) % k_history;
        if (history_pos == 0) {
          history_full = true;
        }
        const auto count = history_full ? k_history : history_pos;
        double sum = 0.0;
        for (std::size_t i = 0; i < count; ++i) {
          sum += history[i];
        }
        avg_us = sum / static_cast<double>(count);
      }

      auto reset() -> void {
        last_us = 0.0;
        max_us = 0.0;
        avg_us = 0.0;
        history.fill(0.0);
        history_pos = 0;
        history_full = false;
      }
    };

    std::array<phase_data, phase_count> g_phases{};

    double g_freq_inv = 0.0;

    auto freq_inv() -> double {
      if (g_freq_inv == 0.0) {
        LARGE_INTEGER freq{};
        QueryPerformanceFrequency(&freq);
        g_freq_inv = 1.0e6 / static_cast<double>(freq.QuadPart);
      }
      return g_freq_inv;
    }

    const char* phase_labels[phase_count] = {
      "init_imgui",
      "hwnd_rebind",
      "core_tick",
      "flush_log",
      "imgui_begin",
      "menu_draw",
      "imgui_end",
      "  sub: cps_title",
      "  sub: cnif_ingame",
      "  sub: iface_mgr",
      "  sub: cps_version",
    };

    auto format_us(double us) -> const char* {
      thread_local char buf[32]{};
      if (us < 1.0) {
        std::snprintf(buf, sizeof(buf), "%.0f ns", us * 1000.0);
      } else if (us < 1000.0) {
        std::snprintf(buf, sizeof(buf), "%.1f us", us);
      } else {
        std::snprintf(buf, sizeof(buf), "%.2f ms", us / 1000.0);
      }
      return buf;
    }

  } // namespace

  auto begin_phase(phase_id phase) -> void {
    if (!g_phases[static_cast<std::size_t>(phase)].enabled) {
      return;
    }
    thread_local LARGE_INTEGER start{};
    QueryPerformanceCounter(&start);
    // Store start time in the phase_data's last_us temporarily
    g_phases[static_cast<std::size_t>(phase)].last_us = static_cast<double>(start.QuadPart);
  }

  auto end_phase(phase_id phase) -> void {
    auto& data = g_phases[static_cast<std::size_t>(phase)];
    if (!data.enabled) {
      return;
    }
    LARGE_INTEGER end{};
    QueryPerformanceCounter(&end);
    const auto start_ticks = static_cast<LONGLONG>(data.last_us);
    const auto delta_ticks = static_cast<double>(end.QuadPart - start_ticks);
    data.record(delta_ticks * freq_inv());
  }

  auto is_phase_enabled(phase_id phase) -> bool {
    return g_phases[static_cast<std::size_t>(phase)].enabled;
  }

  auto set_phase_enabled(phase_id phase, bool enabled) -> void {
    auto& data = g_phases[static_cast<std::size_t>(phase)];
    if (data.enabled && !enabled) {
      data.reset();
    }
    data.enabled = enabled;
  }

  auto draw() -> void {
    ImGui::TextDisabled("Frame phase profiler");
    ImGui::Separator();

    if (ImGui::BeginTable("profiler_phases", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
      ImGui::TableSetupColumn("phase", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("last", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("avg", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("max", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("enabled", ImGuiTableColumnFlags_WidthFixed, 60.0f);
      ImGui::TableHeadersRow();

      for (std::size_t i = 0; i < phase_count; ++i) {
        const auto& data = g_phases[i];
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(phase_labels[i]);

        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(format_us(data.last_us));

        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(format_us(data.avg_us));

        ImGui::TableSetColumnIndex(3);
        if (data.max_us > data.avg_us * 3.0 && data.max_us > 100.0) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.2f, 1.0f));
          ImGui::TextUnformatted(format_us(data.max_us));
          ImGui::PopStyleColor();
        } else {
          ImGui::TextUnformatted(format_us(data.max_us));
        }

        ImGui::TableSetColumnIndex(4);
        char label[32]{};
        std::snprintf(label, sizeof(label), "##enable_%zu", i);
        bool enabled = data.enabled;
        if (ImGui::Checkbox(label, &enabled)) {
          set_phase_enabled(static_cast<phase_id>(i), enabled);
        }
      }

      ImGui::EndTable();
    }

    ImGui::Spacing();
    if (ImGui::Button("Reset max")) {
      for (auto& data : g_phases) {
        data.max_us = 0.0;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset all")) {
      for (auto& data : g_phases) {
        data.reset();
      }
    }
  }

} // namespace ext_client::menu::debug_profiler
