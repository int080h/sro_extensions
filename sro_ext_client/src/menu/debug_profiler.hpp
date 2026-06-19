#pragma once

#include <cstddef>
#include <cstdint>

namespace ext_client::menu::debug_profiler {

  enum class phase_id : std::uint8_t {
    init_imgui = 0,
    hwnd_rebind = 1,
    core_tick = 2,
    flush_log = 3,
    imgui_begin = 4,
    menu_draw = 5,
    imgui_end = 6,
    sub_cps_title = 7,
    sub_cnif_ingame = 8,
    sub_iface_mgr = 9,
    sub_cps_version = 10,
    count,
  };

  constexpr std::size_t phase_count = static_cast<std::size_t>(phase_id::count);

  auto begin_phase(phase_id phase) -> void;
  auto end_phase(phase_id phase) -> void;

  auto is_phase_enabled(phase_id phase) -> bool;
  auto set_phase_enabled(phase_id phase, bool enabled) -> void;

  auto draw() -> void;

} // namespace ext_client::menu::debug_profiler
