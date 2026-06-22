#include "plugins/title_plugin.hpp"
#include "core/core_plugin_manager.hpp"
#include <Windows.h>
#include <imgui.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <vector>
#include <string>

#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cclient_config.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cif_decorated_static.hpp"
#include "sdk/cif_static.hpp"
#include "sdk/cps_outer_interface.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "sdk/ccontroler.hpp"
#include "sdk/cps_title.hpp"
#include "utils/imgui_helpers.hpp"
#include "utils/log.hpp"
#include "utils/ui_res_catalog.hpp"
#include "utils/string.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::title {

  // =========================================================================
  // 1. Global Variables Definition
  // =========================================================================
  cif_static* g_data_version_label = nullptr;
  cif_static* g_exe_version_label = nullptr;
  double g_original_exe_version_value = 1.0;
  cps_title* g_bound_title = nullptr;

  bool g_saw_title = false;
  DWORD g_server_ui_update_time = 0;
  bool g_server_ui_update_pending = false;

  void* g_last_hid_channel_list_btn = nullptr;
  cgwnd* g_cached_channel_list_btn = nullptr;
  cps_title* g_cached_channel_list_title = nullptr;

  cgwnd* g_cached_big_logo = nullptr;
  cgwnd* g_cached_logo = nullptr;

  title_login_layout g_login_layout{};
  std::vector<logo_baseline_entry> g_logo_baselines{};
  char g_original_login_frame_ddj[128] = "";

  // =========================================================================
  // 2. title_login_layout Methods Implementation
  // =========================================================================
  auto title_login_layout::title_ui_child(cps_title* title, int res_id) const -> cgwnd* {
    return title->find_child(res_id);
  }

  auto title_login_layout::translated_rect(widget_rect rect, int x_adjust, int y_adjust) const -> widget_rect {
    rect.x += x_adjust;
    rect.y += y_adjust;
    return rect;
  }

  auto title_login_layout::fallback_rect_for_id(int res_id, widget_rect& out) const -> bool {
    out = {res_id, 0, 0, 0, 0, true};
    switch (res_id) {
      case title_res_id::id_edit:
        out.x = 90;
        out.y = 29;
        out.width = 130;
        out.height = 20;
        return true;
      case title_res_id::password_edit:
        out.x = 90;
        out.y = 58;
        out.width = 186;
        out.height = 20;
        return true;
      case title_res_id::server_value:
        out.x = 90;
        out.y = 113;
        out.width = 186;
        out.height = 20;
        return true;
      case title_res_id::server_button:
        out.x = 284;
        out.y = 112;
        out.width = 48;
        out.height = 24;
        return true;
      case title_res_id::channel_value:
        out.x = 90;
        out.y = 85;
        out.width = 186;
        out.height = 20;
        return true;
      case title_res_id::channel_button:
        out.x = 284;
        out.y = 84;
        out.width = 48;
        out.height = 24;
        return true;
      case title_res_id::id_label:
        out.x = 14;
        out.y = 30;
        out.width = 70;
        out.height = 15;
        return true;
      case title_res_id::password_label:
        out.x = 14;
        out.y = 58;
        out.width = 70;
        out.height = 15;
        return true;
      case title_res_id::server_label:
        out.x = 14;
        out.y = 114;
        out.width = 70;
        out.height = 15;
        return true;
      case title_res_id::channel_label:
        out.x = 14;
        out.y = 86;
        out.width = 70;
        out.height = 15;
        return true;
      case title_res_id::channel_message:
        out.x = 0;
        out.y = 0;
        out.width = 339;
        out.height = 24;
        return true;
      default:
        out = {res_id, 0, 0, 0, 0, false};
        return false;
    }
  }

  auto title_login_layout::apply_widget_rect(cps_title* title, int target_res_id, const widget_rect& rect, int base_x, int base_y) const
    -> bool {
    auto* wnd = title_ui_child(title, target_res_id);
    if (!wnd || !wnd->is_live()) {
      return false;
    }

    const int target_x = rect.valid ? base_x + rect.x : wnd->rect_x() + rect.x;
    const int target_y = rect.valid ? base_y + rect.y : wnd->rect_y() + rect.y;
    if (wnd->rect_x() != target_x || wnd->rect_y() != target_y) {
      cgwnd::set_position(wnd, target_x, target_y);
    }
    if (rect.valid && rect.width > 0 && rect.height > 0 && (wnd->rect_w() != rect.width || wnd->rect_h() != rect.height)) {
      cgwnd::set_size(wnd, rect.width, rect.height);
    }
    return true;
  }

  auto title_login_layout::hide_title_child(cps_title* title, int res_id) const -> void {
    auto* wnd = title_ui_child(title, res_id);
    if (!wnd || !wnd->is_live()) {
      return;
    }
    cgwnd::set_visible(wnd, false);
    wnd->m_visible = 0;
  }

  auto title_login_layout::load_named_or_id(const char* gdr_name, int fallback_res_id, widget_rect& out) -> bool {
    const auto* entry = ext_client::utils::ui_res_catalog::find_by_name(ext_client::utils::ui_res_catalog::screen::pstitle, gdr_name);
    if (!entry) {
      entry = ext_client::utils::ui_res_catalog::find(ext_client::utils::ui_res_catalog::screen::pstitle, fallback_res_id);
    }
    if (!entry || entry->id < 0 || entry->rect_w <= 0 || entry->rect_h <= 0) {
      return fallback_rect_for_id(fallback_res_id, out);
    }

    out.res_id = fallback_res_id;
    out.x = entry->rect_x;
    out.y = entry->rect_y;
    out.width = entry->rect_w;
    out.height = entry->rect_h;
    out.valid = true;
    return true;
  }

  auto title_login_layout::load_from_game(cps_title* title) -> bool {
    if (!title) {
      return false;
    }

    ext_client::utils::ui_res_catalog::sync_from_game(title->res_ui_root(), ext_client::utils::ui_res_catalog::screen::pstitle);

    bool ok = true;
    ok = load_named_or_id("GDR_STATIC1", title_res_id::id_label, id_label) && ok;
    ok = load_named_or_id("GDR_EDIT_ID", title_res_id::id_edit, id_edit) && ok;
    ok = load_named_or_id("GDR_STATIC2", title_res_id::password_label, password_label) && ok;
    ok = load_named_or_id("GDR_EDIT_PASS", title_res_id::password_edit, password_edit) && ok;
    ok = load_named_or_id("GDR_STATIC3", title_res_id::server_label, server_label) && ok;
    ok = load_named_or_id("GDR_STA_SERVER", title_res_id::server_value, server_value) && ok;
    ok = load_named_or_id("GDR_BTN_SERVER", title_res_id::server_button, server_button) && ok;
    ok = load_named_or_id("GDR_STATIC4", title_res_id::channel_label, channel_label) && ok;
    ok = load_named_or_id("GDR_STA_CHANNEL", title_res_id::channel_value, channel_value) && ok;
    ok = load_named_or_id("GDR_BTN_CHANNEL", title_res_id::channel_button, channel_button) && ok;
    load_named_or_id("GDR_STA_CHANNELMESSAGE", title_res_id::channel_message, channel_name);
    return ok;
  }

  auto title_login_layout::apply_eu_frame(cps_title* title, cgwnd* frame) const -> bool {
    if (!title || !frame || !frame->is_live()) {
      return false;
    }
    const int base_x = frame->rect_x();
    const int base_y = frame->rect_y();
    const auto& config = ext_client::core::config::data().title;
    bool moved = true;
    moved = apply_widget_rect(title, id_label.res_id, translated_rect(id_label, static_cast<int>(config.eu_login_id_label_adjust.x), static_cast<int>(config.eu_login_id_label_adjust.y)), base_x, base_y) && moved;
    moved = apply_widget_rect(title, id_edit.res_id, translated_rect(id_edit, static_cast<int>(config.eu_login_id_input_adjust.x), static_cast<int>(config.eu_login_id_input_adjust.y)), base_x, base_y) && moved;
    moved = apply_widget_rect(title, password_label.res_id, translated_rect(password_label, static_cast<int>(config.eu_login_pw_label_adjust.x), static_cast<int>(config.eu_login_pw_label_adjust.y)), base_x, base_y) && moved;
    moved = apply_widget_rect(title, password_edit.res_id, translated_rect(password_edit, static_cast<int>(config.eu_login_pw_input_adjust.x), static_cast<int>(config.eu_login_pw_input_adjust.y)), base_x, base_y) && moved;
    moved = apply_widget_rect(title, server_label.res_id, translated_rect(server_label, static_cast<int>(config.eu_login_server_label_adjust.x), static_cast<int>(config.eu_login_server_label_adjust.y)), base_x, base_y) && moved;
    auto eu_server_value = translated_rect(server_value, static_cast<int>(config.eu_login_server_value_adjust.x), static_cast<int>(config.eu_login_server_value_adjust.y));
    if (eu_server_value.width > 186) {
      eu_server_value.width = 186;
    }
    moved = apply_widget_rect(title, server_value.res_id, eu_server_value, base_x, base_y) && moved;
    moved = apply_widget_rect(title, server_button.res_id, translated_rect(server_button, static_cast<int>(config.eu_login_server_button_adjust.x), static_cast<int>(config.eu_login_server_button_adjust.y)), base_x, base_y) && moved;
    hide_title_child(title, channel_label.res_id);
    hide_title_child(title, channel_value.res_id);
    hide_title_child(title, channel_button.res_id);
    hide_title_child(title, channel_name.res_id);
    return moved;
  }

  auto title_login_layout::restore_eu_frame(cps_title* title, cgwnd* frame) const -> bool {
    if (!title || !frame || !frame->is_live()) {
      return false;
    }
    const int base_x = frame->rect_x();
    const int base_y = frame->rect_y();
    bool moved = true;
    moved = apply_widget_rect(title, id_label.res_id, id_label, base_x, base_y) && moved;
    moved = apply_widget_rect(title, id_edit.res_id, id_edit, base_x, base_y) && moved;
    moved = apply_widget_rect(title, password_label.res_id, password_label, base_x, base_y) && moved;
    moved = apply_widget_rect(title, password_edit.res_id, password_edit, base_x, base_y) && moved;
    moved = apply_widget_rect(title, server_label.res_id, server_label, base_x, base_y) && moved;
    moved = apply_widget_rect(title, server_value.res_id, server_value, base_x, base_y) && moved;
    moved = apply_widget_rect(title, server_button.res_id, server_button, base_x, base_y) && moved;
    auto show_title_child = [](cps_title* t, int res_id) {
      auto* wnd = t->find_child(res_id);
      if (wnd && wnd->is_live() && !wnd->visible()) {
        cgwnd::set_visible(wnd, true);
        wnd->m_visible = 1;
      }
    };
    show_title_child(title, channel_label.res_id);
    show_title_child(title, channel_value.res_id);
    show_title_child(title, channel_button.res_id);
    show_title_child(title, channel_name.res_id);

    return moved;
  }

  // =========================================================================
  // 3. Helper Functions Implementation
  // =========================================================================
  auto control_panel() -> ext_client::core::config::core_config::title_t& {
    return ext_client::core::config::data().title;
  }

  auto resolve_wide_fmt(const char* utf8_fmt, wchar_t* dst, std::size_t dst_count, const wchar_t* fallback) -> const wchar_t* {
    if (!utf8_fmt || utf8_fmt[0] == '\0') {
      return fallback;
    }
    std::wstring wide = ::ext_client::utils::string::to_wide(utf8_fmt);
    if (wide.empty() || wide.size() >= dst_count) {
      return fallback;
    }
    std::wmemcpy(dst, wide.c_str(), wide.size() + 1);
    return dst;
  }

  namespace {
    auto is_title_chrome_res_id(int res_id) -> bool {
      switch (res_id) {
        case ext_client::offsets::cps_title::ui_ids::screen_up:
        case ext_client::offsets::cps_title::ui_ids::screen_down:
        case ext_client::offsets::cps_title::ui_ids::border_screen_up:
        case ext_client::offsets::cps_title::ui_ids::border_screen_down:
        case ext_client::offsets::cps_title::ui_ids::title_text:
        case ext_client::offsets::cps_title::ui_ids::big_logo:
        case ext_client::offsets::cps_title::ui_ids::logo:
        case ext_client::offsets::cps_title::ui_ids::login_edit:
        case ext_client::offsets::cps_title::ui_ids::password_edit:
        case ext_client::offsets::cps_title::ui_ids::login_button:
        case ext_client::offsets::cps_title::ui_ids::exit_button:
        case ext_client::offsets::cps_title::ui_ids::server_combo:
        case ext_client::offsets::cps_title::ui_ids::channel_combo:
        case ext_client::offsets::cps_title::ui_ids::channel_info:
        case ext_client::offsets::cps_title::ui_ids::server_list_button:
        case ext_client::offsets::cps_title::ui_ids::channel_list_button:
        case ext_client::offsets::cps_title::ui_ids::slider:
        case ext_client::offsets::cps_title::ui_ids::channel_list_popup:
        case ext_client::offsets::cps_title::ui_ids::unity_server:
        case ext_client::offsets::cps_title::ui_ids::msgbox:
          return true;
        default:
          return false;
      }
    }

    auto ddj_basename_matches(const char* ddj, const char* name) -> bool {
      if (!ddj || !name) {
        return false;
      }
      const char* base = std::strrchr(ddj, '\\');
      base = base ? base + 1 : ddj;
      const char* base_f = std::strrchr(base, '/');
      base = base_f ? base_f + 1 : base;
      return _stricmp(base, name) == 0;
    }

    auto is_eu_located_ddj(const char* ddj) -> bool {
      if (!ddj) {
        return false;
      }
      const char* base = std::strrchr(ddj, '\\');
      base = base ? base + 1 : ddj;
      const char* base_f = std::strrchr(base, '/');
      base = base_f ? base_f + 1 : base;

      char lower_base[128]{};
      std::strncpy(lower_base, base, sizeof(lower_base) - 1);
      for (int i = 0; lower_base[i]; i++) {
        lower_base[i] = static_cast<char>(std::tolower(lower_base[i]));
      }
      return std::strstr(lower_base, "login_window_eu_located") != nullptr;
    }

    auto title_ui_child(cps_title* title, int res_id) -> cgwnd* {
      return title->find_child(res_id);
    }

    auto detect_label_type(const wchar_t* text, bool check_bottom_rules) -> label_type {
      if (!text) {
        return label_type::none;
      }
      if (wcsstr(text, L"Data Version") != nullptr) {
        return label_type::data;
      }
      if (wcsstr(text, L"Exe Version") != nullptr || (wcsstr(text, L"Version") != nullptr && wcsstr(text, L"Beta") != nullptr)) {
        return label_type::exe;
      }
      if (check_bottom_rules && wcsstr(text, L"Version") != nullptr) {
        if (wcsstr(text, L"Data Version") != nullptr) {
          return label_type::data;
        }
        return label_type::exe;
      }
      return label_type::none;
    }

    auto is_plausible_version_label(cps_title* title, const cif_static* label) -> bool {
      if (!label) {
        return false;
      }
      const auto* wnd = reinterpret_cast<const cgwnd*>(label);
      if (wnd->rect_w() > k_version_label_max_w || wnd->rect_h() > k_version_label_max_h) {
        return false;
      }
      const int screen_h = cgwnd::screen_height();
      if (screen_h > 0 && wnd->rect_y() < screen_h - 120) {
        return false;
      }
      if (title) {
        const int res_key = title->res_map_key_for(wnd);
        if (res_key >= 0 && is_title_chrome_res_id(res_key)) {
          return false;
        }
      }
      if (is_title_chrome_res_id(wnd->unique_id())) {
        return false;
      }
      wchar_t text[256]{};
      if (!cif_static::read_text(label, text, 256) || text[0] == L'\0') {
        return false;
      }
      return wcsstr(text, L"Data Version") != nullptr || wcsstr(text, L"Exe Version") != nullptr ||
             (wcsstr(text, L"Version") != nullptr && wcsstr(text, L"Beta") != nullptr);
    }

    auto sanitize_version_label_ptr(cif_static*& label) -> void {
      if (label != nullptr && !is_plausible_version_label(g_bound_title, label)) {
        label = nullptr;
      }
    }

    auto parse_exe_version_value(const wchar_t* text) -> double {
      if (!text || text[0] == L'\0') {
        return 1.0;
      }
      const wchar_t* end = text + wcslen(text);
      const wchar_t* cursor = end;
      while (cursor > text && cursor[-1] != L' ' && cursor[-1] != L'\t') {
        --cursor;
      }
      double value = 1.0;
      if (swscanf_s(cursor, L"%lf", &value) == 1) {
        return value;
      }
      return 1.0;
    }

    auto visit_discover_version_label(cgwnd* widget, void* raw) -> void {
      auto* visit = static_cast<label_discover_ctx*>(raw);
      if (!visit || (!visit->need_data && !visit->need_exe)) {
        return;
      }
      auto* label = cif_static::as_if_static(widget);
      if (!label || !is_plausible_version_label(visit->title, label)) {
        return;
      }
      wchar_t text[256]{};
      if (!cif_static::read_text(label, text, 256)) {
        return;
      }
      auto type = detect_label_type(text, false);
      if (type == label_type::none) {
        type = detect_label_type(text, true);
      }
      if (type == label_type::none) {
        return;
      }
      if (type == label_type::data && visit->need_data && !g_data_version_label) {
        g_data_version_label = label;
        visit->need_data = false;
        if (control_panel().log_events) {
          log_msg("[title_plugin] discovered data label %p", label);
        }
      } else if (type == label_type::exe && visit->need_exe && !g_exe_version_label) {
        g_exe_version_label = label;
        g_original_exe_version_value = parse_exe_version_value(text);
        visit->need_exe = false;
        if (control_panel().log_events) {
          log_msg("[title_plugin] discovered exe label %p (value=%.3f)", label, g_original_exe_version_value);
        }
      }
    }

    auto apply_version_label_style(cif_static* label) -> void {
      if (!label) {
        return;
      }
      if (control_panel().enabled && control_panel().override_version_label_color) {
        label->set_text_color(control_panel().version_label_color);
      } else {
        label->set_text_color(0xFFFFFFFF);
      }
      const auto mode = control_panel().enabled && control_panel().version_labels_clip ? cif_text_clip_mode::ellipsis_hover : cif_text_clip_mode::full;
      label->set_text_clip_mode(mode, control_panel().version_label_ellipsis_width);
    }

    auto apply_label_text(cif_static* label, const wchar_t* fmt, bool is_exe_fmt, int major, int minor, double exe_value) -> void {
      if (!label || !fmt) {
        return;
      }
      if (wcschr(fmt, L'%') != nullptr) {
        if (is_exe_fmt) {
          label->set_text_fmt(fmt, exe_value);
        } else {
          label->set_text_fmt(fmt, major, minor);
        }
      } else {
        label->set_text(fmt);
      }
      apply_version_label_style(label);
    }

    auto hide_title_widget(cgwnd* widget) -> void {
      if (!widget || !widget->is_live()) {
        return;
      }
      cgwnd::set_visible(widget, false);
      widget->m_visible = 0;
    }

    auto hide_title_child_by_id(cps_title* self, int res_id) -> void {
      auto* widget = self->find_child(res_id);
      if (widget && widget->is_live()) {
        hide_title_widget(widget);
      }
    }

    auto set_static_texture(cif_static* widget, const char* path) -> bool {
      if (!widget || !path || path[0] == '\0') {
        return false;
      }
      auto* image_sub =
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(widget) + ext_client::offsets::cif_static::fields::image_subobject);
      if (!image_sub) {
        return false;
      }
      const auto* image_vftable = *reinterpret_cast<void***>(image_sub);
      if (!image_vftable) {
        return false;
      }
      using set_texture_fn = void(__thiscall*)(void*, ext_client::msvc9::string_pod, int, int);
      const auto set_tex_fn =
        reinterpret_cast<set_texture_fn>(image_vftable[ext_client::offsets::cif_static::cif_image_renderer::vtable::set_texture]);
      if (!set_tex_fn) {
        return false;
      }
      ext_client::msvc9::string s(path);
      ext_client::msvc9::string_pod pod{};
      std::memcpy(&pod, s.raw(), sizeof(pod));
      ext_client::msvc9::string empty_s;
      std::memcpy(s.raw(), empty_s.raw(), sizeof(pod));
      set_tex_fn(image_sub, pod, 0, 0);
      return true;
    }

    auto is_login_frame_ddj(const char* ddj) -> bool {
      return ddj_basename_matches(ddj, k_login_frame_id_ddj) || ddj_basename_matches(ddj, k_login_frame_email_ddj) ||
             ddj_basename_matches(ddj, k_login_frame_default_ddj) || is_eu_located_ddj(ddj);
    }

    auto visit_find_login_frame(cgwnd* wnd, void* raw) -> void {
      auto* ctx = static_cast<login_frame_walk_ctx*>(raw);
      if (!ctx || ctx->found || !wnd->is_live() || !cif_static::is_static(wnd)) {
        return;
      }
      char current_ddj[128]{};
      if (cif_static::read_ddj_path(wnd, current_ddj, sizeof(current_ddj)) && is_login_frame_ddj(current_ddj)) {
        ctx->found = wnd;
      }
    }

    auto resolve_login_frame(cps_title* title) -> cgwnd* {
      if (auto* wnd = title_ui_child(title, ext_client::offsets::cps_title::ui_ids::login_edit)) {
        if (wnd && wnd->is_live() && cif_static::is_static(wnd)) {
          char current_ddj[128]{};
          if (cif_static::read_ddj_path(wnd, current_ddj, sizeof(current_ddj)) && is_login_frame_ddj(current_ddj)) {
            return wnd;
          }
        }
      }
      login_frame_walk_ctx ctx{};
      reinterpret_cast<cgwnd*>(title)->walk_each(12, visit_find_login_frame, &ctx);
      return ctx.found;
    }

    auto apply_login_frame_override(cps_title* title) -> void {
      auto* wnd = resolve_login_frame(title);
      if (!wnd || !wnd->is_live() || !cif_static::is_static(wnd)) {
        return;
      }
      char current_ddj[128]{};
      if (!cif_static::read_ddj_path(wnd, current_ddj, sizeof(current_ddj))) {
        return;
      }

      if (control_panel().enabled && control_panel().replace_login_frame && control_panel().login_frame_path[0]) {
        if (g_original_login_frame_ddj[0] == '\0' && !ddj_basename_matches(current_ddj, control_panel().login_frame_path)) {
          std::strncpy(g_original_login_frame_ddj, current_ddj, sizeof(g_original_login_frame_ddj) - 1);
        }

        bool skip_replace = false;
        if (is_eu_located_ddj(control_panel().login_frame_path) && is_eu_located_ddj(current_ddj)) {
          skip_replace = true;
        } else if (ddj_basename_matches(current_ddj, control_panel().login_frame_path)) {
          skip_replace = true;
        }

        if (!skip_replace &&
            set_static_texture(static_cast<cif_static*>(wnd), control_panel().login_frame_path) && control_panel().log_events) {
          log_msg("[title_plugin] login frame %s -> %s", current_ddj, control_panel().login_frame_path);
        }
        if (g_login_layout.load_from_game(title)) {
          g_login_layout.apply_eu_frame(title, wnd);
        }
      } else {
        if (g_original_login_frame_ddj[0] != '\0' && !ddj_basename_matches(current_ddj, g_original_login_frame_ddj)) {
          if (set_static_texture(static_cast<cif_static*>(wnd), g_original_login_frame_ddj)) {
            if (control_panel().log_events) {
              log_msg("[title_plugin] restored login frame -> %s", g_original_login_frame_ddj);
            }
            g_original_login_frame_ddj[0] = '\0';
          }
        }
        if (g_login_layout.load_from_game(title)) {
          g_login_layout.restore_eu_frame(title, wnd);
        }
      }
    }

    auto clear_version_label_capture() -> void {
      g_data_version_label = nullptr;
      g_exe_version_label = nullptr;
      g_original_exe_version_value = 1.0;
    }

    auto clear_channel_list_cache() -> void {
      g_cached_channel_list_btn = nullptr;
      g_cached_channel_list_title = nullptr;
      g_last_hid_channel_list_btn = nullptr;
    }

    auto clear_logo_baselines() -> void {
      g_logo_baselines.clear();
    }

    auto reset_title_binding() -> void {
      g_bound_title = nullptr;
      g_saw_title = false;
      clear_version_label_capture();
      clear_channel_list_cache();
      clear_logo_baselines();
      g_cached_big_logo = nullptr;
      g_cached_logo = nullptr;
    }

    auto hide_channel_login_widgets(cps_title* self) -> void {
      if (!self || !is_title_screen_active(self)) {
        return;
      }
      if (control_panel().enabled && control_panel().replace_login_frame) {
        hide_title_child_by_id(self, 104);
        hide_title_child_by_id(self, 45);
        hide_title_child_by_id(self, ext_client::offsets::cps_title::ui_ids::channel_list_button);
        hide_title_child_by_id(self, ext_client::offsets::cps_title::ui_ids::channel_info);
      } else {
        auto show_title_child = [](cps_title* t, int res_id) {
          auto* child = t->find_child(res_id);
          if (child && child->is_live() && !child->visible()) {
            cgwnd::set_visible(child, true);
            child->m_visible = 1;
          }
        };
        show_title_child(self, 104);
        show_title_child(self, 45);
        show_title_child(self, ext_client::offsets::cps_title::ui_ids::channel_list_button);
        show_title_child(self, ext_client::offsets::cps_title::ui_ids::channel_info);
      }
    }

    auto is_channel_row_list_candidate(const cgwnd* wnd, const cgwnd* channel_combo) -> bool {
      if (!wnd || !wnd->is_live() || !channel_combo || !channel_combo->is_live()) {
        return false;
      }
      const int row_y = channel_combo->rect_y();
      const int row_right = channel_combo->rect_x() + channel_combo->rect_w();
      if (std::abs(wnd->rect_y() - row_y) > 16) {
        return false;
      }
      if (wnd->rect_x() < row_right - 40) {
        return false;
      }
      if (wnd->rect_w() <= 0 || wnd->rect_h() <= 0 || wnd->rect_w() > 120 || wnd->rect_h() > 60) {
        return false;
      }
      char ddj[128]{};
      if (!cif_static::read_ddj_path(wnd, ddj, sizeof(ddj))) {
        return true;
      }
      return std::strstr(ddj, "list_button") != nullptr;
    }

    auto visit_find_channel_list_button(cgwnd* wnd, void* raw) -> void {
      auto* ctx = static_cast<channel_list_find_ctx*>(raw);
      if (!ctx || ctx->found) {
        return;
      }
      if (wnd->unique_id() == ext_client::offsets::cps_title::ui_ids::channel_list_button && wnd->is_live()) {
        ctx->found = wnd;
        return;
      }
      if (is_channel_row_list_candidate(wnd, ctx->channel_combo)) {
        ctx->found = wnd;
      }
    }

    auto is_title_list_button_widget(const cgwnd* wnd) -> bool {
      if (!wnd || !wnd->is_live()) {
        return false;
      }
      const bool is_button =
        ext_client::gfx_runtime::is_class_name_match(wnd, "CIFButton") || ext_client::gfx_runtime::is_class_name_match(wnd, "CNIFButton");
      const bool is_decorated = ext_client::gfx_runtime::is_class_name_match(wnd, "CIFDecoratedStatic");
      if (!is_button && !is_decorated) {
        return false;
      }
      const int w = wnd->rect_w();
      const int h = wnd->rect_h();
      return w > 0 && h > 0 && w <= 200 && h <= 80;
    }

    auto is_channel_row_list_button(const cgwnd* wnd, const cgwnd* channel_combo) -> bool {
      if (!is_title_list_button_widget(wnd)) {
        return false;
      }
      if (!channel_combo || !channel_combo->is_live()) {
        return true;
      }
      const int row_y = channel_combo->rect_y();
      const int row_right = channel_combo->rect_x() + channel_combo->rect_w();
      return std::abs(wnd->rect_y() - row_y) <= 12 && wnd->rect_x() >= row_right - 24;
    }

    auto pick_channel_row_list_button(cgwnd* const* buttons, int count) -> cgwnd* {
      cgwnd* channel_btn = nullptr;
      int channel_y = INT_MAX;
      for (int i = 0; i < count; ++i) {
        if (!buttons[i] || !buttons[i]->is_live()) {
          continue;
        }
        for (int j = i + 1; j < count; ++j) {
          if (!buttons[j] || !buttons[j]->is_live()) {
            continue;
          }
          const int xi = buttons[i]->rect_x();
          const int xj = buttons[j]->rect_x();
          const int yi = buttons[i]->rect_y();
          const int yj = buttons[j]->rect_y();
          if (std::abs(xi - xj) > 8) {
            continue;
          }
          auto* upper = yi <= yj ? buttons[i] : buttons[j];
          const int upper_y = yi <= yj ? yi : yj;
          if (upper_y < channel_y) {
            channel_y = upper_y;
            channel_btn = upper;
          }
        }
      }
      if (channel_btn) {
        return channel_btn;
      }
      for (int i = 0; i < count; ++i) {
        if (!buttons[i] || !buttons[i]->is_live()) {
          continue;
        }
        const int y = buttons[i]->rect_y();
        if (y < channel_y) {
          channel_y = y;
          channel_btn = buttons[i];
        }
      }
      return (channel_btn && channel_btn->is_live()) ? channel_btn : nullptr;
    }

    auto collect_title_list_button(cgwnd* wnd, void* ctx) -> void {
      auto* collect = static_cast<title_list_button_collect_ctx*>(ctx);
      if (!collect || collect->count >= 8) {
        return;
      }
      if (!is_title_list_button_widget(wnd)) {
        return;
      }
      collect->buttons[collect->count++] = wnd;
    }

    auto scan_title_channel_list_button(cps_title* title) -> cgwnd* {
      if (!title) {
        return nullptr;
      }
      auto* root = reinterpret_cast<cgwnd*>(title);
      if (!root || !root->is_live()) {
        return nullptr;
      }
      title_list_button_collect_ctx collect{};
      root->walk_each(12, collect_title_list_button, &collect);
      if (collect.count == 0) {
        return nullptr;
      }
      auto* channel_combo = title->find_child(ext_client::offsets::cps_title::ui_ids::channel_combo);
      if (channel_combo && channel_combo->is_live()) {
        cgwnd* best = nullptr;
        int best_dx = INT_MAX;
        const int row_right = channel_combo->rect_x() + channel_combo->rect_w();
        for (int i = 0; i < collect.count; ++i) {
          auto* wnd = collect.buttons[i];
          if (!is_channel_row_list_button(wnd, channel_combo)) {
            continue;
          }
          const int dx = wnd->rect_x() - row_right;
          if (dx < best_dx) {
            best_dx = dx;
            best = wnd;
          }
        }
        if (best) {
          return best;
        }
      }
      return pick_channel_row_list_button(collect.buttons, collect.count);
    }

    auto find_channel_list_button(cps_title* self) -> cgwnd* {
      if (auto* widget = self->find_child(ext_client::offsets::cps_title::ui_ids::channel_list_button)) {
        if (widget && widget->is_live()) {
          return widget;
        }
      }
      channel_list_find_ctx ctx{};
      ctx.channel_combo = self->find_child(ext_client::offsets::cps_title::ui_ids::channel_combo);
      reinterpret_cast<cgwnd*>(self)->walk_each(14, visit_find_channel_list_button, &ctx);
      if (ctx.found) {
        return ctx.found;
      }
      if (auto* widget = scan_title_channel_list_button(self)) {
        if (!ctx.channel_combo || is_channel_row_list_candidate(widget, ctx.channel_combo)) {
          return widget;
        }
      }
      return nullptr;
    }

    auto hide_channel_list_button(cps_title* self, bool log_if_missing) -> void {
      if (!is_title_screen_active(self)) {
        clear_channel_list_cache();
        return;
      }
      cgwnd* widget = nullptr;
      if (g_cached_channel_list_title == self && g_cached_channel_list_btn && g_cached_channel_list_btn->is_live()) {
        widget = g_cached_channel_list_btn;
      } else {
        clear_channel_list_cache();
        widget = find_channel_list_button(self);
        if (!widget || !widget->is_live()) {
          if (log_if_missing && control_panel().log_events) {
            log_msg("[title_plugin] channel list button not found");
          }
          return;
        }
        g_cached_channel_list_btn = widget;
        g_cached_channel_list_title = self;
      }

      if (control_panel().enabled && control_panel().hide_channel_list_button) {
        if (control_panel().log_events && widget != g_last_hid_channel_list_btn) {
          g_last_hid_channel_list_btn = widget;
          log_msg("[title_plugin] hid channel list button %p", widget);
        }
        hide_title_widget(widget);
      } else {
        if (widget->is_live() && !widget->visible()) {
          cgwnd::set_visible(widget, true);
          widget->m_visible = 1;
          if (control_panel().log_events) {
            log_msg("[title_plugin] restored channel list button visibility");
          }
        }
      }
    }

    auto is_login_logo_widget(const cgwnd* wnd, const char* ddj_name) -> bool {
      if (!wnd || !wnd->is_live()) {
        return false;
      }
      char ddj[128]{};
      if (!cif_static::read_ddj_path(wnd, ddj, sizeof(ddj))) {
        return false;
      }
      return ddj_basename_matches(ddj, ddj_name);
    }

    auto visit_find_logo_ddj(cgwnd* wnd, void* raw) -> void {
      auto* ctx = static_cast<logo_walk_ctx*>(raw);
      if (!ctx || ctx->found) {
        return;
      }
      if (is_login_logo_widget(wnd, ctx->ddj_name)) {
        ctx->found = wnd;
      }
    }

    auto find_logo_by_ddj(cps_title* title, const char* ddj_name) -> cgwnd* {
      logo_walk_ctx ctx{ddj_name, nullptr};
      reinterpret_cast<cgwnd*>(title)->walk_each(12, visit_find_logo_ddj, &ctx);
      return ctx.found;
    }

    auto resolve_title_logo(cps_title* title, int res_id, const char* ddj_name) -> cgwnd* {
      const bool is_big = (std::strcmp(ddj_name, "logo-big.ddj") == 0);
      if (is_big) {
        if (g_cached_big_logo && g_cached_big_logo->is_live() && is_login_logo_widget(g_cached_big_logo, ddj_name)) {
          return g_cached_big_logo;
        }
      } else {
        if (g_cached_logo && g_cached_logo->is_live() && is_login_logo_widget(g_cached_logo, ddj_name)) {
          return g_cached_logo;
        }
      }
      cgwnd* result = nullptr;
      if (auto* wnd = title->find_child(res_id)) {
        if (is_login_logo_widget(wnd, ddj_name)) {
          result = wnd;
        }
      }
      if (!result) {
        result = find_logo_by_ddj(title, ddj_name);
      }
      if (result) {
        if (is_big) {
          g_cached_big_logo = result;
        } else {
          g_cached_logo = result;
        }
      }
      return result;
    }

    auto logo_default_y(const char* ddj_name) -> int {
      return std::strcmp(ddj_name, "logo-big.ddj") == 0 ? k_big_logo_default_y : k_logo_default_y;
    }

    auto find_logo_baseline(cgwnd* wnd) -> int* {
      for (auto& entry : g_logo_baselines) {
        if (entry.widget == wnd) {
          return &entry.y;
        }
      }
      return nullptr;
    }

    auto resolve_logo_catalog_y(cps_title* title, int res_id, const char* ddj_name) -> int {
      if (title) {
        ext_client::utils::ui_res_catalog::sync_from_game(title->res_ui_root(), ext_client::utils::ui_res_catalog::screen::pstitle);
        if (const auto* entry = ext_client::utils::ui_res_catalog::find(ext_client::utils::ui_res_catalog::screen::pstitle, res_id)) {
          if (entry->rect_h > 0) {
            return entry->rect_y;
          }
        }
        if (const auto* entry =
              ext_client::utils::ui_res_catalog::find_by_ddj(ext_client::utils::ui_res_catalog::screen::pstitle, ddj_name)) {
          if (entry->rect_h > 0) {
            return entry->rect_y;
          }
        }
      }
      return logo_default_y(ddj_name);
    }

    auto capture_logo_baseline_y(cps_title* title, cgwnd* wnd, int res_id, const char* ddj_name) -> int {
      if (int* stored = find_logo_baseline(wnd)) {
        return *stored;
      }
      const int baseline_y = resolve_logo_catalog_y(title, res_id, ddj_name);
      g_logo_baselines.push_back({wnd, baseline_y});
      return baseline_y;
    }

    auto apply_logo_y_offset(cps_title* title, cgwnd* wnd, int res_id, int offset, const char* ddj_name) -> void {
      if (!wnd || !wnd->is_live()) {
        return;
      }
      const int baseline_y = capture_logo_baseline_y(title, wnd, res_id, ddj_name);
      if (control_panel().enabled && offset != 0) {
        const int target_y = baseline_y - offset;
        if (wnd->rect_y() != target_y) {
          cgwnd::set_position(wnd, wnd->rect_x(), target_y);
        }
      } else {
        if (wnd->rect_y() != baseline_y) {
          cgwnd::set_position(wnd, wnd->rect_x(), baseline_y);
        }
      }
    }
  } // namespace

  // =========================================================================
  // 4. Main Reorganized Functions Implementation
  // =========================================================================
  auto discover_version_labels(cps_title* title) -> void {
    if (!title) {
      return;
    }
    sanitize_version_label_ptr(g_data_version_label);
    sanitize_version_label_ptr(g_exe_version_label);
    if (g_data_version_label && g_exe_version_label) {
      return;
    }
    label_discover_ctx ctx{};
    ctx.title = title;
    ctx.need_data = g_data_version_label == nullptr;
    ctx.need_exe = g_exe_version_label == nullptr;
    reinterpret_cast<cgwnd*>(title)->walk_each(16, visit_discover_version_label, &ctx);
  }

  auto hide_version_labels() -> void {
    hide_title_widget(g_data_version_label);
    hide_title_widget(g_exe_version_label);
  }

  auto reset_logo_baselines() -> void {
    clear_logo_baselines();
  }

  auto force_eu_layout(cps_title* title) -> void {
    auto* wnd = resolve_login_frame(title);
    if (wnd && wnd->is_live() && g_login_layout.load_from_game(title)) {
      g_login_layout.apply_eu_frame(title, wnd);
    }
  }

  auto is_title_screen_active(const cps_title* self) -> bool {
    if (!self) {
      return false;
    }
    const char* name = ccontroler::active_child_process_name();
    return name && std::strcmp(name, "CPSTitle") == 0;
  }

  auto apply_version_label_overrides() -> void {
    if (!control_panel().override_version_labels) {
      return;
    }
    sanitize_version_label_ptr(g_data_version_label);
    sanitize_version_label_ptr(g_exe_version_label);
    if (!g_data_version_label && !g_exe_version_label) {
      return;
    }
    wchar_t data_fmt_buf[128]{};
    wchar_t exe_fmt_buf[128]{};
    const wchar_t* data_fmt = resolve_wide_fmt(control_panel().data_version_fmt, data_fmt_buf, 128, k_default_data_fmt);
    const wchar_t* exe_fmt = resolve_wide_fmt(control_panel().exe_version_fmt, exe_fmt_buf, 128, k_default_exe_fmt);
    const unsigned data_version = cgwnd::client_data_version();
    const int major = static_cast<int>(data_version / 1000u);
    const int minor = static_cast<int>(data_version % 1000u);
    const double exe_value = g_original_exe_version_value;
    apply_label_text(g_data_version_label, data_fmt, false, major, minor, exe_value);
    apply_label_text(g_exe_version_label, exe_fmt, true, major, minor, exe_value);
  }

  auto restore_original_version_labels() -> void {
    if (!g_data_version_label && !g_exe_version_label) {
      return;
    }
    const unsigned data_version = cgwnd::client_data_version();
    const int major = static_cast<int>(data_version / 1000u);
    const int minor = static_cast<int>(data_version % 1000u);
    const double exe_value = g_original_exe_version_value;
    if (g_data_version_label) {
      g_data_version_label->set_text_fmt(k_default_data_fmt, major, minor);
    }
    if (g_exe_version_label) {
      g_exe_version_label->set_text_fmt(k_default_exe_fmt, exe_value);
    }
  }

  auto apply_version_label_layout_internal() -> void {
    apply_version_label_style(g_data_version_label);
    apply_version_label_style(g_exe_version_label);
  }

  auto apply_logo_layout_internal(cps_title* self) -> void {
    if (!is_title_screen_active(self)) {
      return;
    }
    const int offset = control_panel().enabled ? control_panel().logo_y_offset : 0;
    apply_logo_y_offset(self,
                        resolve_title_logo(self, ext_client::offsets::cps_title::ui_ids::big_logo, "logo-big.ddj"),
                        ext_client::offsets::cps_title::ui_ids::big_logo,
                        offset,
                        "logo-big.ddj");
    apply_logo_y_offset(self,
                        resolve_title_logo(self, ext_client::offsets::cps_title::ui_ids::logo, "logo.ddj"),
                        ext_client::offsets::cps_title::ui_ids::logo,
                        offset,
                        "logo.ddj");
  }

  auto apply_logo_layout(cps_title* title) -> void {
    apply_logo_layout_internal(title);
  }

  auto restore_original_title_ui(cps_title* title) -> void {
    if (!title) {
      return;
    }
    // 1. Restore login frame texture
    auto* wnd = resolve_login_frame(title);
    if (wnd && wnd->is_live() && cif_static::is_static(wnd)) {
      char current_ddj[128]{};
      if (cif_static::read_ddj_path(wnd, current_ddj, sizeof(current_ddj))) {
        if (g_original_login_frame_ddj[0] != '\0' && !ddj_basename_matches(current_ddj, g_original_login_frame_ddj)) {
          set_static_texture(static_cast<cif_static*>(wnd), g_original_login_frame_ddj);
        }
      }
    }
    g_original_login_frame_ddj[0] = '\0';
    if (g_login_layout.load_from_game(title) && wnd) {
      g_login_layout.restore_eu_frame(title, wnd);
    }

    // 2. Restore channel widgets & list button
    auto show_title_child = [](cps_title* t, int res_id) {
      auto* child = t->find_child(res_id);
      if (child && child->is_live() && !child->visible()) {
        cgwnd::set_visible(child, true);
        child->m_visible = 1;
      }
    };
    show_title_child(title, 104);
    show_title_child(title, 45);
    show_title_child(title, ext_client::offsets::cps_title::ui_ids::channel_list_button);
    show_title_child(title, ext_client::offsets::cps_title::ui_ids::channel_info);

    // 3. Restore logos
    for (auto& entry : g_logo_baselines) {
      if (entry.widget && entry.widget->is_live()) {
        cgwnd::set_position(entry.widget, entry.widget->rect_x(), entry.y);
      }
    }
    g_logo_baselines.clear();

    // 4. Restore labels
    restore_original_version_labels();
    if (g_data_version_label) {
      g_data_version_label->set_text_color(0xFFFFFFFF);
      g_data_version_label->set_text_clip_mode(cif_text_clip_mode::full);
    }
    if (g_exe_version_label) {
      g_exe_version_label->set_text_color(0xFFFFFFFF);
      g_exe_version_label->set_text_clip_mode(cif_text_clip_mode::full);
    }
  }

  auto apply_title_ui(cps_title* title, bool force_apply) -> void {
    if (!title || !is_title_screen_active(title)) {
      return;
    }
    if (!control_panel().enabled) {
      restore_original_title_ui(title);
      return;
    }
    bind_title_instance(title);
    discover_version_labels(title);
    apply_login_frame_override(title);
    hide_channel_login_widgets(title);
    hide_channel_list_button(title, force_apply);
    apply_logo_layout_internal(title);
    if (control_panel().override_version_labels) {
      apply_version_label_overrides();
    } else {
      restore_original_version_labels();
    }
    apply_version_label_layout_internal();
  }

  auto bind_title_instance(cps_title* title) -> void {
    if (g_bound_title == title) {
      return;
    }
    reset_title_binding();
    g_bound_title = title;
  }

  auto tick() -> void {
    auto* title = cps_title::current();
    if (!title || !is_title_screen_active(title)) {
      if (g_bound_title) {
        reset_title_binding();
      }
      g_saw_title = false;
      g_server_ui_update_pending = false;
      return;
    }
    if (control_panel().logo_y_offset != 0) {
      bind_title_instance(title);
      apply_logo_layout_internal(title);
    }
    if (!g_saw_title) {
      g_saw_title = true;
      apply_title_ui(title, true);
    }

    if (GetTickCount() >= g_server_ui_update_time) {
      if (auto client_config = cgwnd::client_config()) {
        auto update_selected_server_ui = reinterpret_cast<void(__thiscall*)(cps_title*, const wchar_t*)>(ext_client::offsets::cps_title::functions::update_selected_server_ui);
        update_selected_server_ui(title, client_config->m_selected_server.c_str());
        g_server_ui_update_time = GetTickCount() + 5000;
      }
    } 
  }

  // =========================================================================
  // 5. Public API Functions Implementation
  // =========================================================================
  auto captured_version_label_count() -> int {
    return (g_data_version_label ? 1 : 0) + (g_exe_version_label ? 1 : 0);
  }

  auto apply_version_labels() -> void {
    if (auto* title = cps_title::current()) {
      discover_version_labels(title);
    }
    if (control_panel().override_version_labels) {
      apply_version_label_overrides();
    } else {
      restore_original_version_labels();
    }
    apply_version_label_layout_internal();
  }

  auto apply_version_label_layout() -> void {
    apply_version_label_layout_internal();
  }

  auto apply_from_control() -> void {
    auto* title = cps_title::current();
    if (title && is_title_screen_active(title)) {
      apply_title_ui(title, true);
    }
  }

  // =========================================================================
  // 6. Named Event Handlers Implementation
  // =========================================================================
  auto handle_tick(void*) -> void {
    tick();
  }

  auto handle_menu(void* raw_ctx) -> void {
    auto* ctx = static_cast<menu_draw_context*>(raw_ctx);
    if (!ctx || !ctx->menu_visible)
      return;

    if (ImGui::BeginTabItem("Login Screen")) {
      ext_client::utils::imgui_helpers::section_header("Custom Login Visuals Settings");

      auto& title = ext_client::core::config::data().title;
      bool changed = false;

      changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Enable Custom Login Visuals", &title.enabled);
      changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Hide Channel List Button", &title.hide_channel_list_button);
      changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Replace Login Background Frame", &title.replace_login_frame);
      if (title.replace_login_frame) {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 20.0f);
        changed |= ext_client::utils::imgui_helpers::input_text_dirty("Login DDJ Image Path", title.login_frame_path, sizeof(title.login_frame_path));
      }

      changed |= ext_client::utils::imgui_helpers::slider_int_dirty("Logo Y Offset", &title.logo_y_offset, -200, 200);

      ImGui::Spacing();
      if (ImGui::CollapsingHeader("Version Label Customization")) {
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Override System Version Labels", &title.override_version_labels);
        if (title.override_version_labels) {
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
          changed |= ext_client::utils::imgui_helpers::input_text_dirty("Data Version Format String", title.data_version_fmt, sizeof(title.data_version_fmt));
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
          changed |= ext_client::utils::imgui_helpers::input_text_dirty("Exe Version Format String", title.exe_version_fmt, sizeof(title.exe_version_fmt));
        }
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Override Version Label Color", &title.override_version_label_color);
        if (title.override_version_label_color) {
          float col[4] = {
            ((title.version_label_color & 0x00FF0000) >> 16) / 255.f,
            ((title.version_label_color & 0x0000FF00) >> 8) / 255.f,
            ((title.version_label_color & 0x000000FF) >> 0) / 255.f,
            ((title.version_label_color & 0xFF000000) >> 24) / 255.f};
          if (ImGui::ColorEdit4("Version Label Color", col)) {
            title.version_label_color =
              (static_cast<std::uint32_t>(col[3] * 255.f + 0.5f) << 24) |
              (static_cast<std::uint32_t>(col[0] * 255.f + 0.5f) << 16) |
              (static_cast<std::uint32_t>(col[1] * 255.f + 0.5f) << 8) |
              (static_cast<std::uint32_t>(col[2] * 255.f + 0.5f) << 0);
            changed = true;
          }
        }
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Clip Version Labels", &title.version_labels_clip);
        if (title.version_labels_clip) {
          changed |= ext_client::utils::imgui_helpers::slider_int_dirty("Ellipsis Clip Width", &title.version_label_ellipsis_width, 10, 300);
        }
      }

      ImGui::Spacing();
      if (ImGui::CollapsingHeader("EU Frame Layout Position Adjustments")) {
        float val[2];

        val[0] = title.eu_login_id_label_adjust.x;
        val[1] = title.eu_login_id_label_adjust.y;
        if (ext_client::utils::imgui_helpers::drag_float2_dirty("ID Label Adjust", val)) {
          title.eu_login_id_label_adjust.x = val[0];
          title.eu_login_id_label_adjust.y = val[1];
        }

        val[0] = title.eu_login_id_input_adjust.x;
        val[1] = title.eu_login_id_input_adjust.y;
        if (ext_client::utils::imgui_helpers::drag_float2_dirty("ID Input Adjust", val)) {
          title.eu_login_id_input_adjust.x = val[0];
          title.eu_login_id_input_adjust.y = val[1];
        }

        val[0] = title.eu_login_pw_label_adjust.x;
        val[1] = title.eu_login_pw_label_adjust.y;
        if (ext_client::utils::imgui_helpers::drag_float2_dirty("PW Label Adjust", val)) {
          title.eu_login_pw_label_adjust.x = val[0];
          title.eu_login_pw_label_adjust.y = val[1];
        }

        val[0] = title.eu_login_pw_input_adjust.x;
        val[1] = title.eu_login_pw_input_adjust.y;
        if (ext_client::utils::imgui_helpers::drag_float2_dirty("PW Input Adjust", val)) {
          title.eu_login_pw_input_adjust.x = val[0];
          title.eu_login_pw_input_adjust.y = val[1];
        }

        val[0] = title.eu_login_server_label_adjust.x;
        val[1] = title.eu_login_server_label_adjust.y;
        if (ext_client::utils::imgui_helpers::drag_float2_dirty("ServerLabel Adjust", val)) {
          title.eu_login_server_label_adjust.x = val[0];
          title.eu_login_server_label_adjust.y = val[1];
        }

        val[0] = title.eu_login_server_value_adjust.x;
        val[1] = title.eu_login_server_value_adjust.y;
        if (ext_client::utils::imgui_helpers::drag_float2_dirty("Server Value Adjust", val)) {
          title.eu_login_server_value_adjust.x = val[0];
          title.eu_login_server_value_adjust.y = val[1];
        }

        val[0] = title.eu_login_server_button_adjust.x;
        val[1] = title.eu_login_server_button_adjust.y;
        if (ext_client::utils::imgui_helpers::drag_float2_dirty("Server Button Adjust", val)) {
          title.eu_login_server_button_adjust.x = val[0];
          title.eu_login_server_button_adjust.y = val[1];
        }
      }

      if (changed) {
        apply_from_control();
      }

      ImGui::EndTabItem();
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("title", "Title Customizer", "Modifies login window, logo offsets, and custom text versions.");

    ADD_PLUGIN_EVENT("title", event_type::on_tick, handle_tick);
    ADD_PLUGIN_EVENT("title", event_type::on_menu, handle_menu);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::title
