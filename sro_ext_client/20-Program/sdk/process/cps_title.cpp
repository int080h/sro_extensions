#include "pch.hpp"
#include "sdk/process/cps_title.hpp"

#include "sdk/game/ccontroler.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "utils/msvc9_stl.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;

  auto has_title_ui_root(const cps_title* title) -> bool {
    if (!title) {
      return false;
    }
    const void* map = reinterpret_cast<const std::uint8_t*>(title) + 0x0B0;
    return map != nullptr;
  }

  auto find_from_widget_chain() -> cps_title* {
    const cgwnd* start = cgwnd::get_interface_under_cursor();
    if (!start) {
      return nullptr;
    }

    for (const cgwnd* walk = start; walk != nullptr; walk = walk->get_parent()) {
      if (!cps_title::is_instance(walk)) {
        continue;
      }
      auto* title = const_cast<cps_title*>(reinterpret_cast<const cps_title*>(walk));
      if (has_title_ui_root(title)) {
        return title;
      }
    }
    return nullptr;
  }

  auto read_active_instance() -> cps_title* {
    auto* title = global_at<cps_title*>(0x0117E91C);
    if (!title || !cps_title::is_instance(title) || !has_title_ui_root(title)) {
      return nullptr;
    }
    return title;
  }
} // namespace

auto cps_title::is_instance(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSTitle");
}

auto cps_title::current() -> cps_title* {
  return resolve_live();
}

auto cps_title::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSTitle");
}

auto cps_title::create() -> cps_title* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x00976CE0);
  return reinterpret_cast<cps_title*>(fn());
}

auto cps_title::resolve_live() -> cps_title* {
  if (auto* title = ccontroler::active_child_as<cps_title>("CPSTitle")) {
    return title;
  }

  if (auto* active = ccontroler::active_child()) {
    if (auto* title = reinterpret_cast<cps_title*>(active); cps_title::is_instance(title) && has_title_ui_root(title)) {
      return title;
    }
  }

  if (auto* title = read_active_instance()) {
    return title;
  }

  return find_from_widget_chain();
}

auto cps_title::get_channel_index() -> int {
  return global_at<int>(0x0117E918);
}

auto cps_title::is_captcha_active() const -> bool {
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(this);
  return bytes[0x1C1] != 0;
}

auto cps_title::get_autologin_dialog() const -> void* {
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(this);
  return *reinterpret_cast<void* const*>(bytes + 0x210);
}

auto cps_title::trigger_login() -> int {
  using do_login_fn = int(__thiscall*)(cps_title * self);
  const auto fn = as_fn<do_login_fn>(0x0096A3A0);
  return fn(this);
}
