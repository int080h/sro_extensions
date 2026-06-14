#pragma once

// Silkroad outer-process class hierarchy (virtual inheritance, all bases at mdisp 0).
//
//   CPSCharacterSelect    vft 0x103298C  (41)  size 0x240
//   CPSTitle              vft 0x10344F4  (41)  size 0x218
//     CPSOuterInterface   vft 0x1032E34  (41)  shared ends 0x10C
//       CPSilkroad        vft 0x102EFFC  (41)  slice 0xB0..0xEF
//         CProcess        vft 0x1068A2C  (40)  slice 0x84..0xAF
//           CGWnd         vft 0x106850C  (38)  fields 0x2C..0x83
//             CGWndBase   vft 0x106874C  (25)  slice 0x20..0x2B
//               cobj_child vft 0xFE2A54   (24)  fields 0x0C..0x1F
//                 cobj    vft 0xFE2654   (3)   prefix 0x00..0x0B
//
// Runtime vftable at +0x00 is the most-derived type. Use as_*() to view each slice.

#include "cgwnd_base.hpp"
#include "cobj.hpp"
#include "cprocess.hpp"
#include "cps_outer_interface.hpp"
#include "cps_silkroad.hpp"
#include "cgwnd.hpp"

struct cps_title;
struct cps_version_check;
struct cps_character_select;

namespace ext_client::sdk::hierarchy {

  template<typename To, typename From> auto cast(From* ptr) -> To* {
    return reinterpret_cast<To*>(ptr);
  }

  template<typename To, typename From> auto cast(const From* ptr) -> const To* {
    return reinterpret_cast<const To*>(ptr);
  }

  template<typename To, typename From> inline auto cast_offset(From* ptr, std::size_t offset) -> To* {
    return reinterpret_cast<To*>(reinterpret_cast<std::uint8_t*>(ptr) + offset);
  }

  template<typename To, typename From> inline auto cast_offset(const From* ptr, std::size_t offset) -> const To* {
    return reinterpret_cast<const To*>(reinterpret_cast<const std::uint8_t*>(ptr) + offset);
  }

  auto as_outer(cps_title* title) -> cps_outer_interface*;
  auto as_outer(cps_version_check* screen) -> cps_outer_interface*;
  auto as_outer(void* screen) -> cps_outer_interface*;

  auto as_cobj(void* obj) -> cobj*;
  auto as_cobj_child(void* obj) -> cobj_child*;
  auto as_gwnd_base(void* obj) -> cgwnd_base*;
  auto as_gwnd(void* obj) -> cgwnd*;
  auto as_cprocess(void* obj) -> cprocess*;
  auto as_silkroad(void* obj) -> cps_silkroad*;

  auto as_gwnd(cps_outer_interface* outer) -> cgwnd*;
  auto as_title(cps_outer_interface* outer) -> cps_title*;
  auto as_version_check(cps_outer_interface* outer) -> cps_version_check*;
  auto as_character_select(cps_outer_interface* outer) -> cps_character_select*;

} // namespace ext_client::sdk::hierarchy
