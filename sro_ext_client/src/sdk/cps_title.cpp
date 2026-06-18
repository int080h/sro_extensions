#include "cps_title.hpp"

#include "cgwnd.hpp"
#include "live_instance.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/process_manager.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

  auto vtable_contains_fn(const void* widget, std::uint32_t fn) -> bool {
    if (!widget || fn == 0 || !ext_client::msvc9::is_readable_ptr(widget, sizeof(std::uint32_t))) {
      return false;
    }

    const auto vtable = *reinterpret_cast<const std::uint32_t*>(widget);
    if (!vtable || !ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(vtable), 0x150)) {
      return false;
    }

    for (std::size_t off = 0; off < 0x150; off += sizeof(std::uint32_t)) {
      if (*reinterpret_cast<const std::uint32_t*>(vtable + off) == fn) {
        return true;
      }
    }
    return false;
  }

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
  if (!ptr || !ext_client::msvc9::is_readable_ptr(ptr, sizeof(void*) * 4)) {
    return false;
  }

  if (is_live(ptr)) {
    return true;
  }

  using namespace ext_client::offsets::cps_title::functions;
  return vtable_contains_fn(ptr, on_create) || vtable_contains_fn(ptr, do_login);
}

auto cps_title::current() -> cps_title* {
  return ext_client::sdk::live_instance<cps_title>::get();
}

auto cps_title::is_live(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  std::uint32_t vft = 0;
  if (!ext_client::msvc9::try_read_u32(ptr, &vft)) {
    return false;
  }
  return vft == ext_client::offsets::cps_title::vtable::address;
}

auto cps_title::create() -> cps_title* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(ext_client::offsets::cps_title::functions::create_instance);
  return reinterpret_cast<cps_title*>(fn());
}

auto cps_title::resolve_live() -> cps_title* {
  if (auto* title = ext_client::utils::process_manager::active_child_as<cps_title>(ext_client::offsets::cps_title::vtable::address)) {
    return title;
  }

  if (auto* active = ext_client::utils::process_manager::active_child()) {
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
