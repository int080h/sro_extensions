#include "hooks/title_hook.hpp"

#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <vector>
#include <string>

#include "hooks/title_login_layout.hpp"
#include "hooks/version_check_hook.hpp"
#include "sdk/calarm_guide_mgr_wnd.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cif_manager.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "sdk/cps_character_select.hpp"
#include "sdk/cps_title.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/process_manager.hpp"
#include "utils/ui_res_catalog.hpp"
#include "utils/string.hpp"

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::title {
  namespace {

    enum class label_type {
      none,
      data,
      exe,
    };

    constexpr wchar_t k_default_data_fmt[] = L"Client Data Version %d.%03d";
    constexpr wchar_t k_default_exe_fmt[] = L"Client Exe Version %.3f";
    constexpr char k_login_frame_id_ddj[] = "login_window_2016_id.ddj";
    constexpr char k_login_frame_email_ddj[] = "login_window_2016_email.ddj";
    constexpr char k_login_frame_default_ddj[] = "login_window.ddj";
    constexpr char k_login_frame_eu_located_ddj[] = "login_window_eu_located.ddj";

    cif_static* g_data_version_label = nullptr;
    cif_static* g_exe_version_label = nullptr;
    double g_original_exe_version_value = 1.0;
    cps_title* g_bound_title = nullptr;

    void* g_last_active_child = nullptr;
    bool g_saw_title = false;
    bool g_auto_login_done = false;
    std::uint32_t g_enforce_counter = 0;

    constexpr std::uint32_t k_enforce_every_n_ticks = 15;

    constexpr int k_big_logo_default_y = 448;
    constexpr int k_logo_default_y = 403;

    void* g_last_hid_channel_list_btn = nullptr;
    cgwnd* g_cached_channel_list_btn = nullptr;
    cps_title* g_cached_channel_list_title = nullptr;
    title_login_layout g_login_layout{};

    struct logo_baseline_entry {
      cgwnd* widget = nullptr;
      int y = 0;
    };

    struct string_pod {
      std::uint32_t words[7];
    };

    std::vector<logo_baseline_entry> g_logo_baselines{};

    make_hook<convention_type::thiscall_t, char, cps_title*, int> g_on_create;
    make_hook<convention_type::thiscall_t, int, cps_title*, cmsg_stream_buffer*> g_on_msg_recv;
    hook_group g_hooks;

    auto needs_packet_hooks() -> bool {
      return control_panel().enabled;
    }

    auto title_ui_child(cps_title* title, int res_id) -> cgwnd*;

    auto needs_create_hook() -> bool {
      return control_panel().enabled && control_panel().skip_on_create;
    }

    auto needs_any_hook() -> bool {
      return needs_create_hook() || needs_packet_hooks() || control_panel().enabled;
    }

    auto __fastcall on_create_detour(cps_title* self, void*, int mode) -> char {
      if (control_panel().enabled && control_panel().skip_on_create) {
        if (control_panel().log_events) {
          log_msg("[title_hook] OnCreate skipped (self=%p mode=%d)", self, mode);
        }
        return 1;
      }

      return g_on_create.call_original(self, mode);
    }

    auto __fastcall on_msg_recv_detour(cps_title* self, void*, cmsg_stream_buffer* packet) -> int {
      if (!control_panel().enabled) {
        return g_on_msg_recv.call_original(self, packet);
      }

      const auto opcode = packet->opcode();
      if (control_panel().log_events) {
        log_msg("[title_hook] OnMsgRecv opcode=0x%04X", opcode);
      }

      if (control_panel().block_login_packets) {
        switch (opcode) {
          case ext_client::offsets::cps_title::packets::auth_response:
          case ext_client::offsets::cps_title::packets::auth_notify:
          case ext_client::offsets::cps_title::packets::login_response:
          case ext_client::offsets::cps_title::packets::login_msgbox:
          case ext_client::offsets::cps_title::packets::captcha_begin:
          case ext_client::offsets::cps_title::packets::captcha_end:
            if (control_panel().log_events) {
              log_msg("[title_hook] blocked login packet 0x%04X", opcode);
            }
            return 0;
          default:
            break;
        }
      }

      if (control_panel().suppress_captcha && (opcode == ext_client::offsets::cps_title::packets::captcha_begin ||
                                         opcode == ext_client::offsets::cps_title::packets::captcha_end)) {
        return 0;
      }

      const int result = g_on_msg_recv.call_original(self, packet);

      if (opcode == ext_client::offsets::cps_title::packets::auth_notify) {
        if (control_panel().log_events) {
          log_msg("[title_hook] auth notify received, manually updating selected server UI");
        }
        void* client_config = cgwnd::client_config();
        if (client_config) {
          const void* wstr_ptr =
            static_cast<const char*>(client_config) + ext_client::offsets::cgwnd::client_config_fields::selected_server;
          auto ref = ext_client::msvc9::wstring_ref::from(wstr_ptr);
          const wchar_t* server_name = ref.data();
          if (server_name && server_name[0] != L'\0') {
            if (control_panel().log_events) {
              log_msg("[title_hook] updating selected server UI for %ls", server_name);
            }
            using update_selected_server_ui_fn = void(__thiscall*)(cps_title*, const wchar_t*);
            auto update_selected_server_ui =
              reinterpret_cast<update_selected_server_ui_fn>(ext_client::offsets::cps_title::functions::update_selected_server_ui);
            update_selected_server_ui(self, server_name);
          }
        }
      }

      return result;
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

    constexpr int k_version_label_max_w = 520;
    constexpr int k_version_label_max_h = 40;

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
        const int res_key = cif_manager::res_map_key_for(title, wnd);
        if (res_key >= 0 && is_title_chrome_res_id(res_key)) {
          return false;
        }
      }

      if (is_title_chrome_res_id(wnd->unique_id())) {
        return false;
      }

      wchar_t text[256]{};
      if (!cif_manager::read_static_text(label, text, 256) || text[0] == L'\0') {
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
      g_auto_login_done = false;
      g_enforce_counter = 0;
      clear_version_label_capture();
      clear_channel_list_cache();
      clear_logo_baselines();
    }

    auto is_title_screen_active(cps_title* self) -> bool {
      if (!cps_title::is_instance(self)) {
        return false;
      }
      if (cps_character_select::is_live(cps_character_select::current())) {
        return false;
      }
      return true;
    }

    auto sync_active_screens() -> cps_title* {
      const void* active = ext_client::utils::process_manager::active_child();
      if (active != g_last_active_child) {
        g_last_active_child = const_cast<void*>(active);
        calarm_guide_mgr_wnd::invalidate_cache();
      }

      auto* title = cps_title::sync_current();
      auto* character_select = cps_character_select::sync_current();
      if (character_select) {
        cps_title::set_current(nullptr);
        title = nullptr;
      }

      if (!title) {
        reset_title_binding();
      }
      return title;
    }

    auto bind_title_instance(cps_title* title) -> void {
      if (g_bound_title == title) {
        return;
      }
      reset_title_binding();
      g_bound_title = title;
    }

    struct label_discover_ctx {
      cps_title* title = nullptr;
      bool need_data = true;
      bool need_exe = true;
    };

    auto visit_discover_version_label(cgwnd* widget, void* raw) -> void {
      auto* visit = static_cast<label_discover_ctx*>(raw);
      if (!visit || (!visit->need_data && !visit->need_exe)) {
        return;
      }
      auto* label = cif_manager::as_static_if(widget);
      if (!label || !is_plausible_version_label(visit->title, label)) {
        return;
      }

      wchar_t text[256]{};
      if (!cif_manager::read_static_text(label, text, 256)) {
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
          log_msg("[title_hook] discovered data label %p", label);
        }
      } else if (type == label_type::exe && visit->need_exe && !g_exe_version_label) {
        g_exe_version_label = label;
        g_original_exe_version_value = parse_exe_version_value(text);
        visit->need_exe = false;
        if (control_panel().log_events) {
          log_msg("[title_hook] discovered exe label %p (value=%.3f)", label, g_original_exe_version_value);
        }
      }
    }

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
      cif_manager::walk_each(reinterpret_cast<cgwnd*>(title), 16, visit_discover_version_label, &ctx);
    }

    auto apply_version_label_layout_for(cif_static* label) -> void {
      if (!label) {
        return;
      }
      const auto mode = control_panel().version_labels_clip ? cif_text_clip_mode::ellipsis_hover : cif_text_clip_mode::full;
      label->set_text_clip_mode(mode, control_panel().version_label_ellipsis_width);
    }

    auto apply_version_label_layout_internal() -> void {
      apply_version_label_layout_for(g_data_version_label);
      apply_version_label_layout_for(g_exe_version_label);
    }

    auto apply_label_text(cif_static* label, const wchar_t* fmt, bool is_exe_fmt, int major, int minor, double exe_value) -> void {
      if (!label || !fmt) {
        return;
      }

      if (control_panel().override_version_label_color) {
        label->set_text_color(control_panel().version_label_color);
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

      apply_version_label_layout_for(label);
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

    auto hide_title_widget(cgwnd* widget) -> void {
      if (!cif_manager::is_live_widget(widget)) {
        return;
      }

      cgwnd::set_visible(widget, false);
      widget->m_visible = 0;
    }

    auto title_ui_child(cps_title* title, int res_id) -> cgwnd* {
      return ext_client::cif_manager::find_title_child(title, res_id);
    }

    auto ddj_basename_matches(const char* ddj, const char* name) -> bool {
      if (!ddj || !name) {
        return false;
      }
      const char* base = std::strrchr(ddj, '\\');
      base = base ? base + 1 : ddj;
      return std::strcmp(base, name) == 0;
    }

    auto set_static_texture(cif_static* widget, const char* path) -> bool {
      if (!widget || !path || path[0] == '\0') {
        return false;
      }

      auto* image_sub =
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(widget) + ext_client::offsets::cif_static::fields::image_subobject);
      if (!ext_client::msvc9::is_readable_ptr(image_sub, sizeof(void*) * 4)) {
        return false;
      }

      const auto* image_vftable = *reinterpret_cast<void***>(image_sub);
      if (!ext_client::msvc9::is_readable_ptr(image_vftable, 0x38)) {
        return false;
      }

      using set_texture_fn = void(__thiscall*)(void*, string_pod, int, int);
      const auto set_tex_fn =
        reinterpret_cast<set_texture_fn>(image_vftable[ext_client::offsets::cif_static::cif_image_renderer::vtable::set_texture]);
      if (!set_tex_fn) {
        return false;
      }

      ext_client::msvc9::string s(path);
      string_pod pod{};
      std::memcpy(&pod, s.raw(), sizeof(pod));

      ext_client::msvc9::string empty_s;
      std::memcpy(s.raw(), empty_s.raw(), sizeof(pod));

      set_tex_fn(image_sub, pod, 0, 0);
      return true;
    }

    auto is_login_frame_ddj(const char* ddj) -> bool {
      return ddj_basename_matches(ddj, k_login_frame_id_ddj) || ddj_basename_matches(ddj, k_login_frame_email_ddj) ||
             ddj_basename_matches(ddj, k_login_frame_default_ddj) || ddj_basename_matches(ddj, k_login_frame_eu_located_ddj);
    }

    struct login_frame_walk_ctx {
      cgwnd* found = nullptr;
    };

    auto visit_find_login_frame(cgwnd* wnd, void* raw) -> void {
      auto* ctx = static_cast<login_frame_walk_ctx*>(raw);
      if (!ctx || ctx->found || !cif_manager::is_live_widget(wnd) || !cif_manager::is_static(wnd)) {
        return;
      }

      char current_ddj[128]{};
      if (cif_manager::read_widget_ddj_path(wnd, current_ddj, sizeof(current_ddj)) && is_login_frame_ddj(current_ddj)) {
        ctx->found = wnd;
      }
    }

    auto resolve_login_frame(cps_title* title) -> cgwnd* {
      if (auto* wnd = title_ui_child(title, ext_client::offsets::cps_title::ui_ids::login_edit)) {
        if (cif_manager::is_live_widget(wnd) && cif_manager::is_static(wnd)) {
          char current_ddj[128]{};
          if (cif_manager::read_widget_ddj_path(wnd, current_ddj, sizeof(current_ddj)) && is_login_frame_ddj(current_ddj)) {
            return wnd;
          }
        }
      }

      login_frame_walk_ctx ctx{};
      cif_manager::walk_each(reinterpret_cast<cgwnd*>(title), 12, visit_find_login_frame, &ctx);
      return ctx.found;
    }

    auto apply_eu_login_row_offsets(cps_title* title, cgwnd* frame) -> void {
      if (!title || !cif_manager::is_live_widget(frame)) {
        return;
      }

      if (!g_login_layout.load_from_game(title)) {
        if (control_panel().log_events) {
          log_msg("[title_hook] login layout descriptors missing from game pstitle document");
        }
        return;
      }

      g_login_layout.apply_eu_frame(title, frame);
    }

    auto apply_login_frame_override(cps_title* title) -> void {
      if (!control_panel().replace_login_frame || !control_panel().login_frame_path[0]) {
        return;
      }

      auto* wnd = resolve_login_frame(title);
      if (!cif_manager::is_live_widget(wnd) || !cif_manager::is_static(wnd)) {
        return;
      }

      char current_ddj[128]{};
      if (!cif_manager::read_widget_ddj_path(wnd, current_ddj, sizeof(current_ddj)) || !is_login_frame_ddj(current_ddj)) {
        return;
      }

      if (!ddj_basename_matches(current_ddj, k_login_frame_eu_located_ddj) &&
          set_static_texture(static_cast<cif_static*>(wnd), control_panel().login_frame_path) && control_panel().log_events) {
        log_msg("[title_hook] login frame %s -> %s", current_ddj, control_panel().login_frame_path);
      }
      apply_eu_login_row_offsets(title, wnd);
    }

    struct channel_list_find_ctx {
      cgwnd* channel_combo = nullptr;
      cgwnd* found = nullptr;
    };

    auto is_channel_row_list_candidate(const cgwnd* wnd, const cgwnd* channel_combo) -> bool {
      if (!cif_manager::is_live_widget(wnd) || !cif_manager::is_live_widget(channel_combo)) {
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
      if (!cif_manager::read_widget_ddj_path(wnd, ddj, sizeof(ddj))) {
        return true;
      }
      return std::strstr(ddj, "list_button") != nullptr;
    }

    auto visit_find_channel_list_button(cgwnd* wnd, void* raw) -> void {
      auto* ctx = static_cast<channel_list_find_ctx*>(raw);
      if (!ctx || ctx->found) {
        return;
      }

      if (cif_manager::unique_id(wnd) == ext_client::offsets::cps_title::ui_ids::channel_list_button && cif_manager::is_live_widget(wnd)) {
        ctx->found = wnd;
        return;
      }

      if (is_channel_row_list_candidate(wnd, ctx->channel_combo)) {
        ctx->found = wnd;
      }
    }

    auto find_channel_list_button(cps_title* self) -> cgwnd* {
      if (auto* widget = title_ui_child(self, ext_client::offsets::cps_title::ui_ids::channel_list_button)) {
        if (cif_manager::is_live_widget(widget)) {
          return widget;
        }
      }

      channel_list_find_ctx ctx{};
      ctx.channel_combo = title_ui_child(self, ext_client::offsets::cps_title::ui_ids::channel_combo);
      cif_manager::walk_each(reinterpret_cast<cgwnd*>(self), 14, visit_find_channel_list_button, &ctx);
      if (ctx.found) {
        return ctx.found;
      }

      if (auto* widget = cif_manager::find_title_channel_list_button(self)) {
        if (!ctx.channel_combo || is_channel_row_list_candidate(widget, ctx.channel_combo)) {
          return widget;
        }
      }
      return nullptr;
    }

    auto hide_channel_list_button(cps_title* self, bool log_if_missing) -> void {
      if (!control_panel().hide_channel_list_button) {
        return;
      }
      if (!is_title_screen_active(self)) {
        clear_channel_list_cache();
        return;
      }

      cgwnd* widget = nullptr;
      if (g_cached_channel_list_title == self && cif_manager::is_live_widget(g_cached_channel_list_btn)) {
        widget = g_cached_channel_list_btn;
      } else {
        clear_channel_list_cache();
        widget = find_channel_list_button(self);
        if (!cif_manager::is_live_widget(widget)) {
          if (log_if_missing && control_panel().log_events) {
            log_msg("[title_hook] channel list button (res %d) not found", ext_client::offsets::cps_title::ui_ids::channel_list_button);
          }
          return;
        }
        g_cached_channel_list_btn = widget;
        g_cached_channel_list_title = self;
      }

      if (control_panel().log_events && widget != g_last_hid_channel_list_btn) {
        g_last_hid_channel_list_btn = widget;
        log_msg("[title_hook] hid channel list button %p (res %d)", widget, ext_client::offsets::cps_title::ui_ids::channel_list_button);
      }
      hide_title_widget(widget);
    }

    auto find_title_child_by_id(cps_title* self, int res_id) -> cgwnd* {
      return ext_client::cif_manager::find_title_child(self, res_id);
    }

    auto hide_title_child_by_id(cps_title* self, int res_id) -> void {
      auto* widget = find_title_child_by_id(self, res_id);
      if (cif_manager::is_live_widget(widget)) {
        hide_title_widget(widget);
      }
    }

    auto hide_channel_login_widgets(cps_title* self) -> void {
      if (!control_panel().replace_login_frame || !is_title_screen_active(self)) {
        return;
      }

      hide_title_child_by_id(self, 104); // CH label
      hide_title_child_by_id(self, 45);  // channel name/value
      hide_title_child_by_id(self, ext_client::offsets::cps_title::ui_ids::channel_list_button);
      hide_title_child_by_id(self, ext_client::offsets::cps_title::ui_ids::channel_info);
    }

    auto is_login_logo_widget(const cgwnd* wnd, const char* ddj_name) -> bool {
      if (!cif_manager::is_live_widget(wnd)) {
        return false;
      }
      char ddj[128]{};
      if (!cif_manager::read_widget_ddj_path(wnd, ddj, sizeof(ddj))) {
        return false;
      }
      return ddj_basename_matches(ddj, ddj_name);
    }

    struct logo_walk_ctx {
      const char* ddj_name = nullptr;
      cgwnd* found = nullptr;
    };

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
      cif_manager::walk_each(reinterpret_cast<cgwnd*>(title), 12, visit_find_logo_ddj, &ctx);
      return ctx.found;
    }

    auto resolve_title_logo(cps_title* title, int res_id, const char* ddj_name) -> cgwnd* {
      if (auto* wnd = title_ui_child(title, res_id)) {
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
      if (!cif_manager::is_live_widget(wnd) || offset == 0) {
        return;
      }

      const int baseline_y = capture_logo_baseline_y(title, wnd, res_id, ddj_name);
      const int target_y = baseline_y - offset;
      if (wnd->rect_y() != target_y) {
        cgwnd::set_position(wnd, wnd->rect_x(), target_y);
      }
    }

    auto apply_logo_layout_internal(cps_title* self) -> void {
      if (!is_title_screen_active(self) || control_panel().logo_y_offset == 0) {
        return;
      }

      const int offset = control_panel().logo_y_offset;
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

    auto maybe_auto_login(cps_title* self) -> void {
      if (!control_panel().enabled || !control_panel().auto_login_on_create || g_auto_login_done || !self) {
        return;
      }

      g_auto_login_done = true;
      if (control_panel().log_events) {
        log_msg("[title_hook] auto DoLogin");
      }
      self->trigger_login();
    }

    auto apply_title_ui(cps_title* title, bool log_channel_miss) -> void {
      if (!control_panel().enabled || !title || !is_title_screen_active(title)) {
        return;
      }

      bind_title_instance(title);
      discover_version_labels(title);
      apply_login_frame_override(title);
      hide_channel_login_widgets(title);
      hide_channel_list_button(title, log_channel_miss);
      apply_logo_layout_internal(title);

      if (control_panel().override_version_labels) {
        apply_version_label_overrides();
      } else {
        restore_original_version_labels();
      }
      apply_version_label_layout_internal();

      maybe_auto_login(title);
    }

  } // namespace

  auto control_panel() -> control& {
    return ext_client::config::data().title;
  }

  auto on_screen_transition() -> void {
    calarm_guide_mgr_wnd::invalidate_cache();
    sync_active_screens();
  }

  auto on_set_child_process(int /*process_type*/, int /*activate*/) -> void {
    on_screen_transition();
  }

  auto data_version_label() -> cif_static* {
    return g_data_version_label;
  }

  auto exe_version_label() -> cif_static* {
    return g_exe_version_label;
  }

  auto captured_version_label_count() -> int {
    return (g_data_version_label ? 1 : 0) + (g_exe_version_label ? 1 : 0);
  }

  auto set_version_label_clip_mode(cif_text_clip_mode mode, int ellipsis_width) -> void {
    if (g_data_version_label) {
      g_data_version_label->set_text_clip_mode(mode, ellipsis_width);
    }
    if (g_exe_version_label) {
      g_exe_version_label->set_text_clip_mode(mode, ellipsis_width);
    }
  }

  auto apply_version_label_layout() -> void {
    apply_version_label_layout_internal();
  }

  auto apply_logo_layout() -> void {
    if (auto* title = cps_title::current()) {
      apply_logo_layout_internal(title);
    }
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

  auto apply_from_control() -> void {
    auto* title = sync_active_screens();
    if (title && is_title_screen_active(title)) {
      apply_title_ui(title, true);
    }
  }

  auto tick() -> void {
    auto* title = sync_active_screens();

    if (!title || !is_title_screen_active(title)) {
      g_saw_title = false;
      g_enforce_counter = 0;
      return;
    }

    if (control_panel().logo_y_offset != 0) {
      bind_title_instance(title);
      apply_logo_layout_internal(title);
    }

    if (!g_saw_title) {
      g_saw_title = true;
      apply_title_ui(title, true);
      g_enforce_counter = 0;
      return;
    }

    if (++g_enforce_counter < k_enforce_every_n_ticks) {
      return;
    }
    g_enforce_counter = 0;
    apply_title_ui(title, false);
  }

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    if (!needs_any_hook()) {
      log_msg("[title_hook] hook-free title UI (tick poll only)");
      return true;
    }

    using namespace ext_client::offsets::cps_title;
    bool ok = true;

    if (needs_create_hook()) {
      ok = g_hooks.install(g_on_create, functions::on_create, &on_create_detour, "title_hook", "on_create");
    }
    if (ok && needs_packet_hooks()) {
      ok = g_hooks.install(g_on_msg_recv, functions::on_msg_recv, &on_msg_recv_detour, "title_hook", "on_msg_recv");
    }

    if (!ok) {
      g_hooks.uninstall();
      return false;
    }

    log_msg("[title_hook] hooks installed");
    return true;
  }

  auto uninstall() -> void {
    if (!g_hooks.is_installed()) {
      return;
    }

    g_hooks.uninstall();
    log_msg("[title_hook] hooks removed");
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

} // namespace ext_client::hooks::title
