#include "pch.hpp"
#include "sdk/game/ci_charactor.hpp"
#include "sdk/game/ccompound_obj.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

auto ci_charactor::play_emote(unsigned char action_type, int emote_id) -> bool {
  using play_emote_fn = char(__thiscall*)(ci_charactor* this_ptr, unsigned char action_type, int emote_id);
  const auto play_emote_func = reinterpret_cast<play_emote_fn>(0x00B2A9B0);
  return play_emote_func(this, action_type, emote_id) != 0;
}

auto ci_charactor::get_compound_obj() -> ccompound_obj* {
  return ext_client::off::field_at<ccompound_obj*>(this, 0x004);
}

auto ci_charactor::get_position() const -> const s_position* {
  return &ext_client::off::field_at<s_position>(this, 0x07C);
}

auto ci_charactor::get_hp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x554);
}

auto ci_charactor::get_display_hp() const -> std::uint32_t {
  if (!this) {
    return 0;
  }
  using display_hp_fn = std::uint32_t(__thiscall*)(const ci_charactor*);
  const auto fn = ext_client::off::as_fn<display_hp_fn>(0x00B287B0);
  return fn(this);
}

auto ci_charactor::get_mp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x558);
}

auto ci_charactor::get_max_hp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x55C);
}

auto ci_charactor::get_max_mp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x560);
}

auto ci_charactor::set_hp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_hp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_hp_func = reinterpret_cast<set_hp_fn>(vtable[42/*set_hp*/]);
  set_hp_func(this, val);
}

auto ci_charactor::set_mp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_mp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_mp_func = reinterpret_cast<set_mp_fn>(vtable[43/*set_mp*/]);
  set_mp_func(this, val);
}

auto ci_charactor::set_max_hp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_max_hp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_max_hp_func = reinterpret_cast<set_max_hp_fn>(vtable[46/*set_max_hp*/]);
  set_max_hp_func(this, val);
}

auto ci_charactor::set_max_mp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_max_mp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_max_mp_func = reinterpret_cast<set_max_mp_fn>(vtable[47/*set_max_mp*/]);
  set_max_mp_func(this, val);
}

auto ci_charactor::get_state_619() const -> std::uint8_t {
  auto** vtable = *reinterpret_cast<void***>(const_cast<ci_charactor*>(this));
  using get_state_619_fn = std::uint8_t(__thiscall*)(const ci_charactor* this_ptr);
  const auto get_state_619_func = reinterpret_cast<get_state_619_fn>(vtable[50/*get_state_619*/]);
  return get_state_619_func(this);
}

auto ci_charactor::set_state_619(std::uint8_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_state_619_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint8_t val);
  const auto set_state_619_func = reinterpret_cast<set_state_619_fn>(vtable[51/*set_state_619*/]);
  set_state_619_func(this, val);
}

auto ci_charactor::get_idle_timer() const -> int {
  return ext_client::off::field_at<int>(this, 0x61C);
}

auto ci_charactor::set_idle_timer(int val) -> void {
  ext_client::off::field_at<int>(this, 0x61C) = val;
}

auto ci_charactor::get_state_mask() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x768);
}

auto ci_charactor::set_state_mask(std::uint16_t val) -> void {
  ext_client::off::field_at<std::uint16_t>(this, 0x768) = val;
}

auto ci_charactor::get_unique_id() const -> std::uint32_t {
  if (!this) {
    return 0;
  }
  using uid_fn = std::uint32_t(__thiscall*)(const ci_charactor*);
  const auto fn = ext_client::off::as_fn<uid_fn>(0x009D5C20);
  return fn(this);
}

auto ci_charactor::get_display_name() const -> const wchar_t* {
  if (!this) {
    return L"";
  }

  using name_ptr_fn = const wchar_t*(__thiscall*)(const ci_charactor*);
  const auto player_name_fn = ext_client::off::as_fn<name_ptr_fn>(0x009D5BC0);
  if (const wchar_t* player_name = player_name_fn(this)) {
    if (player_name && player_name[0] != L'\0') {
      return player_name;
    }
  }

  using refdata_fn = void*(__thiscall*)(const ci_charactor*);
  const auto refdata_lookup = ext_client::off::as_fn<refdata_fn>(0x009D5BF0);
  void* refdata = refdata_lookup(this);
  if (!refdata) {
    return L"";
  }

  constexpr std::size_t k_refdata_name = 0x15C;
  const auto name_ref = ext_client::msvc9::wstring_ref::from(reinterpret_cast<const std::uint8_t*>(refdata) + k_refdata_name);
  if (!name_ref.empty()) {
    return name_ref.data();
  }
  return L"";
}
