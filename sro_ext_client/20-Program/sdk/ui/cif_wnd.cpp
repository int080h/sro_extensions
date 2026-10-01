#include "pch.hpp"
#include "sdk/ui/cif_wnd.hpp"

#include "utils/offsets.hpp"

auto cif_wnd::get_ui_res_map() -> ui_res_map_t* {
  return &ext_client::off::field_at<ui_res_map_t>(this, 0x1C4);
}

auto cif_wnd::get_ui_res_map() const -> const ui_res_map_t* {
  return &ext_client::off::field_at<ui_res_map_t>(this, 0x1C4);
}

auto cif_wnd::get_textboard() -> ctext_board* {
  return static_cast<ctext_board*>(this);
}

auto cif_wnd::get_textboard() const -> const ctext_board* {
  return static_cast<const ctext_board*>(this);
}
