#include "sdk/cicharactor.hpp"
#include "sdk/ccompoundobj.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

auto ci_charactor::play_emote(unsigned char action_type, int emote_id) -> bool {
  using play_emote_fn = char(__thiscall*)(ci_charactor* this_ptr, unsigned char action_type, int emote_id);
  const auto play_emote_func = reinterpret_cast<play_emote_fn>(ext_client::offsets::ci_charactor::functions::play_emote);
  return play_emote_func(this, action_type, emote_id) != 0;
}

auto ci_charactor::get_compound_obj() -> ccompoundobj* {
  return m_compound_obj;
}

auto ci_charactor::position() const -> const SPosition* {
  return &m_coords;
}

auto ci_charactor::hp() const -> std::uint32_t {
  return m_hp;
}

auto ci_charactor::display_hp() const -> std::uint32_t {
  if (!this) {
    return 0;
  }
  using display_hp_fn = std::uint32_t(__thiscall*)(const ci_charactor*);
  const auto fn = ext_client::offsets::as_fn<display_hp_fn>(ext_client::offsets::ci_charactor::functions::display_hp);
  return fn(this);
}

auto ci_charactor::mp() const -> std::uint32_t {
  return m_mp;
}

auto ci_charactor::max_hp() const -> std::uint32_t {
  return m_max_hp;
}

auto ci_charactor::max_mp() const -> std::uint32_t {
  return m_max_mp;
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
  return m_idle_timer;
}

auto ci_charactor::set_idle_timer(int val) -> void {
  m_idle_timer = val;
}

auto ci_charactor::state_mask() const -> std::uint16_t {
  return m_state_mask;
}

auto ci_charactor::set_state_mask(std::uint16_t val) -> void {
  m_state_mask = val;
}

auto ci_charactor::unique_id() const -> std::uint32_t {
  if (!this) {
    return 0;
  }
  using uid_fn = std::uint32_t(__thiscall*)(const ci_charactor*);
  const auto fn = ext_client::offsets::as_fn<uid_fn>(ext_client::offsets::entity_manager::functions::unique_id);
  return fn(this);
}

auto ci_charactor::display_name() const -> const wchar_t* {
  if (!this) {
    return L"";
  }

  using name_ptr_fn = const wchar_t*(__thiscall*)(const ci_charactor*);
  const auto player_name_fn = ext_client::offsets::as_fn<name_ptr_fn>(ext_client::offsets::entity_manager::functions::display_name_ptr);
  if (const wchar_t* player_name = player_name_fn(this)) {
    if (player_name && player_name[0] != L'\0') {
      return player_name;
    }
  }

  using refdata_fn = void*(__thiscall*)(const ci_charactor*);
  const auto refdata_lookup = ext_client::offsets::as_fn<refdata_fn>(ext_client::offsets::entity_manager::functions::refdata);
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
