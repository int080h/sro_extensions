#include "pch.hpp"
#include "sdk/process/cps_restart.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cps_restart::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSRestart");
}

auto cps_restart::create() -> cps_restart* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x009677F0);
  return reinterpret_cast<cps_restart*>(fn());
}

auto cps_restart::current() -> cps_restart* {
  return resolve_live();
}

auto cps_restart::resolve_live() -> cps_restart* {
  return ccontroler::active_child_as<cps_restart>("CPSRestart");
}
