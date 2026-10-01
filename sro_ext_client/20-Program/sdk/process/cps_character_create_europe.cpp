#include "pch.hpp"
#include "sdk/process/cps_character_create_europe.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;

} // namespace

auto cps_character_create_europe::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSCharacterCreateEurope");
}

auto cps_character_create_europe::create() -> cps_character_create_europe* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x00956020);
  return reinterpret_cast<cps_character_create_europe*>(fn());
}

auto cps_character_create_europe::current() -> cps_character_create_europe* {
  return resolve_live();
}

auto cps_character_create_europe::resolve_live() -> cps_character_create_europe* {
  return ccontroler::active_child_as<cps_character_create_europe>("CPSCharacterCreateEurope");
}

auto cps_character_create_europe::get_appearance_list() -> ext_client::msvc9::n_vector<void*>& {
  return ext_client::off::field_at<ext_client::msvc9::n_vector<void*>>(this, 0x10C);
}

auto cps_character_create_europe::get_max_char_count() const -> int {
  return ext_client::off::field_at<int>(this, 0x374);
}

auto cps_character_create_europe::get_cur_char_count() const -> int {
  return ext_client::off::field_at<int>(this, 0x370);
}
