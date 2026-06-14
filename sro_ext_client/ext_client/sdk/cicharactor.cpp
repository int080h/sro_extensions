#include "sdk/cicharactor.hpp"
#include "sdk/ccompoundobj.hpp"
#include "utils/offsets.hpp"

auto SPosition::region_x() const -> std::uint8_t {
  return static_cast<std::uint8_t>(region_id & 0xFF);
}

auto SPosition::region_y() const -> std::uint8_t {
  return static_cast<std::uint8_t>(region_id >> 8);
}

auto ci_charactor::play_emote(unsigned char action_type, int emote_id) -> bool {
  using play_emote_fn = char(__thiscall*)(ci_charactor* this_ptr, unsigned char action_type, int emote_id);
  const auto play_emote_func = reinterpret_cast<play_emote_fn>(ext_client::offsets::ci_charactor::functions::play_emote);
  return play_emote_func(this, action_type, emote_id) != 0;
}

auto ci_charactor::get_compound_obj() -> ccompoundobj* {
  return ext_client::offsets::field_at<ccompoundobj*>(this, ext_client::offsets::ci_charactor::fields::compound_obj);
}

auto ci_charactor::position() const -> const SPosition* {
  return &ext_client::offsets::field_at<SPosition>(this, ext_client::offsets::ci_charactor::fields::position);
}

auto ci_charactor::hp() const -> std::uint32_t {
  return ext_client::offsets::field_at<std::uint32_t>(this, ext_client::offsets::ci_charactor::fields::hp);
}

auto ci_charactor::mp() const -> std::uint32_t {
  return ext_client::offsets::field_at<std::uint32_t>(this, ext_client::offsets::ci_charactor::fields::mp);
}

auto ci_charactor::max_hp() const -> std::uint32_t {
  return ext_client::offsets::field_at<std::uint32_t>(this, ext_client::offsets::ci_charactor::fields::max_hp);
}

auto ci_charactor::max_mp() const -> std::uint32_t {
  return ext_client::offsets::field_at<std::uint32_t>(this, ext_client::offsets::ci_charactor::fields::max_mp);
}

auto ci_charactor::set_hp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_hp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_hp_func = reinterpret_cast<set_hp_fn>(vtable[ext_client::offsets::ci_charactor::vtable::set_hp]);
  set_hp_func(this, val);
}

auto ci_charactor::set_mp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_mp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_mp_func = reinterpret_cast<set_mp_fn>(vtable[ext_client::offsets::ci_charactor::vtable::set_mp]);
  set_mp_func(this, val);
}

auto ci_charactor::set_max_hp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_max_hp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_max_hp_func = reinterpret_cast<set_max_hp_fn>(vtable[ext_client::offsets::ci_charactor::vtable::set_max_hp]);
  set_max_hp_func(this, val);
}

auto ci_charactor::set_max_mp(std::uint32_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_max_mp_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint32_t val);
  const auto set_max_mp_func = reinterpret_cast<set_max_mp_fn>(vtable[ext_client::offsets::ci_charactor::vtable::set_max_mp]);
  set_max_mp_func(this, val);
}

auto ci_charactor::state_619() const -> std::uint8_t {
  auto** vtable = *reinterpret_cast<void***>(const_cast<ci_charactor*>(this));
  using get_state_619_fn = std::uint8_t(__thiscall*)(const ci_charactor* this_ptr);
  const auto get_state_619_func = reinterpret_cast<get_state_619_fn>(vtable[ext_client::offsets::ci_charactor::vtable::get_state_619]);
  return get_state_619_func(this);
}

auto ci_charactor::set_state_619(std::uint8_t val) -> void {
  auto** vtable = *reinterpret_cast<void***>(this);
  using set_state_619_fn = void(__thiscall*)(ci_charactor* this_ptr, std::uint8_t val);
  const auto set_state_619_func = reinterpret_cast<set_state_619_fn>(vtable[ext_client::offsets::ci_charactor::vtable::set_state_619]);
  set_state_619_func(this, val);
}

auto ci_charactor::idle_timer() const -> int {
  return ext_client::offsets::field_at<int>(this, ext_client::offsets::ci_charactor::fields::idle_timer);
}

auto ci_charactor::set_idle_timer(int val) -> void {
  ext_client::offsets::field_at<int>(this, ext_client::offsets::ci_charactor::fields::idle_timer) = val;
}

auto ci_charactor::state_mask() const -> std::uint16_t {
  return ext_client::offsets::field_at<std::uint16_t>(this, ext_client::offsets::ci_charactor::fields::state_mask);
}

auto ci_charactor::set_state_mask(std::uint16_t val) -> void {
  ext_client::offsets::field_at<std::uint16_t>(this, ext_client::offsets::ci_charactor::fields::state_mask) = val;
}
