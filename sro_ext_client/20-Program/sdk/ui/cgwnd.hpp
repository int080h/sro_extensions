#pragma once

#include "sdk/ui/cgwnd_base.hpp"
#include "sdk/types/cgwnd_create_rect.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

class cclient_config;
class cgwnd;

namespace cgwnd_fn {

  using get_child_by_unique_id = cgwnd*(__thiscall*)(cgwnd*, int);
  using get_client_config = cclient_config*(__cdecl*)();
  using get_manager = void*(__cdecl*)();
  using get_screen_size = void*(__cdecl*)();
  using hit_test_contains = bool(__thiscall*)(const cgwnd*, int, int);
  using is_visible = char(__thiscall*)(cgwnd*);
  using pick_at_point = int(__thiscall*)(cgwnd*, int, int);
  using refresh_interface_under_cursor = char(__thiscall*)(void*);
  using scalar_deleting_dtor = void*(__thiscall*)(cgwnd*, char);
  using set_anim = void(__cdecl*)(cgwnd*, int, float, float, int);
  using set_position = int(__thiscall*)(cgwnd*, int, int);
  using set_size = int(__thiscall*)(cgwnd*, int, int);
  using set_visible = int(__thiscall*)(cgwnd*, std::uint8_t);
} // namespace cgwnd_fn

// ---------------------------------------------------------------------------
// CGWnd — Base UI window class in the engine
// Native VTable: 0x0106850C (38 slots) | Extends CGWndBase (+0x2C..+0x83)
// ---------------------------------------------------------------------------
class cgwnd : public cgwnd_base {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x0106850C;
  static constexpr std::size_t   vtable_slots  = 38;

  using child_visitor_fn = void (*)(cgwnd* child, void* ctx);

  // 1. Raw VTable Access
  [[nodiscard]] auto get_vftable() const -> const std::uintptr_t* {
    return ext_client::off::raw_vftable(this);
  }

  // 2. Geometry & Bounds
  [[nodiscard]] auto get_rect_x() const -> int;
  [[nodiscard]] auto get_rect_y() const -> int;
  [[nodiscard]] auto get_rect_w() const -> int;
  [[nodiscard]] auto get_rect_h() const -> int;
  [[nodiscard]] auto get_bounds() const -> cgwnd_bounds;
  auto set_rect_w(int width) -> void;
  auto set_position(int x, int y) -> int;
  auto set_size(int width, int height) -> int;

  // 3. Identification & Hierarchy
  [[nodiscard]] auto get_control_id() const -> int;
  [[nodiscard]] auto get_unique_id() const -> int;
  [[nodiscard]] auto get_parent() const -> cgwnd*;
  [[nodiscard]] auto get_topmost_ancestor() -> cgwnd*;

  // 4. State & Predicates
  [[nodiscard]] auto is_visible() const -> bool;
  [[nodiscard]] auto is_hit_test_contains(int x, int y) const -> bool;
  [[nodiscard]] auto is_live() const -> bool;
  auto set_visible(bool visible) -> int;
  auto set_anim(int alpha, float speed, float delay, int mode) -> void;
  auto destroy() -> void;

  // 5. Child Iteration & Walking
  auto for_each_child(child_visitor_fn visit, void* ctx) -> void;
  auto walk_each(int max_depth, child_visitor_fn visit, void* ctx) -> void;

  // 6. Static Queries & Manipulators
  static auto is_pickable(const cgwnd* wnd) -> bool;
  static auto get_child_by_unique_id(cgwnd* parent, int unique_id) -> cgwnd*;
  static auto get_client_config() -> cclient_config*;
  static auto get_client_data_version() -> unsigned;
  static auto get_game_ui_root() -> cgwnd*;
  static auto get_interface_under_cursor() -> cgwnd*;
  static auto get_manager() -> void*;
  static auto get_pick_at_point(cgwnd* root, int x, int y) -> cgwnd*;
  static auto get_screen_height() -> int;
  static auto get_screen_width() -> int;
  static auto get_type_name(const void* obj) -> const char*;
  static auto get_type_name_vftable(std::uint32_t vftable) -> const char*;
  static auto refresh_interface_under_cursor() -> bool;
  static auto set_position(cgwnd* wnd, int x, int y) -> int;
  static auto set_size(cgwnd* wnd, int width, int height) -> int;
  static auto set_visible(cgwnd* wnd, bool visible) -> int;
};
