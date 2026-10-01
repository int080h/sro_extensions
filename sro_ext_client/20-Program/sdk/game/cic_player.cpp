#include "pch.hpp"
#include "sdk/game/cic_player.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

auto cic_player::local() -> cic_player* {
  return *reinterpret_cast<cic_player**>(0x01199114);
}

auto cic_player::name() const -> const wchar_t* {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8CC).data();
}

auto cic_player::guild_name() const -> const wchar_t* {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x8FC).data();
}

auto cic_player::is_gamemaster() const -> bool {
  return ext_client::off::field_at<std::uint32_t>(this, 0x3A58) == 0x10001;
}

auto cic_player::level() const -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0xA14);
}

auto cic_player::exp() const -> std::uint64_t {
  return ext_client::off::field_at<std::uint64_t>(this, 0xA18);
}

auto cic_player::sp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0xA20);
}

auto cic_player::gold() const -> std::uint64_t {
  return *reinterpret_cast<const std::uint64_t*>(0x0119B610);
}

auto cic_player::strength() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0xA24);
}

auto cic_player::intelligence() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0xA26);
}

auto cic_player::attribute_points() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0xA28);
}
