#include "pch.hpp"
#include "sdk/game/c_auto_potion.hpp"

#include "utils/memory.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::off::as_fn;
  using ext_client::off::field_at;
  using ext_client::off::global_at;
  using ext_client::utils::memory::is_game_ptr;

  using direct_use_fn = char(__thiscall*)(void* self, int a3, std::uint8_t a4, int a5);
  using tick_fn = char(__thiscall*)(void* self);
} // namespace

// ===========================================================================
// 1. Singleton Access
// ===========================================================================
auto c_auto_potion::get_instance() -> c_auto_potion* {
  auto* ginterface = global_at<std::uint8_t*>(k_ginterface_addr);
  if (!is_game_ptr(ginterface)) {
    return nullptr;
  }
  auto* pot = *reinterpret_cast<c_auto_potion**>(ginterface + k_autopotion_offset);
  return is_game_ptr(pot) ? pot : nullptr;
}


// ===========================================================================
// 2. Direct Potion & Pill Actions
// ===========================================================================
auto c_auto_potion::use_hp_potion() -> bool {
  const auto fn = as_fn<direct_use_fn>(0x009267A0);
  return fn ? (fn(this, 0, 0, 0) != 0) : false;
}

auto c_auto_potion::use_mp_potion() -> bool {
  const auto fn = as_fn<direct_use_fn>(0x009267A0);
  return fn ? (fn(this, 0, 1, 0) != 0) : false;
}

auto c_auto_potion::use_universal_pill() -> bool {
  const auto fn = as_fn<direct_use_fn>(0x009267A0);
  return fn ? (fn(this, 0, 2, 0) != 0) : false;
}

auto c_auto_potion::use_vigor_potion() -> bool {
  const auto fn = as_fn<direct_use_fn>(0x009267A0);
  return fn ? (fn(this, 1, 0, 0) != 0) : false;
}

// ===========================================================================
// 3. Native Periodic Evaluation Handlers
// ===========================================================================
auto c_auto_potion::tick_hp() -> bool {
  const auto fn = as_fn<tick_fn>(0x00926920);
  return fn ? (fn(this) != 0) : false;
}

auto c_auto_potion::tick_mp() -> bool {
  const auto fn = as_fn<tick_fn>(0x009269A0);
  return fn ? (fn(this) != 0) : false;
}

auto c_auto_potion::tick_pill() -> bool {
  const auto fn = as_fn<tick_fn>(0x00926A10);
  return fn ? (fn(this) != 0) : false;
}

auto c_auto_potion::tick_vigor() -> bool {
  const auto fn = as_fn<tick_fn>(0x00926A90);
  return fn ? (fn(this) != 0) : false;
}

// ===========================================================================
// 4. Native Configuration & Thresholds
// ===========================================================================
auto c_auto_potion::is_hp_enabled() const -> bool {
  return field_at<std::uint8_t>(this, 0x04) != 0;
}

auto c_auto_potion::set_hp_enabled(bool enabled) -> void {
  field_at<std::uint8_t>(this, 0x04) = enabled ? 1 : 0;
}

auto c_auto_potion::hp_threshold() const -> std::uint32_t {
  return field_at<std::uint32_t>(this, 0x08);
}

auto c_auto_potion::set_hp_threshold(std::uint32_t val) -> void {
  field_at<std::uint32_t>(this, 0x08) = val;
}

auto c_auto_potion::is_mp_enabled() const -> bool {
  return field_at<std::uint8_t>(this, 0x10) != 0;
}

auto c_auto_potion::set_mp_enabled(bool enabled) -> void {
  field_at<std::uint8_t>(this, 0x10) = enabled ? 1 : 0;
}

auto c_auto_potion::mp_threshold() const -> std::uint32_t {
  return field_at<std::uint32_t>(this, 0x14);
}

auto c_auto_potion::set_mp_threshold(std::uint32_t val) -> void {
  field_at<std::uint32_t>(this, 0x14) = val;
}

auto c_auto_potion::is_pill_enabled() const -> bool {
  return field_at<std::uint8_t>(this, 0x1C) != 0;
}

auto c_auto_potion::set_pill_enabled(bool enabled) -> void {
  field_at<std::uint8_t>(this, 0x1C) = enabled ? 1 : 0;
}

auto c_auto_potion::is_vigor_enabled() const -> bool {
  return field_at<std::uint8_t>(this, 0x28) != 0;
}

auto c_auto_potion::set_vigor_enabled(bool enabled) -> void {
  field_at<std::uint8_t>(this, 0x28) = enabled ? 1 : 0;
}

auto c_auto_potion::vigor_threshold() const -> std::uint32_t {
  return field_at<std::uint32_t>(this, 0x2C);
}

auto c_auto_potion::set_vigor_threshold(std::uint32_t val) -> void {
  field_at<std::uint32_t>(this, 0x2C) = val;
}
