#pragma once

#include "cif_static.hpp"
#include "utils/offsets.hpp"

// CIFButton: clickable static (resinfo buttons). Size 0x3E8; same layout prefix as CIFStatic.
class cif_button : public cif_static {
public:
  DECLARE_SDK_WND_HELPERS(cif_button, "CIFButton")
};
