#include "cnif_sro_ingame_start.hpp"

#include "cg_interface.hpp"
#include "cgwnd.hpp"
#include "cninterface_manager.hpp"
#include "utils/rtti.hpp"

namespace {

  using ext_client::offsets::as_fn;

  cnif_sro_ingame_start* g_cached_start_panel = nullptr;
  cgwnd* g_cached_survey_button = nullptr;

  auto diagnose_ingame_res(int res_key) -> ingame_res_lookup {
    ingame_res_lookup out{};
    out.res_key = res_key;

    auto* mgr = cninterface_manager::instance();
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

  auto panel_vftable(const void* wnd) -> std::uint32_t {
    if (!wnd) {
      return 0;
    }
    return *reinterpret_cast<const std::uint32_t*>(wnd);
  }

  auto is_survey_button(const cgwnd* wnd) -> bool {
    if (!wnd || !wnd->is_live()) {
      return false;
    }

    return wnd->unique_id() == ext_client::offsets::cnif_sro_ingame_start::child_ids::survey_button &&
           ext_client::gfx_runtime::is_class_name_match(wnd, "CIFButton");
  }

  auto panel_child(cgwnd* panel, int unique_id) -> cgwnd* {
    if (!panel) {
      return nullptr;
    }

    using get_child_fn = cgwnd*(__thiscall*)(cgwnd*, int);
    const auto fn = as_fn<get_child_fn>(ext_client::offsets::cnif_sro_ingame_start::functions::get_child_by_unique_id);
    return fn(panel, unique_id);
  }

  auto set_start_visible(cnif_sro_ingame_start* panel, bool visible) -> void {
    if (!panel || !panel->is_live()) {
      return;
    }

    using set_visible_fn = char(__thiscall*)(void*, unsigned char);
    const auto fn = as_fn<set_visible_fn>(ext_client::offsets::cnif_sro_ingame_start::functions::set_visible_impl);
    fn(panel, visible ? 1 : 0);
  }

  struct survey_find_ctx {
    cnif_sro_ingame_start* start_panel = nullptr;
    cgwnd* survey_button = nullptr;
  };

  auto visit_find_survey_widgets(cgwnd* wnd, void* raw) -> void {
    auto* ctx = static_cast<survey_find_ctx*>(raw);
    if (!ctx || !wnd->is_live()) {
      return;
    }

    if (ctx->start_panel == nullptr && cnif_sro_ingame_start::is_instance(wnd)) {
      ctx->start_panel = cnif_sro_ingame_start::from(wnd);
    }
    if (ctx->survey_button == nullptr && is_survey_button(wnd)) {
      ctx->survey_button = wnd;
    }
  }

  auto find_by_walk(cg_interface* iface, survey_find_ctx& ctx) -> void {
    cninterface_manager::instance()->walk_roots(visit_find_survey_widgets, &ctx, 12);
    if (ctx.start_panel != nullptr && ctx.survey_button != nullptr) {
      return;
    }
    if (!iface) {
      return;
    }
    iface->as_gwnd()->walk_each(32, visit_find_survey_widgets, &ctx);
  }

  auto clear_survey_cache() -> void {
    g_cached_start_panel = nullptr;
    g_cached_survey_button = nullptr;
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
    auto* raw = cninterface_manager::instance()->get_interface_obj_raw(ext_client::offsets::cninterface_manager::res_ids::sro_ingame_start);
    if (cnif_sro_ingame_start::is_instance(raw)) {
      return cnif_sro_ingame_start::from(raw);
    }
    return nullptr;
  }

  auto resolve_info_from_map() -> cnif_sro_ingame_info* {
    auto* raw = cninterface_manager::instance()->get_interface_obj_raw(ext_client::offsets::cninterface_manager::res_ids::sro_ingame_info);
    if (cnif_sro_ingame_info::is_instance(raw)) {
      return cnif_sro_ingame_info::from(raw);
    }
    return nullptr;
  }

} // namespace

auto cnif_sro_ingame_start::is_instance(const void* wnd) -> bool {
  if (!wnd || !reinterpret_cast<const cgwnd*>(wnd)->is_live()) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CNIFSroInGameStart");
}

auto cnif_sro_ingame_info::is_instance(const void* wnd) -> bool {
  if (!wnd || !reinterpret_cast<const cgwnd*>(wnd)->is_live()) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CNIFSroInGameInfo");
}

auto cnif_sro_ingame_start::survey_button() -> cgwnd* {
  return panel_child(this, ext_client::offsets::cnif_sro_ingame_start::child_ids::survey_button);
}

auto cnif_sro_ingame_start::survey_button() const -> const cgwnd* {
  return const_cast<cnif_sro_ingame_start*>(this)->survey_button();
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
  if (!wnd || !wnd->is_live() || wnd->unique_id() != unique_id) {
    return false;
  }

  if (unique_id != ext_client::offsets::cnif_sro_ingame_start::child_ids::survey_button || !is_survey_button(wnd)) {
    return false;
  }

  for (const cgwnd* parent = wnd->parent(); parent != nullptr; parent = parent->parent()) {
    if (cnif_sro_ingame_start::is_instance(parent)) {
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
    live.start_panel = resolve_start(iface);
  }
  if (live.info_panel == nullptr) {
    live.info_panel = resolve_info(iface);
  }
  if (live.survey_button == nullptr && live.start_panel != nullptr) {
    live.survey_button = live.start_panel->survey_button();
  }

  if (live.start_panel == nullptr || live.survey_button == nullptr) {
    survey_find_ctx ctx{};
    ctx.start_panel = live.start_panel;
    ctx.survey_button = live.survey_button;
    find_by_walk(iface, ctx);
    if (live.start_panel == nullptr) {
      live.start_panel = ctx.start_panel;
    }
    if (live.survey_button == nullptr) {
      live.survey_button = ctx.survey_button;
    }
  }

  if (live.start_panel == nullptr && live.survey_button == nullptr) {
    clear_survey_cache();
  } else {
    cache_live_widgets(live);
  }

  return live;
}

auto cnif_sro_ingame_start::diagnose(cg_interface* iface) -> survey_resolve_diag {
  survey_resolve_diag diag{};
  diag.start_map = diagnose_ingame_res(ext_client::offsets::cninterface_manager::res_ids::sro_ingame_start);
  diag.info_map = diagnose_ingame_res(ext_client::offsets::cninterface_manager::res_ids::sro_ingame_info);
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
  m_show_survey = 1;
  set_start_visible(this, true);
  if (auto* button = survey_button()) {
    cgwnd::set_visible(button, true);
  }
}

auto cnif_sro_ingame_start::hide_start_panel() -> void {
  m_show_survey = 0;
  if (auto* button = survey_button()) {
    cgwnd::set_visible(button, false);
  }
  set_start_visible(this, false);
}

auto cnif_sro_ingame_start::hide_survey_panel() -> void {
  hide_start_panel();
}

auto cnif_sro_ingame_info::hide_info_panel() -> void {
  using set_visible_fn = char(__thiscall*)(void*, unsigned char);
  const auto fn = as_fn<set_visible_fn>(ext_client::offsets::cnif_sro_ingame_info::functions::set_visible);
  fn(this, 0);
}
