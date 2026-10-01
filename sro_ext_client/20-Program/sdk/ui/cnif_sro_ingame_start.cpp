#include "pch.hpp"
#include "sdk/ui/cnif_sro_ingame_start.hpp"
#include "sdk/ui/cnif_sro_ingame_info.hpp"

#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/cninterface_manager.hpp"
#include "sdk/runtime/rtti.hpp"

namespace {

  using ext_client::off::as_fn;

  cnif_sro_ingame_start* g_cached_start_panel = nullptr;
  cgwnd* g_cached_survey_button = nullptr;
  bool g_walk_attempted = false;

  auto diagnose_ingame_res(int res_key) -> ingame_res_lookup {
    ingame_res_lookup out{};
    out.res_key = res_key;

    auto* mgr = cninterface_manager::get_instance();
    out.map_readable = mgr != nullptr;
    if (!out.map_readable) {
      return out;
    }

    out.raw = mgr->get_interface_obj_raw(res_key);
    out.found = out.raw != nullptr;
    if (out.raw == nullptr) {
      return out;
    }

    out.vftable = *reinterpret_cast<const std::uint32_t*>(out.raw);

    auto* wnd = reinterpret_cast<cgwnd*>(out.raw);
    out.live = wnd && wnd->is_live();
    out.wnd = out.live ? wnd : nullptr;
    return out;
  }

  auto is_survey_button(const cgwnd* wnd) -> bool {
    if (!wnd || !wnd->is_live()) {
      return false;
    }

    return wnd->get_unique_id() == 3/*survey_button*/ &&
           ext_client::gfx_runtime::is_class_name_match(wnd, "CIFButton");
  }

  auto panel_child(cgwnd* panel, int unique_id) -> cgwnd* {
    if (!panel) {
      return nullptr;
    }

    using get_child_fn = cgwnd*(__thiscall*)(cgwnd*, int);
    const auto fn = as_fn<get_child_fn>(0x00407E50);
    return fn(panel, unique_id);
  }

  auto set_start_visible(cnif_sro_ingame_start* panel, bool visible) -> void {
    if (!panel || !panel->is_live()) {
      return;
    }

    using set_visible_fn = char(__thiscall*)(void*, unsigned char);
    const auto fn = as_fn<set_visible_fn>(0x00407550);
    fn(panel, visible ? 1 : 0);
  }

  struct survey_find_ctx {
    cnif_sro_ingame_start* start_panel = nullptr;
    cgwnd* survey_button = nullptr;
  };

  auto is_start_panel(const void* wnd) -> bool {
    if (!wnd || !reinterpret_cast<const cgwnd*>(wnd)->is_live()) {
      return false;
    }
    return ext_client::gfx_runtime::is_class_name_match(wnd, "CNIFSroInGameStart");
  }

  auto is_info_panel(const void* wnd) -> bool {
    if (!wnd || !reinterpret_cast<const cgwnd*>(wnd)->is_live()) {
      return false;
    }
    return ext_client::gfx_runtime::is_class_name_match(wnd, "CNIFSroInGameInfo");
  }

  auto start_panel_at(void* raw) -> cnif_sro_ingame_start* {
    return is_start_panel(raw) ? reinterpret_cast<cnif_sro_ingame_start*>(raw) : nullptr;
  }

  auto info_panel_at(void* raw) -> cnif_sro_ingame_info* {
    return is_info_panel(raw) ? reinterpret_cast<cnif_sro_ingame_info*>(raw) : nullptr;
  }

  auto visit_find_survey_widgets(cgwnd* wnd, void* raw) -> void {
    auto* ctx = static_cast<survey_find_ctx*>(raw);
    if (!ctx || !wnd->is_live()) {
      return;
    }

    if (ctx->start_panel == nullptr && is_start_panel(wnd)) {
      ctx->start_panel = reinterpret_cast<cnif_sro_ingame_start*>(wnd);
    }
    if (ctx->survey_button == nullptr && is_survey_button(wnd)) {
      ctx->survey_button = wnd;
    }
  }

  auto find_by_walk(cg_interface* iface, survey_find_ctx& ctx) -> void {
    cninterface_manager::get_instance()->walk_roots(visit_find_survey_widgets, &ctx, 12);
    if (ctx.start_panel != nullptr && ctx.survey_button != nullptr) {
      return;
    }
    if (!iface) {
      return;
    }
    iface->walk_each(32, visit_find_survey_widgets, &ctx);
  }

  auto clear_survey_cache() -> void {
    g_cached_start_panel = nullptr;
    g_cached_survey_button = nullptr;
    g_walk_attempted = false;
  }

  auto cache_live_widgets(const cnif_sro_ingame_start_live& live) -> void {
    if (live.start_panel != nullptr && live.start_panel->is_live()) {
      g_cached_start_panel = live.start_panel;
    }
    if (live.survey_button != nullptr && live.survey_button->is_live()) {
      g_cached_survey_button = live.survey_button;
    }
  }

  auto resolve_start_from_map() -> cnif_sro_ingame_start* {
    auto* raw = cninterface_manager::get_instance()->get_interface_obj_raw(0x35);
    return start_panel_at(raw);
  }

  auto resolve_info_from_map() -> cnif_sro_ingame_info* {
    auto* raw = cninterface_manager::get_instance()->get_interface_obj_raw(0x34);
    return info_panel_at(raw);
  }

} // namespace

