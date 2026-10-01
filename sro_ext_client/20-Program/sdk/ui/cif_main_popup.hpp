#pragma once

#include "sdk/ui/cif_wnd.hpp"
#include "sdk/types/alram_entry.hpp"
#include "sdk/types/alram_data.hpp"

#include <cstddef>
#include <cstdint>

inline constexpr std::size_t calram_entry_count = 13;

class cg_interface;

class cif_main_popup : public cif_wnd {
public:
  auto get_alarm_data() -> alram_data*;
  auto get_alram() -> alram_data*;
  auto get_alram() const -> const alram_data*;

  auto set_alarm_data(alram_data* val) -> void;

  static auto from_interface(cg_interface* iface) -> cif_main_popup*;
};
