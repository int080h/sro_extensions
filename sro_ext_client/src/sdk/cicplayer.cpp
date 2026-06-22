#include "sdk/cicplayer.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

auto cic_player::local() -> cic_player* {
  return *reinterpret_cast<cic_player**>(ext_client::offsets::cic_player::globals::local_player);
}

auto cic_player::name() const -> const wchar_t* {
  return m_display_name.data();
}

auto cic_player::guild_name() const -> const wchar_t* {
  return m_user_guild_name.data();
}

auto cic_player::is_gamemaster() const -> bool {
  return m_gamemaster == 0x10001;
}

auto cic_player::level() const -> std::uint8_t {
  return m_level;
}

auto cic_player::exp() const -> std::uint64_t {
  return m_exp;
}

auto cic_player::sp() const -> std::uint32_t {
  return m_sp;
}

auto cic_player::gold() const -> std::uint64_t {
  return *reinterpret_cast<const std::uint64_t*>(ext_client::offsets::cic_player::globals::gold);
}

auto cic_player::strength() const -> std::uint16_t {
  return m_strength;
}

auto cic_player::intelligence() const -> std::uint16_t {
  return m_intelligence;
}

auto cic_player::attribute_points() const -> std::uint32_t {
  return m_stat_points;
}
