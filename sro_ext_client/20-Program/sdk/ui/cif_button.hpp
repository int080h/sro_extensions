#pragma once

#include "sdk/ui/cif_static.hpp"

// ---------------------------------------------------------------------------
// CIFButton — Clickable button widget (extends CIFStatic)
// Native VTable: 0x00FF48D4 | Class Size: 0x3E8
// ---------------------------------------------------------------------------
class cif_button : public cif_static {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x00FF48D4;
  static constexpr std::size_t   k_class_size  = 0x03E8;
};
