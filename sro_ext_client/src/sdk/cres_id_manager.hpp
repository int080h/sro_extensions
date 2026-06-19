#pragma once

#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class cres_id_manager;

// CResIDManager — UI resource ID → widget map (sub_9D0190), 0x30 bytes in game memory.

class cres_id_manager {
public:
  static auto is_instance(const void* ptr) -> bool;
  static auto from(void* ptr) -> cres_id_manager*;
  static auto from(const void* ptr) -> const cres_id_manager*;

  auto find(int res_id, bool add_base_key = true) const -> void*;
  auto raw() const -> const void* { return storage_; }
  auto map_view() const -> ext_client::msvc9::map_ref;

private:
  alignas(4) std::uint8_t storage_[ext_client::msvc9::ui_res_map_size]{};
};

static_assert(sizeof(cres_id_manager) == ext_client::msvc9::ui_res_map_size, "cres_id_manager size mismatch");
