#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// CGlobalDataManager — Static game data tables loaded from textdata (refitem, magic options,
//                      devil spirit option tables)
// Singleton: 0x0117EE20 | Native VTable: 0x0103DC3C (??_7CGlobalDataManager@@6B@)
//
// Natives wrapped here (all __thiscall on the singleton, every call is SEH guarded):
//   sub_A93E20  GetRefItem(ref_id)               -> CRefObjItem*
//   sub_A757A0  FindMagicOption(opt_id)          -> node,  sub_AC8CF0(node) -> option info
//   sub_A80130  ResolveItemTypeKey(tid, code)    -> UIIT_* key of the "Sort of item" label
//   sub_A742A0  FindDevilGroup(ref_id)           -> per-spirit level table
//   sub_9DAEE0  GetLevelSlot(group, opt_level)   -> slot, +4 = option list key
//   sub_A74300  FindDevilOptionList(list_key)    -> vector of 36-byte entries (+0x14 / +0x18)
// ---------------------------------------------------------------------------
class cglobal_data_manager {
public:
  static constexpr std::uintptr_t k_singleton_addr = 0x0117EE20;

  // One decoded entry of a Devil's Spirit option list (36 bytes natively):
  //   u16 option id @0, u16 value @2, u8 group @4 (0 basic / 1 additional / 2 magic),
  //   MSVC9 wstring text @8 (a translation key, or "xxx" = format from the option definition)
  struct devil_option_entry {
    std::uint16_t opt_id{0};
    std::uint16_t value{0};
    std::uint8_t  group{0};
    bool          generic{false};
    wchar_t       key[64]{};
  };

  // --- 1. Reference items ---
  static auto get_ref_item(std::uint32_t ref_id) -> void*;

  // --- 1B. Reference characters / NPCs / Pets (sub_A93CF0) ---
  static auto get_ref_char(std::uint32_t char_id) -> void*;

  // --- 2. Magic option definitions (name is the MATTR_* identifier) ---
  static auto find_magic_option_name(std::uint16_t opt_id) -> std::wstring;

  // --- 3. Item type label key (code name + type id -> UIIT_* key) ---
  static auto resolve_item_type_key(std::uint32_t type_id, const void* code_name_wstring,
                                    wchar_t* out_key, std::size_t out_capacity) -> bool;

  // --- 4. Devil's Spirit option lists ---
  static auto query_devil_options(std::uint32_t ref_id, std::uint32_t opt_level,
                                  devil_option_entry* out, int capacity) -> int;
};
