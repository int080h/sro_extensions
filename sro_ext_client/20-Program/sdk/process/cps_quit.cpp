#include "pch.hpp"
#include "sdk/process/cps_quit.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cps_quit::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSQuit");
}

auto cps_quit::create() -> cps_quit* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x0094E1C0);
  return reinterpret_cast<cps_quit*>(fn());
}

auto cps_quit::current() -> cps_quit* {
  return resolve_live();
}

auto cps_quit::resolve_live() -> cps_quit* {
  return ccontroler::active_child_as<cps_quit>("CPSQuit");
}

auto cps_quit::is_quit_flag() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x0E0) != 0;
}

auto cps_quit::set_quit_flag(bool val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x0E0) = val ? 1 : 0;
}
