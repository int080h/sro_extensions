#include "pch.hpp"
#include "sdk/process/cps_mission.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/msvc9_stl.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cps_mission::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSMission");
}

auto cps_mission::create() -> cps_mission* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x0094DFB0);
  return reinterpret_cast<cps_mission*>(fn());
}

auto cps_mission::current() -> cps_mission* {
  auto* mission = global_at<cps_mission*>(0x0117E7B8);
  if (!mission || !is_live(mission)) {
    return nullptr;
  }
  return mission;
}

auto cps_mission::resolve_live() -> cps_mission* {
  if (auto* mission = ccontroler::active_child_as<cps_mission>("CPSMission")) {
    return mission;
  }
  return current();
}
