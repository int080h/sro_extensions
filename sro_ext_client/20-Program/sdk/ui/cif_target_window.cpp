#include "pch.hpp"
#include "sdk/ui/cif_target_window.hpp"
#include "sdk/ui/cif_gauge.hpp"
#include "sdk/ui/cif_static.hpp"

#include "sdk/render/cg_interface.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/memory.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::off::field_at;
  using ext_client::utils::memory::is_game_ptr;
} // namespace

auto cif_target_window::target_slot_id() const -> std::uint32_t {
  if (!this || !is_game_ptr(this)) return 0;
  return field_at<std::uint32_t>(this, 0x374);
}

auto cif_target_window::name_label() -> cif_static* {
  if (!this || !is_game_ptr(this)) return nullptr;
  return field_at<cif_static*>(this, 0x378);
}

auto cif_target_window::hp_gauge() -> cif_gauge* {
  if (!this || !is_game_ptr(this)) return nullptr;
  return field_at<cif_gauge*>(this, 0x37C);
}

auto cif_target_window::rank_label() -> cif_static* {
  if (!this || !is_game_ptr(this)) return nullptr;
  return field_at<cif_static*>(this, 0x380);
}

auto cif_target_window::special_mob_window() -> cif_target_window* {
  if (!this || !is_game_ptr(this)) return nullptr;
  return field_at<cif_target_window*>(this, 0x388);
}



auto cif_target_window::is_live_target_panel(const void* panel) -> bool {
  if (!panel || !is_game_ptr(panel)) {
    return false;
  }
  const auto* wnd = reinterpret_cast<const cif_target_window*>(panel);
  if (!wnd->is_visible() || wnd->target_slot_id() == 0) {
    return false;
  }
  const auto* gauge = const_cast<cif_target_window*>(wnd)->hp_gauge();
  if (!gauge || !gauge->is_visible()) {
    return false;
  }
  const auto bar = gauge->get_bounds();
  return bar.w > 0 && bar.h > 0;
}

auto cif_target_window::active() -> cif_target_window* {
  auto* iface = cg_interface::get();
  if (!iface) {
    return nullptr;
  }

  auto* root = iface->target_window();
  if (!root || !is_game_ptr(root) || !root->is_visible() || root->target_slot_id() == 0) {
    return nullptr;
  }

  // If a special mob (Unique / Giant / Champion) is targeted, its gauge is on child +0x388
  if (auto* special_mob = root->special_mob_window()) {
    if (special_mob && is_game_ptr(special_mob) && special_mob->is_visible()) {
      return special_mob;
    }
  }

  return root;
}

auto cif_target_window::hp_gauge(void* panel) -> cif_gauge* {
  if (!panel) return nullptr;
  return reinterpret_cast<cif_target_window*>(panel)->hp_gauge();
}

auto cif_target_window::name_label(void* panel) -> cif_static* {
  if (!panel) return nullptr;
  return reinterpret_cast<cif_target_window*>(panel)->name_label();
}

auto cif_target_window::rank_label(void* panel) -> cif_static* {
  if (!panel) return nullptr;
  return reinterpret_cast<cif_target_window*>(panel)->rank_label();
}

auto cif_target_window::is_common_enemy(const void* wnd) -> bool {
  if (!wnd) return false;
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CIFTargetWindowCommonEnemy");
}

auto cif_target_window::is_special_mob(const void* wnd) -> bool {
  if (!wnd) return false;
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CIFTargetWindowSpecialMob");
}

auto cif_target_window::is_supported(const void* wnd) -> bool {
  return is_live_target_panel(wnd);
}

auto cif_target_window::target_slot_id(void* wnd) -> std::uint32_t {
  if (!wnd) return 0;
  return reinterpret_cast<cif_target_window*>(wnd)->target_slot_id();
}

auto cif_target_window::hp_percent_label(void* wnd) -> cif_static* {
  return rank_label(wnd);
}

auto cif_target_window::hp_label(void* special_mob_wnd) -> cif_static* {
  return rank_label(special_mob_wnd);
}
