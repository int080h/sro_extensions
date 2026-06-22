#include "cif_target_window.hpp"

#include "cg_interface.hpp"
#include "utils/rtti.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::offsets::field_at;

  void* g_cached_panel = nullptr;

  auto readable_gauge(const void* panel) -> cgwnd* {
    if (!panel) {
      return nullptr;
    }

    auto* gauge = field_at<cgwnd*>(panel, ext_client::offsets::cif_target_window::fields::hp_gauge);
    if (!gauge) {
      return nullptr;
    }
    return gauge;
  }

  auto readable_panel(void* wnd) -> cgwnd* {
    if (!wnd) {
      return nullptr;
    }
    if (!readable_gauge(wnd)) {
      return nullptr;
    }
    return reinterpret_cast<cgwnd*>(wnd);
  }

  // sub_7E8EB0 draws Mangyang / General rank UI on parent+0x388, not the root target shell.
  auto resolve_content_panel(void* root) -> void* {
    if (!root) {
      return nullptr;
    }

    if (void* special_mob = field_at<void*>(root, ext_client::offsets::cif_target_window::fields::special_mob_panel)) {
      if (readable_panel(special_mob)) {
        return special_mob;
      }
    }

    if (readable_panel(root)) {
      return root;
    }
    return nullptr;
  }

  auto lookup_root_from_iface(cg_interface* iface) -> void* {
    if (!iface) {
      return nullptr;
    }

    using get_target_window_fn = void*(__thiscall*)(cg_interface*);
    const auto get_target_window =
      ext_client::offsets::as_fn<get_target_window_fn>(ext_client::offsets::cg_interface::functions::get_target_window);
    if (void* root = get_target_window(iface)) {
      return root;
    }

    return field_at<void*>(iface, ext_client::offsets::cg_interface::fields::target_window);
  }

} // namespace

auto cif_target_window::note_active_panel(void* panel) -> void {
  if (readable_panel(panel)) {
    g_cached_panel = panel;
  }
}

auto cif_target_window::clear_active_panel() -> void {
  g_cached_panel = nullptr;
}

auto cif_target_window::is_live_target_panel(void* panel) -> bool {
  if (!readable_panel(panel)) {
    return false;
  }

  if (target_slot_id(panel) == 0) {
    return false;
  }

  cgwnd* gauge = hp_gauge(panel);
  if (!gauge) {
    return false;
  }

  const cgwnd_bounds bar = gauge->get_bounds();
  if (bar.w <= 0 || bar.h <= 0) {
    return false;
  }

  return true;
}

auto cif_target_window::active() -> void* {
  if (g_cached_panel && is_live_target_panel(g_cached_panel)) {
    return g_cached_panel;
  }

  if (g_cached_panel && target_slot_id(g_cached_panel) == 0) {
    g_cached_panel = nullptr;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return nullptr;
  }

  void* panel = resolve_content_panel(lookup_root_from_iface(iface));
  if (!is_live_target_panel(panel)) {
    return nullptr;
  }

  g_cached_panel = panel;
  return panel;
}

auto cif_target_window::hp_gauge(void* panel) -> cgwnd* {
  return readable_gauge(panel);
}

auto cif_target_window::name_label(void* panel) -> cif_static* {
  if (!panel) {
    return nullptr;
  }
  auto* label = field_at<cif_static*>(panel, ext_client::offsets::cif_target_window::fields::name_label);
  if (!label) {
    return nullptr;
  }
  return label;
}

auto cif_target_window::rank_label(void* panel) -> cif_static* {
  return hp_percent_label(panel);
}

auto cif_target_window::is_common_enemy(const void* wnd) -> bool {
  if (!wnd) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CIFTargetWindowCommonEnemy");
}

auto cif_target_window::is_special_mob(const void* wnd) -> bool {
  if (!wnd) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CIFTargetWindowSpecialMob");
}

auto cif_target_window::is_supported(const void* wnd) -> bool {
  return readable_panel(const_cast<void*>(wnd)) != nullptr;
}

auto cif_target_window::target_slot_id(void* wnd) -> std::uint32_t {
  if (!wnd) {
    return 0;
  }
  return field_at<std::uint32_t>(wnd, ext_client::offsets::cif_target_window::fields::target_entity_id);
}

auto cif_target_window::hp_percent_label(void* wnd) -> cif_static* {
  if (!wnd) {
    return nullptr;
  }

  auto* label = field_at<cif_static*>(wnd, ext_client::offsets::cif_target_window::fields::hp_percent_label);
  if (!label) {
    return nullptr;
  }
  return label;
}

auto cif_target_window::as_special_mob(void* wnd) -> void* {
  return is_special_mob(wnd) ? wnd : nullptr;
}

auto cif_target_window::hp_label(void* special_mob_wnd) -> cif_static* {
  return hp_percent_label(special_mob_wnd);
}
