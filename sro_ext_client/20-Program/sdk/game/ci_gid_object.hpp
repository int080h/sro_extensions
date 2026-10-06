#pragma once

#include "sdk/game/ci_object.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CIGidObject — Graphical ID Game Object base class
// Extends CIObject (+0x84 MI) | Base for CICharactor
// Manages resource string tags, names, view distances, and fade timers.
// ---------------------------------------------------------------------------
class ci_gid_object : public ci_object {
public:
  // 1. Resource Names & Tags
  auto get_res_name() const -> ext_client::msvc9::wstring;
  auto get_tag_name() const -> ext_client::msvc9::wstring;
  auto get_status_name() const -> ext_client::msvc9::wstring;
  auto get_display_name() const -> ext_client::msvc9::wstring;
  auto get_gid_tag() const -> ext_client::msvc9::wstring;
  auto get_tag_list_size() const -> std::size_t;
  auto get_tag_list_cap() const -> std::size_t;

  auto set_res_name(ext_client::msvc9::wstring val) -> void;
  auto set_tag_name(ext_client::msvc9::wstring val) -> void;
  auto set_status_name(ext_client::msvc9::wstring val) -> void;
  auto set_display_name(ext_client::msvc9::wstring val) -> void;
  auto set_gid_tag(ext_client::msvc9::wstring val) -> void;
  auto set_tag_list_size(std::size_t val) -> void;
  auto set_tag_list_cap(std::size_t val) -> void;

  // 2. View Distance & Fade Timers
  auto get_view_distance() const -> float;
  auto get_fade_timer() const -> float;
  auto get_fade_speed() const -> float;

  auto set_view_distance(float val) -> void;
  auto set_fade_timer(float val) -> void;
  auto set_fade_speed(float val) -> void;
};
