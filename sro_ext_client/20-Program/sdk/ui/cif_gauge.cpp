#include "pch.hpp"
#include "sdk/ui/cif_gauge.hpp"

#include "sdk/runtime/rtti.hpp"
#include "utils/offsets.hpp"

#include <algorithm>

auto cif_gauge::get_current_ratio() const -> float {
  const float ratio = ext_client::off::field_at<float>(this, 0x398);
  return std::clamp(ratio, 0.0f, 1.0f);
}

auto cif_gauge::get_target_ratio() const -> float {
  const float ratio = ext_client::off::field_at<float>(this, 0x39C);
  return std::clamp(ratio, 0.0f, 1.0f);
}

auto cif_gauge::get_speed() const -> float {
  return ext_client::off::field_at<float>(this, 0x3A0);
}

auto cif_gauge::get_current_percent() const -> int {
  return static_cast<int>(get_current_ratio() * 100.0f + 0.5f);
}

auto cif_gauge::is_gauge(const void* wnd) -> bool {
  if (!wnd || !ext_client::msvc9::is_game_ptr(wnd)) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CIFGauge");
}
