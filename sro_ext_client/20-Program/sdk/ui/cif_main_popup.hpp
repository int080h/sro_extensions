#pragma once

#include "sdk/ui/cif_wnd.hpp"
#include "sdk/ui/alram_data.hpp"
#include "sdk/ui/alram_entry.hpp"

#include <cstddef>
#include <cstdint>

inline constexpr std::size_t calram_entry_count = 13;

class cg_interface;

// ---------------------------------------------------------------------------
// CIFMainPopup — Main popup window managing notifications and alarm items
// Child of CGInterface; owns the alarm container alram_data
// ---------------------------------------------------------------------------
class cif_main_popup : public cif_wnd {
public:
  // 1. Instance Resolution
  static auto from_interface(cg_interface* iface) -> cif_main_popup*;

  // 2. Alarm Data Accessors
  auto get_alarm_data() -> alram_data*;
  auto get_alram() -> alram_data*;
  auto get_alram() const -> const alram_data*;
  auto set_alarm_data(alram_data* val) -> void;
};
