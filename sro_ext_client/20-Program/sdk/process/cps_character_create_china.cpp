#include "pch.hpp"
#include "sdk/process/cps_character_create_china.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cps_character_create_china::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSCharacterCreateChina");
}

auto cps_character_create_china::create() -> cps_character_create_china* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x00952020);
  return reinterpret_cast<cps_character_create_china*>(fn());
}

auto cps_character_create_china::current() -> cps_character_create_china* {
  return resolve_live();
}

auto cps_character_create_china::resolve_live() -> cps_character_create_china* {
  return ccontroler::active_child_as<cps_character_create_china>("CPSCharacterCreateChina");
}

auto cps_character_create_china::get_appearance_list() -> ext_client::msvc9::n_vector<void*>& {
  return ext_client::off::field_at<ext_client::msvc9::n_vector<void*>>(this, 0x10C);
}

auto cps_character_create_china::get_max_char_count() const -> int {
  return ext_client::off::field_at<int>(this, 0x370);
}

auto cps_character_create_china::get_cur_char_count() const -> int {
  return ext_client::off::field_at<int>(this, 0x36C);
}
