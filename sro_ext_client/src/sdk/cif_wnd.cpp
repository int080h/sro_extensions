#include "sdk/cif_wnd.hpp"

#include "sdk/ctextboard.hpp"
#include "utils/offsets.hpp"

auto cif_wnd::textboard() -> ctextboard* {
  return reinterpret_cast<ctextboard*>(reinterpret_cast<std::uint8_t*>(this) +
                                       ext_client::offsets::cif_wnd::fields::textboard_vftable);
}
