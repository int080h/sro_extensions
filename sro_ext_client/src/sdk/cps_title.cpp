#include "cps_title.hpp"

#include "ccontroler.hpp"
#include "cgwnd.hpp"
#include "live_instance.hpp"
#include "utils/msvc9_stl.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

  auto has_title_ui_root(const cps_title* title) -> bool {
    if (!title) {
      return false;
    }
    const void* map = reinterpret_cast<const std::uint8_t*>(title) + ext_client::offsets::cps_silkroad::fields::res_ui_root;
    return ext_client::msvc9::is_readable_ptr(map, ext_client::msvc9::ui_res_map_size);
  }

  auto find_from_widget_chain() -> cps_title* {
    const cgwnd* start = cgwnd::interface_under_cursor();
    if (!start) {
      return nullptr;
    }

    for (const cgwnd* walk = start; walk != nullptr; walk = walk->parent()) {
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
    if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::cps_title::globals::active_instance),
                                            sizeof(void*))) {
      return nullptr;
    }
    auto* title = global_at<cps_title*>(ext_client::offsets::cps_title::globals::active_instance);
    if (!title || !cps_title::is_instance(title) || !has_title_ui_root(title)) {
      return nullptr;
    }
    return title;
  }

} // namespace

void cps_title::set_current(cps_title* instance) {
  ext_client::sdk::live_instance<cps_title>::set(instance);
}

auto cps_title::is_instance(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSTitle");
}

auto cps_title::current() -> cps_title* {
  return ext_client::sdk::live_instance<cps_title>::get();
}

auto cps_title::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSTitle");
}

auto cps_title::create() -> cps_title* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(ext_client::offsets::cps_title::functions::create_instance);
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

auto cps_title::sync_current() -> cps_title* {
  auto* live = resolve_live();
  cps_title::set_current(live);
  return live;
}

auto cps_title::channel_index() -> int {
  return global_at<int>(ext_client::offsets::cps_title::globals::channel_index);
}

auto cps_title::captcha_active() const -> bool {
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(this);
  return bytes[ext_client::offsets::cps_title::fields::captcha_active] != 0;
}

auto cps_title::autologin_dialog() const -> void* {
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(this);
  return *reinterpret_cast<void* const*>(bytes + ext_client::offsets::cps_title::fields::autologin_dialog);
}

auto cps_title::trigger_login() -> int {
  using do_login_fn = int(__thiscall*)(cps_title * self);
  const auto fn = as_fn<do_login_fn>(ext_client::offsets::cps_title::functions::do_login);
  return fn(this);
}
