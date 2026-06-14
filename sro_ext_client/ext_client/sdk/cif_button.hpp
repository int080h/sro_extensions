#pragma once

#include "cif_static.hpp"
#include "utils/offsets.hpp"

// CIFButton: clickable static (resinfo buttons). Size 0x3E8; same layout prefix as CIFStatic.
class cif_button : public cif_static {
public:
  PAD_TO(ext_client::offsets::cif_static::size, ext_client::offsets::cif_button::size);

  DECLARE_SDK_WND_HELPERS(cif_button, ext_client::offsets::cif_button::vtable::address)
};

static_assert(sizeof(cif_button) == ext_client::offsets::cif_button::size, "cif_button size mismatch");
