#include "pch.hpp"
#include "sdk/process/cps_logo.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cps_logo::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSLogo");
}

auto cps_logo::create() -> cps_logo* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x00963D50);
  return reinterpret_cast<cps_logo*>(fn());
}

auto cps_logo::current() -> cps_logo* {
  return resolve_live();
}

auto cps_logo::resolve_live() -> cps_logo* {
  return ccontroler::active_child_as<cps_logo>("CPSLogo");
}
