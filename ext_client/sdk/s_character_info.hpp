#pragma once

#include "utils/msvc9_stl.hpp"
#include "utils/layout.hpp"
#include "sdk/cic_deco_character.hpp"

#include <cstdint>

template<typename T> struct blend_variable {
  void* vftable;
  PAD(32); // size 36 bytes
};

struct cg_font_texture {
  void* vftable;
  PAD(128); // size 132 bytes
};

struct s_equipped_item {
  std::uint32_t model_id;
  std::uint8_t opt_level;
  std::uint8_t pad[7];
};

struct pcinfo_ui {
  std::uint32_t model_id;                              // 0x000
  ext_client::msvc9::wstring char_name;                // 0x004
  ext_client::msvc9::wstring guild_name;               // 0x020
  std::uint8_t level;                                  // 0x03C
  std::uint8_t style;                                  // 0x03D
  std::uint8_t pad_3e[2];                              // 0x03E
  std::uint64_t exp;                                   // 0x040
  std::uint16_t strength;                              // 0x048
  std::uint16_t intelligence;                          // 0x04A
  std::uint16_t stat_points;                           // 0x04C
  std::uint8_t pad_4e[2];                              // 0x04E
  std::uint32_t hp;                                    // 0x050
  std::uint32_t mp;                                    // 0x054
  std::uint8_t deleting;                               // 0x058
  std::uint8_t pad_59[3];                              // 0x059
  std::uint32_t deletion_time;                         // 0x05C, present when deleting != 0
  std::uint8_t style2;                                 // 0x060
  std::uint8_t pad_61[3];                              // 0x061
  ext_client::msvc9::wstring guild_name2;              // 0x064, only read when packet flag != 0
  s_equipped_item equipped_items[9];                   // 0x080 (size 108)
  ext_client::msvc9::vector<std::uint32_t> avatar_ids; // 0x0EC (size 16), packet stores id + 1-byte flag per entry
  std::uint32_t packed_time;                           // 0x0FC, server last-logout packed timestamp
  std::uint16_t unknown_100;                           // 0x100
  std::uint8_t pad_102[2];                             // 0x102
  std::uint32_t unknown_104;                           // 0x104
  std::uint8_t pad_108[4];                             // 0x108
  blend_variable<std::uint8_t> blend_var1;             // 0x10C (size 36)
  blend_variable<std::uint8_t> blend_var2;             // 0x130 (size 36)
  cg_font_texture font_texture;                        // 0x154 (size 132)
};

static_assert(offsetof(pcinfo_ui, model_id) == 0x000, "pcinfo_ui::model_id offset mismatch");
static_assert(offsetof(pcinfo_ui, char_name) == 0x004, "pcinfo_ui::char_name offset mismatch");
static_assert(offsetof(pcinfo_ui, guild_name) == 0x020, "pcinfo_ui::guild_name offset mismatch");
static_assert(offsetof(pcinfo_ui, level) == 0x03C, "pcinfo_ui::level offset mismatch");
static_assert(offsetof(pcinfo_ui, style) == 0x03D, "pcinfo_ui::style offset mismatch");
static_assert(offsetof(pcinfo_ui, exp) == 0x040, "pcinfo_ui::exp offset mismatch");
static_assert(offsetof(pcinfo_ui, strength) == 0x048, "pcinfo_ui::strength offset mismatch");
static_assert(offsetof(pcinfo_ui, intelligence) == 0x04A, "pcinfo_ui::intelligence offset mismatch");
static_assert(offsetof(pcinfo_ui, stat_points) == 0x04C, "pcinfo_ui::stat_points offset mismatch");
static_assert(offsetof(pcinfo_ui, hp) == 0x050, "pcinfo_ui::hp offset mismatch");
static_assert(offsetof(pcinfo_ui, mp) == 0x054, "pcinfo_ui::mp offset mismatch");
static_assert(offsetof(pcinfo_ui, deleting) == 0x058, "pcinfo_ui::deleting offset mismatch");
static_assert(offsetof(pcinfo_ui, deletion_time) == 0x05C, "pcinfo_ui::deletion_time offset mismatch");
static_assert(offsetof(pcinfo_ui, style2) == 0x060, "pcinfo_ui::style2 offset mismatch");
static_assert(offsetof(pcinfo_ui, guild_name2) == 0x064, "pcinfo_ui::guild_name2 offset mismatch");
static_assert(offsetof(pcinfo_ui, equipped_items) == 0x080, "pcinfo_ui::equipped_items offset mismatch");
static_assert(offsetof(pcinfo_ui, avatar_ids) == 0x0EC, "pcinfo_ui::avatar_ids offset mismatch");
static_assert(offsetof(pcinfo_ui, packed_time) == 0x0FC, "pcinfo_ui::packed_time offset mismatch");
static_assert(offsetof(pcinfo_ui, unknown_100) == 0x100, "pcinfo_ui::unknown_100 offset mismatch");
static_assert(offsetof(pcinfo_ui, unknown_104) == 0x104, "pcinfo_ui::unknown_104 offset mismatch");
static_assert(offsetof(pcinfo_ui, blend_var1) == 0x10C, "pcinfo_ui::blend_var1 offset mismatch");
static_assert(offsetof(pcinfo_ui, blend_var2) == 0x130, "pcinfo_ui::blend_var2 offset mismatch");
static_assert(offsetof(pcinfo_ui, font_texture) == 0x154, "pcinfo_ui::font_texture offset mismatch");
static_assert(sizeof(s_equipped_item) == 12, "s_equipped_item size mismatch");
static_assert(sizeof(blend_variable<std::uint8_t>) == 36, "blend_variable size mismatch");
static_assert(sizeof(cg_font_texture) == 132, "cg_font_texture size mismatch");
static_assert(sizeof(pcinfo_ui) == 0x1D8, "pcinfo_ui size mismatch");


