#pragma once






#include <cstddef>

#include <cstdint>



class sworld;



// SWorld — in-game world / terrain / graphics state (g_sw @ 0x013AB480).

// Per-field offsets: ext_client::off::sworld in offsets/sworld.hpp

class sworld {

public:

  static auto instance() -> sworld*;

  static auto is_instance() -> bool;



  auto cell_limit() const -> std::int32_t;

  auto sight_range() const -> float;

  auto option(unsigned int index) const -> std::int32_t;



  auto set_cell_limit(std::int32_t limit) -> void;

  auto set_sight_range(float range) -> void;

  auto set_option(unsigned int index, std::int32_t value) -> void;

  auto set_graphics_option(unsigned int setting_id, int value) -> int;



private:


};

