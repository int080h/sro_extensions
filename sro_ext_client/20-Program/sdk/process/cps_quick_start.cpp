#include "pch.hpp"
#include "sdk/process/cps_quick_start.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cps_quick_start::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSQuickStart");
}

auto cps_quick_start::create() -> cps_quick_start* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x00966110);
  return reinterpret_cast<cps_quick_start*>(fn());
}

auto cps_quick_start::current() -> cps_quick_start* {
  return resolve_live();
}

auto cps_quick_start::resolve_live() -> cps_quick_start* {
  return ccontroler::active_child_as<cps_quick_start>("CPSQuickStart");
}
