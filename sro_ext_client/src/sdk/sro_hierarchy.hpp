#pragma once

// Silkroad class hierarchy — use as_*() to view each inheritance slice.

#include "cgwnd_base.hpp"
#include "cif_wnd.hpp"
#include "cnif_page_manager.hpp"
#include "cnif_wnd.hpp"
#include "cobj.hpp"
#include "centity_manager.hpp"
#include "centity_manager_client.hpp"
#include "cprocess.hpp"
#include "cps_outer_interface.hpp"
#include "cps_silkroad.hpp"
#include "cgwnd.hpp"
#include "ctextboard.hpp"

struct cps_title;
struct cps_version_check;
struct cps_character_select;
struct cg_interface;

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

  auto as_cif_wnd(void* obj) -> cif_wnd*;
  auto as_ctextboard(void* obj) -> ctextboard*;
  auto as_cg_interface(void* obj) -> cg_interface*;
  auto as_cnif_wnd(void* obj) -> cnif_wnd*;
  auto as_cnif_page_manager(void* obj) -> cnif_page_manager*;
  auto as_entity_manager(void* obj) -> centity_manager*;
  auto as_entity_manager_client(void* obj) -> centity_manager_client*;

} // namespace ext_client::sdk::hierarchy
