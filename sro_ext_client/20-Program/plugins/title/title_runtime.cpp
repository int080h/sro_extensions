#include "pch.hpp"
#include "plugins/title/title_runtime.hpp"
#include "plugins/title/title_layout.hpp"

#include "core/config.hpp"
#include "core/event_bus.hpp"
#include "render/loading_splash_overlay.hpp"
#include "sdk/game/ccontroler.hpp"
#include "sdk/net/cclient_config.hpp"
#include "sdk/process/cps_title.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/runtime/gfx_runtime.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/cif_decorated_static.hpp"
#include "sdk/ui/cif_static.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"
#include "utils/string.hpp"

#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

using ext_client::utils::log_msg;

namespace ext_client::plugins::title {

  enum class label_type {
    none,
    data,
    exe,
  };

  inline constexpr wchar_t k_default_data_fmt[] = L"Client Data Version %d.%03d";
  inline constexpr wchar_t k_default_exe_fmt[] = L"Client Exe Version %.3f";
  inline constexpr char k_login_frame_id_ddj[] = "login_window_2016_id.ddj";
  inline constexpr char k_login_frame_email_ddj[] = "login_window_2016_email.ddj";
  inline constexpr char k_login_frame_default_ddj[] = "login_window.ddj";
  inline constexpr char k_login_frame_eu_located_ddj[] = "login_window_eu_located.ddj";

  inline constexpr int k_big_logo_default_y = 448;
  inline constexpr int k_logo_default_y = 403;
  inline constexpr int k_version_label_max_w = 520;
  inline constexpr int k_version_label_max_h = 40;

  struct logo_baseline_entry {
    cgwnd* widget = nullptr;
    int y = 0;
  };

  struct label_discover_ctx {
    cps_title* title = nullptr;
    bool need_data = true;
    bool need_exe = true;
  };

  struct login_frame_walk_ctx {
    cgwnd* found = nullptr;
  };

  struct channel_list_find_ctx {
    cgwnd* channel_combo = nullptr;
    cgwnd* found = nullptr;
  };

  struct title_list_button_collect_ctx {
    cgwnd* buttons[8]{};
    int count = 0;
  };

  struct logo_walk_ctx {
    const char* ddj_name = nullptr;
    cgwnd* found = nullptr;
  };

  auto is_title_screen_active(const cps_title* title) -> bool;
  auto bind_title_instance(cps_title* title) -> void;
  auto apply_title_ui(cps_title* title, bool force_apply) -> void;

  cif_static* g_data_version_label = nullptr;
  cif_static* g_exe_version_label = nullptr;
  double g_original_exe_version_value = 1.0;
  cps_title* g_bound_title = nullptr;

  bool g_saw_title = false;
  DWORD g_server_ui_update_time = 0;
  bool g_server_ui_update_pending = false;

  void* g_last_hid_channel_list_btn = nullptr;

