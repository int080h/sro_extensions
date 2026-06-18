#include "cps_mission.hpp"

#include "utils/msvc9_stl.hpp"
#include "utils/process_manager.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

auto cps_mission::is_live(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  std::uint32_t vft = 0;
  if (!ext_client::msvc9::try_read_u32(ptr, &vft)) {
    return false;
  }
  return vft == ext_client::offsets::cps_mission::vtable::address;
}

auto cps_mission::create() -> cps_mission* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(ext_client::offsets::cps_mission::functions::create_instance);
  return reinterpret_cast<cps_mission*>(fn());
}

auto cps_mission::current() -> cps_mission* {
  auto* mission = global_at<cps_mission*>(ext_client::offsets::cps_mission::globals::active_instance);
  if (!mission || !is_live(mission)) {
    return nullptr;
  }
  return mission;
}

auto cps_mission::resolve_live() -> cps_mission* {
  if (auto* mission = ext_client::utils::process_manager::active_child_as<cps_mission>(
        ext_client::offsets::cps_mission::vtable::address)) {
    return mission;
  }
  return current();
}
