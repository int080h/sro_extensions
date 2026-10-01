#pragma once

#include <cstddef>
#include <cstdint>

struct alram_entry {
  auto is_active() const -> bool;
  auto is_facebook() const -> bool;
  auto is_magic_lamp() const -> bool;
  auto is_daily_login() const -> bool;
  auto is_hidden() const -> bool;

  auto get_type() const -> int;
  auto get_ref_data_ptr() -> void*;

  auto set_active(bool active) -> void;
  auto set_type(int type) -> void;
};