  title_login_layout g_login_layout{};
  std::vector<logo_baseline_entry> g_logo_baselines{};
  char g_original_login_frame_ddj[128] = "";

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
        case 1/*screen_up*/:
        case 2/*screen_down*/:
        case 3/*border_screen_up*/:
        case 4/*border_screen_down*/:
        case 5/*title_text*/:
        case 7/*big_logo*/:
        case 8/*logo*/:
        case 9/*login_edit*/:
        case 10/*password_edit*/:
        case 11/*login_button*/:
        case 12/*exit_button*/:
        case 13/*server_combo*/:
        case 14/*channel_combo*/:
        case 15/*channel_info*/:
        case 44/*server_list_button*/:
        case 46/*channel_list_button*/:
        case 54/*slider*/:
        case 60/*channel_list_popup*/:
        case 70/*unity_server*/:
        case 500/*msgbox*/:
          return true;
        default:
          return false;
      }
    }

    auto ddj_basename_matches(const char* ddj, const char* name) -> bool {
      if (!ddj || !name) {
        return false;
      }
      const auto base = ext_client::utils::string::basename(ddj);
      return _stricmp(base.data(), name) == 0;
    }

    auto is_eu_located_ddj(const char* ddj) -> bool {
      if (!ddj) {
        return false;
      }
      const auto base = ext_client::utils::string::basename(ddj);

      char lower_base[128]{};
      std::strncpy(lower_base, base.data(), (std::min)(base.size(), sizeof(lower_base) - 1));
      for (int i = 0; lower_base[i]; i++) {
        lower_base[i] = static_cast<char>(std::tolower(lower_base[i]));
      }
      return std::strstr(lower_base, "login_window_eu_located") != nullptr;
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
      const auto* wnd = label;
      if (wnd->get_rect_w() > k_version_label_max_w || wnd->get_rect_h() > k_version_label_max_h) {
        return false;
      }
      const int screen_h = cgwnd::get_screen_height();
      if (screen_h > 0 && wnd->get_rect_y() < screen_h - 120) {
        return false;
      }
      if (title) {
        const int res_key = title->get_res_map_key_for(wnd);
        if (res_key >= 0 && is_title_chrome_res_id(res_key)) {
          return false;
        }
      }
      if (is_title_chrome_res_id(wnd->get_unique_id())) {
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
      auto* label = cif_static::static_label(widget);
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
      const auto cfg = ext_client::core::config::runtime();
      if (type == label_type::data && visit->need_data && !g_data_version_label) {
        g_data_version_label = label;
        visit->need_data = false;
        if (cfg->title.log_events) {
          log_msg("[title_plugin] discovered data label %p", label);
        }
      } else if (type == label_type::exe && visit->need_exe && !g_exe_version_label) {
        g_exe_version_label = label;
        g_original_exe_version_value = parse_exe_version_value(text);
        visit->need_exe = false;
        if (cfg->title.log_events) {
          log_msg("[title_plugin] discovered exe label %p (value=%.3f)", label, g_original_exe_version_value);
        }
      }
    }

    auto apply_version_label_style(cif_static* label) -> void {
      if (!label) {
        return;
      }
      const auto cfg = ext_client::core::config::runtime();
      const auto& title = cfg->title;
      if (title.enabled && title.override_version_label_color) {
        label->set_text_color(title.version_label_color);
      } else {
        label->set_text_color(0xFFFFFFFF);
      }
      const auto mode = title.enabled && title.version_labels_clip ? cif_text_clip_mode::ellipsis_hover : cif_text_clip_mode::full;
      label->set_text_clip_mode(mode, title.version_label_ellipsis_width);
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
      if (auto* wnd = title->find_child(9/*login_edit*/)) {
        if (wnd && wnd->is_live() && cif_static::is_static(wnd)) {
          char current_ddj[128]{};
          if (cif_static::read_ddj_path(wnd, current_ddj, sizeof(current_ddj)) && is_login_frame_ddj(current_ddj)) {
            return wnd;
          }
        }
      }
      login_frame_walk_ctx ctx{};
      title->walk_each(12, visit_find_login_frame, &ctx);
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

      const auto cfg = ext_client::core::config::runtime();
      const auto& title_cfg = cfg->title;
      if (title_cfg.enabled && title_cfg.replace_login_frame && title_cfg.login_frame_path[0]) {
        if (g_original_login_frame_ddj[0] == '\0' && !ddj_basename_matches(current_ddj, title_cfg.login_frame_path)) {
          std::strncpy(g_original_login_frame_ddj, current_ddj, sizeof(g_original_login_frame_ddj) - 1);
        }

        bool skip_replace = false;
        if (is_eu_located_ddj(title_cfg.login_frame_path) && is_eu_located_ddj(current_ddj)) {
          skip_replace = true;
        } else if (ddj_basename_matches(current_ddj, title_cfg.login_frame_path)) {
          skip_replace = true;
        }

        if (!skip_replace &&
            cif_static::static_label(wnd)->set_texture_path(title_cfg.login_frame_path) && title_cfg.log_events) {
          log_msg("[title_plugin] login frame %s -> %s", current_ddj, title_cfg.login_frame_path);
        }
        if (g_login_layout.load_from_game(title)) {
          g_login_layout.apply_eu_frame(title, wnd);
        }
      } else {
        if (g_original_login_frame_ddj[0] != '\0' && !ddj_basename_matches(current_ddj, g_original_login_frame_ddj)) {
          if (cif_static::static_label(wnd)->set_texture_path(g_original_login_frame_ddj)) {
            if (title_cfg.log_events) {
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
    }

    auto hide_channel_login_widgets(cps_title* self) -> void {
      if (!self || !is_title_screen_active(self)) {
        return;
      }
      const auto cfg = ext_client::core::config::runtime();
      if (cfg->title.enabled && cfg->title.replace_login_frame) {
        hide_title_child(self, 104);
        hide_title_child(self, 45);
        hide_title_child(self, 46/*channel_list_button*/);
        hide_title_child(self, 15/*channel_info*/);
      } else {
        show_title_child(self, 104);
        show_title_child(self, 45);
        show_title_child(self, 46/*channel_list_button*/);
        show_title_child(self, 15/*channel_info*/);
      }
    }

    auto is_channel_row_list_candidate(const cgwnd* wnd, const cgwnd* channel_combo) -> bool {
      if (!wnd || !wnd->is_live() || !channel_combo || !channel_combo->is_live()) {
        return false;
      }
      const int row_y = channel_combo->get_rect_y();
      const int row_right = channel_combo->get_rect_x() + channel_combo->get_rect_w();
      if (std::abs(wnd->get_rect_y() - row_y) > 16) {
        return false;
      }
      if (wnd->get_rect_x() < row_right - 40) {
        return false;
      }
      if (wnd->get_rect_w() <= 0 || wnd->get_rect_h() <= 0 || wnd->get_rect_w() > 120 || wnd->get_rect_h() > 60) {
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
      if (wnd->get_unique_id() == 46/*channel_list_button*/ && wnd->is_live()) {
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
      const int w = wnd->get_rect_w();
      const int h = wnd->get_rect_h();
      return w > 0 && h > 0 && w <= 200 && h <= 80;
    }

    auto is_channel_row_list_button(const cgwnd* wnd, const cgwnd* channel_combo) -> bool {
      if (!is_title_list_button_widget(wnd)) {
        return false;
      }
      if (!channel_combo || !channel_combo->is_live()) {
        return true;
      }
      const int row_y = channel_combo->get_rect_y();
      const int row_right = channel_combo->get_rect_x() + channel_combo->get_rect_w();
      return std::abs(wnd->get_rect_y() - row_y) <= 12 && wnd->get_rect_x() >= row_right - 24;
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
          const int xi = buttons[i]->get_rect_x();
          const int xj = buttons[j]->get_rect_x();
          const int yi = buttons[i]->get_rect_y();
          const int yj = buttons[j]->get_rect_y();
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
        const int y = buttons[i]->get_rect_y();
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
      cgwnd* root = title;
      if (!root || !root->is_live()) {
        return nullptr;
      }
      title_list_button_collect_ctx collect{};
      root->walk_each(12, collect_title_list_button, &collect);
      if (collect.count == 0) {
        return nullptr;
      }
      auto* channel_combo = title->find_child(14/*channel_combo*/);
      if (channel_combo && channel_combo->is_live()) {
        cgwnd* best = nullptr;
        int best_dx = INT_MAX;
        const int row_right = channel_combo->get_rect_x() + channel_combo->get_rect_w();
        for (int i = 0; i < collect.count; ++i) {
          auto* wnd = collect.buttons[i];
          if (!is_channel_row_list_button(wnd, channel_combo)) {
            continue;
          }
          const int dx = wnd->get_rect_x() - row_right;
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
      if (auto* widget = self->find_child(46/*channel_list_button*/)) {
        if (widget && widget->is_live()) {
          return widget;
        }
      }
      channel_list_find_ctx ctx{};
      ctx.channel_combo = self->find_child(14/*channel_combo*/);
      self->walk_each(14, visit_find_channel_list_button, &ctx);
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
      const auto cfg = ext_client::core::config::runtime();
      const auto& title_cfg = cfg->title;
      cgwnd* widget = find_channel_list_button(self);
      if (!widget || !widget->is_live()) {
        if (log_if_missing && title_cfg.log_events) {
          log_msg("[title_plugin] channel list button not found");
        }
        return;
      }

      if (title_cfg.enabled && title_cfg.hide_channel_list_button) {
        if (title_cfg.log_events && widget != g_last_hid_channel_list_btn) {
          g_last_hid_channel_list_btn = widget;
          log_msg("[title_plugin] hid channel list button %p", widget);
        }
        hide_title_widget(widget);
      } else {
        if (widget->is_live() && !widget->is_visible()) {
          cgwnd::set_visible(widget, true);
          if (title_cfg.log_events) {
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
      title->walk_each(12, visit_find_logo_ddj, &ctx);
      return ctx.found;
    }

    auto resolve_title_logo(cps_title* title, int res_id, const char* ddj_name) -> cgwnd* {
      if (!title || !cps_title::is_live(title)) {
        return nullptr;
      }
      if (auto* wnd = title->find_child(res_id)) {
        if (is_login_logo_widget(wnd, ddj_name)) {
          return wnd;
        }
      }
      return find_logo_by_ddj(title, ddj_name);
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

    auto capture_logo_baseline_y(cps_title* title, cgwnd* wnd, int res_id, const char* ddj_name) -> int {
      if (int* stored = find_logo_baseline(wnd)) {
        return *stored;
      }
      const int baseline_y = logo_default_y(ddj_name);
      g_logo_baselines.push_back({wnd, baseline_y});
      return baseline_y;
    }

    auto apply_logo_y_offset(cps_title* title, cgwnd* wnd, int res_id, int offset, const char* ddj_name) -> void {
      if (!wnd || !wnd->is_live()) {
        return;
      }
      const int baseline_y = capture_logo_baseline_y(title, wnd, res_id, ddj_name);
      const auto cfg = ext_client::core::config::runtime();
      if (cfg->title.enabled && offset != 0) {
        const int target_y = baseline_y - offset;
        if (wnd->get_rect_y() != target_y) {
          cgwnd::set_position(wnd, wnd->get_rect_x(), target_y);
        }
      } else {
        if (wnd->get_rect_y() != baseline_y) {
          cgwnd::set_position(wnd, wnd->get_rect_x(), baseline_y);
        }
      }
    }
  } // namespace

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
    title->walk_each(16, visit_discover_version_label, &ctx);
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

  struct version_parts {
    int major = 0;
    int minor = 0;
    double exe_value = 0.0;
  };

  auto current_version_parts() -> version_parts {
    const unsigned data_version = cgwnd::get_client_data_version();
    return {static_cast<int>(data_version / 1000u), static_cast<int>(data_version % 1000u), g_original_exe_version_value};
  }

  auto apply_version_label_overrides() -> void {
    const auto cfg = ext_client::core::config::runtime();
    const auto& title_cfg = cfg->title;
    if (!title_cfg.override_version_labels) {
      return;
    }
    sanitize_version_label_ptr(g_data_version_label);
    sanitize_version_label_ptr(g_exe_version_label);
    if (!g_data_version_label && !g_exe_version_label) {
      return;
    }
    wchar_t data_fmt_buf[128]{};
    wchar_t exe_fmt_buf[128]{};
    const wchar_t* data_fmt = resolve_wide_fmt(title_cfg.data_version_fmt, data_fmt_buf, 128, k_default_data_fmt);
    const wchar_t* exe_fmt = resolve_wide_fmt(title_cfg.exe_version_fmt, exe_fmt_buf, 128, k_default_exe_fmt);
    const auto vp = current_version_parts();
    apply_label_text(g_data_version_label, data_fmt, false, vp.major, vp.minor, vp.exe_value);
    apply_label_text(g_exe_version_label, exe_fmt, true, vp.major, vp.minor, vp.exe_value);
  }

  auto restore_original_version_labels() -> void {
    if (!g_data_version_label && !g_exe_version_label) {
      return;
    }
    const auto vp = current_version_parts();
    if (g_data_version_label) {
      g_data_version_label->set_text_fmt(k_default_data_fmt, vp.major, vp.minor);
    }
    if (g_exe_version_label) {
      g_exe_version_label->set_text_fmt(k_default_exe_fmt, vp.exe_value);
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
    const auto cfg = ext_client::core::config::runtime();
    const int offset = cfg->title.enabled ? cfg->title.logo_y_offset : 0;
    struct logo_spec { int res_id; const char* ddj_name; };
    for (const auto& spec : {logo_spec{7/*big_logo*/, "logo-big.ddj"},
                              logo_spec{8/*logo*/, "logo.ddj"}}) {
      apply_logo_y_offset(self,
                          resolve_title_logo(self, spec.res_id, spec.ddj_name),
                          spec.res_id,
                          offset,
                          spec.ddj_name);
    }
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
          cif_static::static_label(wnd)->set_texture_path(g_original_login_frame_ddj);
        }
      }
    }
    g_original_login_frame_ddj[0] = '\0';
    if (g_login_layout.load_from_game(title) && wnd) {
      g_login_layout.restore_eu_frame(title, wnd);
    }

    // 2. Restore channel widgets & list button
    show_title_child(title, 104);
    show_title_child(title, 45);
    show_title_child(title, 46/*channel_list_button*/);
    show_title_child(title, 15/*channel_info*/);

    // 3. Restore logos
    for (auto& entry : g_logo_baselines) {
      if (entry.widget && entry.widget->is_live()) {
        cgwnd::set_position(entry.widget, entry.widget->get_rect_x(), entry.y);
      }
    }
    g_logo_baselines.clear();

    // 4. Restore labels
    restore_original_version_labels();
    for (auto* label : {g_data_version_label, g_exe_version_label}) {
      if (label) {
        label->set_text_color(0xFFFFFFFF);
        label->set_text_clip_mode(cif_text_clip_mode::full);
      }
    }
  }

  auto apply_title_ui(cps_title* title, bool force_apply) -> void {
    if (!title || !is_title_screen_active(title)) {
      return;
    }
    const auto cfg = ext_client::core::config::runtime();
    const auto& title_cfg = cfg->title;
    if (!title_cfg.enabled) {
      restore_original_title_ui(title);
      return;
    }
    bind_title_instance(title);
    discover_version_labels(title);
    apply_login_frame_override(title);
    hide_channel_login_widgets(title);
    hide_channel_list_button(title, force_apply);
    apply_logo_layout_internal(title);
    if (title_cfg.override_version_labels) {
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
    if (ext_client::render::loading_splash_overlay::is_running()) {
      ext_client::render::loading_splash_overlay::stop();
    }
    const auto cfg = ext_client::core::config::runtime();
    if (cfg->title.logo_y_offset != 0) {
      bind_title_instance(title);
      apply_logo_layout_internal(title);
    }
    if (!g_saw_title) {
      g_saw_title = true;
      apply_title_ui(title, true);
    }

    if (GetTickCount() >= g_server_ui_update_time) {
      if (auto client_config = cgwnd::get_client_config()) {
        using update_server_ui_fn = void(__thiscall*)(cps_title*, const wchar_t*);
        const auto update_selected_server_ui = ext_client::off::as_fn<update_server_ui_fn>(0x00970160);
        if (update_selected_server_ui) {
          update_selected_server_ui(title, client_config->get_selected_server().c_str());
        }
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
    const auto cfg = ext_client::core::config::runtime();
    if (cfg->title.override_version_labels) {
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

  auto handle_tick() -> void {
    tick();
  }
} // namespace ext_client::plugins::title
