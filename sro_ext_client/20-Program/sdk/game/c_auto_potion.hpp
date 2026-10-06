#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// CAutoPotion — Native client auto potion & recovery manager
// Instance pointer: *(void**)(*(void**)0x013BAE3C + 0x700)
// Manages potion slots, debounce timers, and auto-consumption.
// ---------------------------------------------------------------------------
class c_auto_potion {
public:
  static constexpr std::uint32_t k_ginterface_addr = 0x013BAE3C;
  static constexpr std::uint32_t k_autopotion_offset = 0x700;

  // 1. Singleton Access
  [[nodiscard]] static auto get_instance() -> c_auto_potion*;

  // 2. Direct Potion & Pill Actions
  auto use_hp_potion() -> bool;
  auto use_mp_potion() -> bool;
  auto use_universal_pill() -> bool;
  auto use_vigor_potion() -> bool;

  // 3. Native Periodic Evaluation Handlers
  auto tick_hp() -> bool;
  auto tick_mp() -> bool;
  auto tick_pill() -> bool;
  auto tick_vigor() -> bool;

  // 4. Native Configuration & Thresholds
  [[nodiscard]] auto is_hp_enabled() const -> bool;
  auto set_hp_enabled(bool enabled) -> void;
  [[nodiscard]] auto hp_threshold() const -> std::uint32_t;
  auto set_hp_threshold(std::uint32_t val) -> void;

  [[nodiscard]] auto is_mp_enabled() const -> bool;
  auto set_mp_enabled(bool enabled) -> void;
  [[nodiscard]] auto mp_threshold() const -> std::uint32_t;
  auto set_mp_threshold(std::uint32_t val) -> void;

  [[nodiscard]] auto is_pill_enabled() const -> bool;
  auto set_pill_enabled(bool enabled) -> void;

  [[nodiscard]] auto is_vigor_enabled() const -> bool;
  auto set_vigor_enabled(bool enabled) -> void;
  [[nodiscard]] auto vigor_threshold() const -> std::uint32_t;
  auto set_vigor_threshold(std::uint32_t val) -> void;

private:
  c_auto_potion() = delete;
  ~c_auto_potion() = delete;
};
