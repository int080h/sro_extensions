#include "cgwnd.hpp"

#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

auto cgwnd::set_visible(bool visible) -> int {
  return set_visible(this, visible);
}

auto cgwnd::set_position(int x, int y) -> int {
  return set_position(this, x, y);
}

auto cgwnd::set_size(int width, int height) -> int {
  return set_size(this, width, height);
}

auto cgwnd::set_anim(int alpha, float speed, float delay, int mode) -> void {
  using set_anim_fn = void(__cdecl*)(cgwnd*, int, float, float, int);
  const auto fn = as_fn<set_anim_fn>(ext_client::offsets::cgwnd::functions::set_anim);
  fn(this, alpha, speed, delay, mode);
}

auto cgwnd::get_bounds() const -> cgwnd_bounds {
  return {rect_x(), rect_y(), rect_w(), rect_h()};
}

auto cgwnd::client_config() -> void* {
  using get_client_config_fn = void*(__cdecl*)();
  const auto fn = as_fn<get_client_config_fn>(ext_client::offsets::cgwnd::functions::get_client_config);
  return fn();
}

auto cgwnd::client_data_version() -> unsigned {
  const auto* cfg = static_cast<const std::uint32_t*>(client_config());
  if (!cfg) {
    return 1000u;
  }
  return cfg[ext_client::offsets::cgwnd::client_config_fields::data_version / sizeof(std::uint32_t)] + 1000u;
}

auto cgwnd::screen_height() -> int {
  using get_screen_size_fn = void*(__cdecl*)();
  const auto fn = as_fn<get_screen_size_fn>(ext_client::offsets::cgwnd::functions::get_screen_size);
  const auto* screen = static_cast<const std::uint8_t*>(fn());
  if (!screen) {
    return 0;
  }
  return ext_client::offsets::field_at<int>(screen, ext_client::offsets::cgwnd::screen_size::height);
}

auto cgwnd::screen_width() -> int {
  using get_screen_size_fn = void*(__cdecl*)();
  const auto fn = as_fn<get_screen_size_fn>(ext_client::offsets::cgwnd::functions::get_screen_size);
  const auto* screen = static_cast<const std::uint8_t*>(fn());
  if (!screen) {
    return 0;
  }
  return ext_client::offsets::field_at<int>(screen, ext_client::offsets::cgwnd::screen_size::width);
}

auto cgwnd::set_position(cgwnd* wnd, int x, int y) -> int {
  if (!wnd) {
    return 0;
  }

  using set_position_fn = int(__thiscall*)(cgwnd*, int, int);
  const auto fn = as_fn<set_position_fn>(ext_client::offsets::cgwnd::functions::set_position);
  return fn(wnd, x, y);
}

auto cgwnd::set_size(cgwnd* wnd, int width, int height) -> int {
  if (!wnd) {
    return 0;
  }

  using set_size_fn = int(__thiscall*)(cgwnd*, int, int);
  const auto fn = as_fn<set_size_fn>(ext_client::offsets::cgwnd::functions::set_size);
  return fn(wnd, width, height);
}

auto cgwnd::set_visible(cgwnd* wnd, bool visible) -> int {
  if (!wnd) {
    return 0;
  }

  using set_visible_fn = int(__thiscall*)(cgwnd*, std::uint8_t);
  const auto vis = static_cast<std::uint8_t>(visible ? 1 : 0);

  const auto* vft = reinterpret_cast<const std::uint32_t*>(wnd->vftable);
  if (ext_client::msvc9::is_readable_ptr(vft, sizeof(std::uint32_t) * 24)) {
    const auto slot23 = vft[23];
    if (slot23 == ext_client::offsets::cif_static::functions::set_visible ||
        slot23 == ext_client::offsets::cif_button::functions::set_visible ||
        slot23 == ext_client::offsets::cnif_button::functions::set_visible) {
      return as_fn<set_visible_fn>(slot23)(wnd, vis);
    }
  }

  return as_fn<set_visible_fn>(ext_client::offsets::cgwnd::functions::set_visible)(wnd, vis);
}

auto cgwnd::interface_under_cursor() -> cgwnd* {
  if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::globals::interface_under_cursor),
                                          sizeof(void*))) {
    return nullptr;
  }

  auto* wnd = global_at<cgwnd*>(ext_client::offsets::globals::interface_under_cursor);
  if (!ext_client::msvc9::is_game_ptr(wnd)) {
    return nullptr;
  }
  if (!ext_client::msvc9::is_readable_ptr(wnd, ext_client::offsets::cgwnd::fields::rect_h + sizeof(int))) {
    return nullptr;
  }
  return wnd;
}
