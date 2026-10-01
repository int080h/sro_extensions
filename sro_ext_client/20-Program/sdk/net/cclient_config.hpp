#pragma once

#include "utils/msvc9_stl.hpp"
#include "sdk/types/s_local_time.hpp"
#include <cstdint>

#pragma warning(push)
#pragma warning(disable: 4624) // destructor was implicitly defined as deleted

class cclient_config {
public:
  auto is_sound_enabled() -> bool;
  auto is_music_enabled() -> bool;
  auto is_locale_flag() -> bool;
  auto is_country_flag() -> bool;

  auto get_media_path() -> ext_client::msvc9::string;
  auto get_local_time() -> s_local_time;
  auto get_selected_server() -> ext_client::msvc9::wstring;
  auto get_data_version() -> std::uint32_t;
  auto get_debug_mode() -> std::uint8_t;
  auto get_debug_show_fps() -> std::uint8_t;
  auto get_lod_scale() -> float;
  auto get_lod_offset() -> float;
  auto get_debug_info() -> ext_client::msvc9::string;
  auto get_window_mode() -> std::uint32_t;
  auto get_width() -> std::uint32_t;
  auto get_height() -> std::uint32_t;
  auto get_color_depth() -> std::uint8_t;
  auto get_shadow_mode() -> std::uint8_t;
  auto get_texture_detail() -> std::uint8_t;
  auto get_shadow_detail() -> std::uint8_t;
  auto get_view_distance() -> std::uint8_t;
  auto get_login_id() -> ext_client::msvc9::string;
  auto get_server_ip() -> ext_client::msvc9::string;
  auto get_game_type() -> ext_client::msvc9::string;
  auto get_game_type_id() -> std::uint32_t;
  auto get_gateport() -> std::uint16_t;
  auto get_version() -> std::uint32_t;
  auto get_locale() -> std::uint32_t;
  auto get_division_index() -> std::uint32_t;
  auto get_server_index() -> std::uint32_t;
  auto get_use_hackshield() -> std::uint8_t;
  auto get_start_character_id() -> std::uint32_t;
  auto get_start_weapon_id() -> std::uint32_t;
  auto get_language() -> std::uint32_t;
  auto get_gameguard() -> ext_client::msvc9::string;
  auto get_mark_ftp_addr() -> ext_client::msvc9::string;
  auto get_mark_ftp_path() -> ext_client::msvc9::string;
  auto get_web_mall_addr() -> ext_client::msvc9::string;
  auto get_web_mall_addr2() -> ext_client::msvc9::string;
  auto get_sro_ingame_addr() -> ext_client::msvc9::string;
  auto get_web_magic_lamp_addr() -> ext_client::msvc9::string;
  auto get_web_daily_login_addr() -> ext_client::msvc9::string;
  auto get_intro_name() -> ext_client::msvc9::string;
  auto get_intro_bgm() -> ext_client::msvc9::string;

  auto set_sound_enabled(bool val) -> void;
  auto set_music_enabled(bool val) -> void;
  auto set_locale_flag(bool val) -> void;
  auto set_country_flag(bool val) -> void;
  auto set_media_path(ext_client::msvc9::string val) -> void;
  auto set_local_time(s_local_time val) -> void;
  auto set_selected_server(ext_client::msvc9::wstring val) -> void;
  auto set_data_version(std::uint32_t val) -> void;
  auto set_debug_mode(std::uint8_t val) -> void;
  auto set_debug_show_fps(std::uint8_t val) -> void;
  auto set_lod_scale(float val) -> void;
  auto set_lod_offset(float val) -> void;
  auto set_debug_info(ext_client::msvc9::string val) -> void;
  auto set_window_mode(std::uint32_t val) -> void;
  auto set_width(std::uint32_t val) -> void;
  auto set_height(std::uint32_t val) -> void;
  auto set_color_depth(std::uint8_t val) -> void;
  auto set_shadow_mode(std::uint8_t val) -> void;
  auto set_texture_detail(std::uint8_t val) -> void;
  auto set_shadow_detail(std::uint8_t val) -> void;
  auto set_view_distance(std::uint8_t val) -> void;
  auto set_login_id(ext_client::msvc9::string val) -> void;
  auto set_server_ip(ext_client::msvc9::string val) -> void;
  auto set_game_type(ext_client::msvc9::string val) -> void;
  auto set_game_type_id(std::uint32_t val) -> void;
  auto set_gateport(std::uint16_t val) -> void;
  auto set_version(std::uint32_t val) -> void;
  auto set_locale(std::uint32_t val) -> void;
  auto set_division_index(std::uint32_t val) -> void;
  auto set_server_index(std::uint32_t val) -> void;
  auto set_use_hackshield(std::uint8_t val) -> void;
  auto set_start_character_id(std::uint32_t val) -> void;
  auto set_start_weapon_id(std::uint32_t val) -> void;
  auto set_language(std::uint32_t val) -> void;
  auto set_gameguard(ext_client::msvc9::string val) -> void;
  auto set_mark_ftp_addr(ext_client::msvc9::string val) -> void;
  auto set_mark_ftp_path(ext_client::msvc9::string val) -> void;
  auto set_web_mall_addr(ext_client::msvc9::string val) -> void;
  auto set_web_mall_addr2(ext_client::msvc9::string val) -> void;
  auto set_sro_ingame_addr(ext_client::msvc9::string val) -> void;
  auto set_web_magic_lamp_addr(ext_client::msvc9::string val) -> void;
  auto set_web_daily_login_addr(ext_client::msvc9::string val) -> void;
  auto set_intro_name(ext_client::msvc9::string val) -> void;
  auto set_intro_bgm(ext_client::msvc9::string val) -> void;
};

#pragma warning(pop)
