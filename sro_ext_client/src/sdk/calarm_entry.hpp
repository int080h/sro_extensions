#pragma once

#include "utils/offsets.hpp"

#include <cstdint>

// One alarm record inside calarm_data (stride 0x1F8, IFMainPopup alarm table).
class calarm_entry {
public:
  auto active() const -> bool;
  auto type() const -> int;
  auto is_facebook() const -> bool;
  auto is_magic_lamp() const -> bool;
  auto is_daily_login() const -> bool;
  auto is_hidden() const -> bool;
  auto set_active(bool active) -> void;
  auto set_type(int type) -> void;
  auto ref_data_ptr() -> void*;

private:
  PAD(ext_client::offsets::calarm_entry::fields::active);
  std::uint8_t m_active;
  PAD_TO(ext_client::offsets::calarm_entry::fields::active + sizeof(std::uint8_t), ext_client::offsets::calarm_entry::fields::type);
  int m_type;
  PAD_TO(ext_client::offsets::calarm_entry::fields::type + sizeof(int), ext_client::offsets::calarm_entry::size);
};

static_assert(sizeof(calarm_entry) == ext_client::offsets::calarm_entry::size, "calarm_entry size mismatch");
