#pragma once

#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "sdk/cic_deco_character.hpp"

#include <cstdint>

template<typename T> struct blend_variable {
  void* vftable;
  union {
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[36 - sizeof(void*)], "pad_end");
  };
};

struct cg_font_texture {
  void* vftable;
  union {
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[132 - sizeof(void*)], "pad_end");
  };
};

struct s_equipped_item {
  std::uint32_t model_id;
  std::uint8_t opt_level;
  std::uint8_t pad[7];
};

struct pcinfo_ui {
  pcinfo_ui() {}
  ~pcinfo_ui() {}
  union {
    DEFINE_MEMBER_0(std::uint32_t model_id, "model_id");                              // 0x000
    DEFINE_MEMBER_N(ext_client::msvc9::wstring char_name, 0x004);                     // 0x004
    DEFINE_MEMBER_N(ext_client::msvc9::wstring guild_name, 0x020);                    // 0x020
    DEFINE_MEMBER_N(std::uint8_t level, 0x03C);                                       // 0x03C
    DEFINE_MEMBER_N(std::uint8_t style, 0x03D);                                       // 0x03D
    DEFINE_MEMBER_N(std::uint64_t exp, 0x040);                                        // 0x040
    DEFINE_MEMBER_N(std::uint16_t strength, 0x048);                                   // 0x048
    DEFINE_MEMBER_N(std::uint16_t intelligence, 0x04A);                               // 0x04A
    DEFINE_MEMBER_N(std::uint16_t stat_points, 0x04C);                                // 0x04C
    DEFINE_MEMBER_N(std::uint32_t hp, 0x050);                                         // 0x050
    DEFINE_MEMBER_N(std::uint32_t mp, 0x054);                                         // 0x054
    DEFINE_MEMBER_N(std::uint8_t deleting, 0x058);                                    // 0x058
    DEFINE_MEMBER_N(std::uint32_t deletion_time, 0x05C);                              // 0x05C
    DEFINE_MEMBER_N(std::uint8_t style2, 0x060);                                      // 0x060
    DEFINE_MEMBER_N(ext_client::msvc9::wstring guild_name2, 0x064);                   // 0x064
    DEFINE_MEMBER_N(s_equipped_item equipped_items[9], 0x080);                        // 0x080
    DEFINE_MEMBER_N(ext_client::msvc9::vector<std::uint32_t> avatar_ids, 0x0EC);      // 0x0EC
    DEFINE_MEMBER_N(std::uint32_t packed_time, 0x0FC);                                // 0x0FC
    DEFINE_MEMBER_N(std::uint16_t unknown_100, 0x100);                                // 0x100
    DEFINE_MEMBER_N(std::uint32_t unknown_104, 0x104);                                // 0x104
    DEFINE_MEMBER_N(blend_variable<std::uint8_t> blend_var1, 0x10C);                  // 0x10C
    DEFINE_MEMBER_N(blend_variable<std::uint8_t> blend_var2, 0x130);                  // 0x130
    DEFINE_MEMBER_N(cg_font_texture font_texture, 0x154);                             // 0x154
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[0x1D8], "pad_end");                        // total size 0x1D8
  };
};

