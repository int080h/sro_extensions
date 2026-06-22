#pragma once

#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"
#include <cstdint>

struct SLocalTime {
  std::uint32_t dwRealTime;
  std::uint16_t wDay;
  std::uint8_t byHour;
  std::uint8_t byMin;
};

#pragma warning(push)
#pragma warning(disable: 4624) // destructor was implicitly defined as deleted

class cclient_config {
public:
  union {
    // Offset 0x30: Media path / base directory (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_media_path, 0x30);
    
    // Offset 0x44: Local time structure (SLocalTime)
    DEFINE_MEMBER_N(SLocalTime m_local_time, 0x44);

    // Offset 0x4C: Selected server name (std::wstring)
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_selected_server, 0x4C);

    // Offset 0x10C: Data version (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_data_version, 0x10C);

    // Offset 0x148: Debug mode flag (bool/uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_debug_mode, 0x148);

    // Offset 0x149: Debug show FPS flag (bool/uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_debug_show_fps, 0x149);

    // Offset 0x14C: LOD Y scale factor (float)
    DEFINE_MEMBER_N(float m_lod_scale, 0x14C);

    // Offset 0x150: LOD Y offset factor (float)
    DEFINE_MEMBER_N(float m_lod_offset, 0x150);

    // Offset 0x154: Debug printed text/info (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_debug_info, 0x154);

    // Offset 0x538: Window mode (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_window_mode, 0x538);

    // Offset 0x53C: Width of window/resolution (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_width, 0x53C);

    // Offset 0x540: Height of window/resolution (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_height, 0x540);

    // Offset 0x544: Color depth (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_color_depth, 0x544);

    // Offset 0x545: Shadow mode (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_shadow_mode, 0x545);

    // Offset 0x546: Sound enabled flag (uint8_t/bool)
    DEFINE_MEMBER_N(std::uint8_t m_sound_enabled, 0x546);

    // Offset 0x547: Music enabled flag (uint8_t/bool)
    DEFINE_MEMBER_N(std::uint8_t m_music_enabled, 0x547);

    // Offset 0x548: Texture detail level (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_texture_detail, 0x548);

    // Offset 0x549: Shadow detail level (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_shadow_detail, 0x549);

    // Offset 0x54A: View distance level (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_view_distance, 0x54A);

    // Offset 0x728: Account Login ID (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_login_id, 0x728);

    // Offset 0x780: Gateway server IP address (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_server_ip, 0x780);

    // Offset 0x79C: Game type string (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_game_type, 0x79C);

    // Offset 0x7B8: Game type ID (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_game_type_id, 0x7B8);

    // Offset 0x822: Gateway server port (uint16_t)
    DEFINE_MEMBER_N(std::uint16_t m_gateport, 0x822);

    // Offset 0x824: Client version (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_version, 0x824);

    // Offset 0x848: Client locale ID (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_locale, 0x848);

    // Offset 0x84C: Division Index (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_division_index, 0x84C);

    // Offset 0x850: Server Index (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_server_index, 0x850);

    // Offset 0x85A: HackShield usage flag (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_use_hackshield, 0x85A);

    // Offset 0x85D: Locale flag (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_locale_flag, 0x85D);

    // Offset 0x85F: Country/Locale visual flag (uint8_t)
    DEFINE_MEMBER_N(std::uint8_t m_country_flag, 0x85F);

    // Offset 0x888: Start character ID (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_start_character_id, 0x888);

    // Offset 0x88C: Start weapon ID (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_start_weapon_id, 0x88C);

    // Offset 0x890: Selected language ID (uint32_t)
    DEFINE_MEMBER_N(std::uint32_t m_language, 0x890);

    // Offset 0x898: GameGuard DLL path (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_gameguard, 0x898);

    // Offset 0x8B4: Guild mark FTP server address (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_mark_ftp_addr, 0x8B4);

    // Offset 0x8D0: Guild mark FTP directory path (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_mark_ftp_path, 0x8D0);

    // Offset 0x8EC: Web mall portal address (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_web_mall_addr, 0x8EC);

    // Offset 0x908: Secondary web mall address (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_web_mall_addr2, 0x908);

    // Offset 0x924: In-game SRO web address (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_sro_ingame_addr, 0x924);

    // Offset 0x940: Web magic lamp address (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_web_magic_lamp_addr, 0x940);

    // Offset 0x95C: Web daily login reward address (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_web_daily_login_addr, 0x95C);

    // Offset 0x978: Intro client screen background DDJ name (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_intro_name, 0x978);

    // Offset 0x994: Intro background music track name (std::string)
    DEFINE_MEMBER_N(ext_client::msvc9::string m_intro_bgm, 0x994);
  };
};

#pragma warning(pop)
