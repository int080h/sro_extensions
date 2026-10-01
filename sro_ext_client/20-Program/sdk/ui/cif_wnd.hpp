#pragma once

#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/ctext_board.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// CIFWnd — CGWnd + CTextBoard @ +0x84, n_map<int,void*> @ +0x1C4. Derived IF controls @ +0x394.
using ui_res_map_t = ext_client::msvc9::n_map<int, void*>;

class cif_wnd : public cgwnd, public ctext_board {
public:
  auto get_ui_res_map() -> ui_res_map_t*;
  auto get_ui_res_map() const -> const ui_res_map_t*;
  auto get_textboard() -> ctext_board*;
  auto get_textboard() const -> const ctext_board*;

  auto set_ui_res_map(ui_res_map_t val) -> void;
};
