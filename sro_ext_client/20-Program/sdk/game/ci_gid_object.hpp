#pragma once

#include "sdk/game/ci_object.hpp"
#include "utils/msvc9_stl.hpp"

class ci_gid_object : public ci_object {
public:
  auto get_res_name() -> ext_client::msvc9::wstring;
  auto get_tag_name() -> ext_client::msvc9::wstring;
  auto get_status_name() -> ext_client::msvc9::wstring;
  auto get_display_name() -> ext_client::msvc9::wstring;
  auto get_gid_tag() -> ext_client::msvc9::wstring;
  auto get_tag_list_size() -> size_t;
  auto get_tag_list_cap() -> size_t;
  auto get_view_distance() -> float;
  auto get_fade_timer() -> float;
  auto get_fade_speed() -> float;
  auto set_res_name(ext_client::msvc9::wstring val) -> void;
  auto set_tag_name(ext_client::msvc9::wstring val) -> void;
  auto set_status_name(ext_client::msvc9::wstring val) -> void;
  auto set_display_name(ext_client::msvc9::wstring val) -> void;
  auto set_gid_tag(ext_client::msvc9::wstring val) -> void;
  auto set_tag_list_size(size_t val) -> void;
  auto set_tag_list_cap(size_t val) -> void;
  auto set_view_distance(float val) -> void;
  auto set_fade_timer(float val) -> void;
  auto set_fade_speed(float val) -> void;
public:
  ci_gid_object() {}
  ~ci_gid_object() override {}
};