auto cnif_sro_ingame_start::is_show_survey() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x7A0) != 0;
}

auto cnif_sro_ingame_start::get_survey_button() -> cgwnd* {
  return panel_child(this, 3/*survey_button*/);
}

auto cnif_sro_ingame_start::get_survey_button() const -> const cgwnd* {
  return const_cast<cnif_sro_ingame_start*>(this)->get_survey_button();
}

auto cnif_sro_ingame_start::resolve_start(cg_interface* iface) -> cnif_sro_ingame_start* {
  if (auto* start = resolve_start_from_map()) {
    return start;
  }

  survey_find_ctx ctx{};
  find_by_walk(iface, ctx);
  return ctx.start_panel;
}

auto cnif_sro_ingame_start::resolve_info(cg_interface* iface) -> cnif_sro_ingame_info* {
  (void)iface;
  return resolve_info_from_map();
}

auto cnif_sro_ingame_start::resolve(cg_interface* iface) -> cnif_sro_ingame_start* {
  return resolve_start(iface);
}

auto cnif_sro_ingame_start::is_child_of_panel(const cgwnd* wnd, int unique_id) -> bool {
  if (!wnd || !wnd->is_live() || wnd->get_unique_id() != unique_id) {
    return false;
  }

  if (unique_id != 3/*survey_button*/ || !is_survey_button(wnd)) {
    return false;
  }

  for (const cgwnd* parent = wnd->get_parent(); parent != nullptr; parent = parent->get_parent()) {
    if (is_start_panel(parent)) {
      return true;
    }
  }
  return false;
}

auto cnif_sro_ingame_start::find_live(cg_interface* iface) -> cnif_sro_ingame_start_live {
  cnif_sro_ingame_start_live live{};

  if (g_cached_start_panel != nullptr && g_cached_start_panel->is_live()) {
    live.start_panel = g_cached_start_panel;
  }
  if (g_cached_survey_button != nullptr && g_cached_survey_button->is_live()) {
    live.survey_button = g_cached_survey_button;
  }

  if (live.start_panel == nullptr) {
    live.start_panel = resolve_start_from_map();
  }
  if (live.info_panel == nullptr) {
    live.info_panel = resolve_info_from_map();
  }
  if (live.survey_button == nullptr && live.start_panel != nullptr) {
    live.survey_button = live.start_panel->get_survey_button();
  }

  // Only walk the UI tree once if start_panel could not be found via fast map lookup
  if (live.start_panel == nullptr && !g_walk_attempted) {
    g_walk_attempted = true;
    survey_find_ctx ctx{};
    find_by_walk(iface, ctx);
    live.start_panel = ctx.start_panel;
    if (live.start_panel != nullptr && live.survey_button == nullptr) {
      live.survey_button = live.start_panel->get_survey_button();
    }
  }

  if (live.start_panel != nullptr || live.survey_button != nullptr) {
    cache_live_widgets(live);
  }

  return live;
}


auto cnif_sro_ingame_start::diagnose(cg_interface* iface) -> survey_resolve_diag {
  survey_resolve_diag diag{};
  diag.start_map = diagnose_ingame_res(0x35);
  diag.info_map = diagnose_ingame_res(0x34);
  diag.live = find_live(iface);
  diag.ready = diag.live.start_panel != nullptr && diag.live.survey_button != nullptr;
  return diag;
}

auto cnif_sro_ingame_start::hide_survey_button(cg_interface* iface) -> void {
  const auto live = find_live(iface);

  if (live.start_panel != nullptr) {
    live.start_panel->hide_start_panel();
    return;
  }

  if (live.survey_button != nullptr) {
    cgwnd::set_visible(live.survey_button, false);
  }
}

auto cnif_sro_ingame_start::show_survey_button(cg_interface* iface) -> void {
  const auto live = find_live(iface);

  if (live.info_panel != nullptr) {
    live.info_panel->hide_info_panel();
  }

  if (live.start_panel != nullptr) {
    live.start_panel->show_start_panel();
    return;
  }

  if (live.survey_button != nullptr) {
    cgwnd::set_visible(live.survey_button, true);
  }
}

auto cnif_sro_ingame_start::hide_panel(cg_interface* iface) -> void {
  hide_survey_button(iface);
}

auto cnif_sro_ingame_start::set_panel_visible(cg_interface* iface, bool visible) -> void {
  if (!visible) {
    hide_panel(iface);
  } else {
    show_survey_button(iface);
  }
}

auto cnif_sro_ingame_start::show_start_panel() -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x7A0) = 1;
  set_start_visible(this, true);
  if (auto* button = get_survey_button()) {
    cgwnd::set_visible(button, true);
  }
}

auto cnif_sro_ingame_start::hide_start_panel() -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x7A0) = 0;
  if (auto* button = get_survey_button()) {
    cgwnd::set_visible(button, false);
  }
  set_start_visible(this, false);
}

auto cnif_sro_ingame_start::hide_survey_panel() -> void {
  hide_start_panel();
}

auto cnif_sro_ingame_info::hide_info_panel() -> void {
  using set_visible_fn = char(__thiscall*)(void*, unsigned char);
  const auto fn = as_fn<set_visible_fn>(0x0068E6C0);
  fn(this, 0);
}
