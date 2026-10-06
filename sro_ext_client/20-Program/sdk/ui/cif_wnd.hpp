#pragma once

#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/ctext_board.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CIFWnd — Base In-Game Interface Window (CGWnd + CTextBoard @ +0x84)
// Native VTable: 0x00FF46BC | Class Size: 0x394
// ---------------------------------------------------------------------------
using ui_res_map_t = ext_client::msvc9::n_map<int, void*>;

class cif_wnd : public cgwnd, public ctext_board {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x00FF46BC;
  static constexpr std::size_t   k_class_size  = 0x0394;

  // 1. Child UI Resources
  auto get_ui_res_map() -> ui_res_map_t*;
  auto get_ui_res_map() const -> const ui_res_map_t*;

  // 2. Embedded Subobjects
  auto get_textboard() -> ctext_board*;
  auto get_textboard() const -> const ctext_board*;
};
