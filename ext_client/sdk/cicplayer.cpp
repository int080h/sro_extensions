#include "sdk/cicplayer.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

auto cic_player::local() -> cic_player* {
  if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::cic_player::globals::local_player), sizeof(void*))) {
    return nullptr;
  }
  return *reinterpret_cast<cic_player**>(ext_client::offsets::cic_player::globals::local_player);
}

auto cic_player::name() const -> const wchar_t* {
  using msvc9_wstring = ext_client::msvc9::wstring;
  const auto* wstr = reinterpret_cast<const msvc9_wstring*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::name);
  return wstr->data();
}

auto cic_player::guild_name() const -> const wchar_t* {
  using msvc9_wstring = ext_client::msvc9::wstring;
  const auto* wstr = reinterpret_cast<const msvc9_wstring*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::guild_name);
  return wstr->data();
}

auto cic_player::is_gamemaster() const -> bool {
  return *reinterpret_cast<const int*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::gamemaster) == 0x10001;
}

auto cic_player::level() const -> std::uint8_t {
  return *reinterpret_cast<const std::uint8_t*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::level);
}

auto cic_player::exp() const -> std::uint64_t {
  return *reinterpret_cast<const std::uint64_t*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::exp);
}

auto cic_player::sp() const -> std::uint32_t {
  return *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::sp);
}

auto cic_player::gold() const -> std::uint64_t {
  if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::cic_player::globals::gold), sizeof(std::uint64_t))) {
    return 0;
  }
  return *reinterpret_cast<const std::uint64_t*>(ext_client::offsets::cic_player::globals::gold);
}

auto cic_player::strength() const -> std::uint16_t {
  return *reinterpret_cast<const std::uint16_t*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::strength);
}

auto cic_player::intelligence() const -> std::uint16_t {
  return *reinterpret_cast<const std::uint16_t*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::intelligence);
}

auto cic_player::attribute_points() const -> std::uint32_t {
  return *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const char*>(this) + ext_client::offsets::cic_player::fields::attribute_points);
}
