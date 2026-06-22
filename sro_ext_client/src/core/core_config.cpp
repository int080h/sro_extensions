#include "core/core_config.hpp"

#include "core/core_main.hpp"
#include "core/core_event_manager.hpp"
#include "render/render_system.hpp"
#include "plugins/hud_customizer_plugin.hpp"
#include "core/core_plugin_manager.hpp"
#include "utils/log.hpp"
#include "utils/string.hpp"

#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>

using ext_client::utils::log_msg;

namespace ext_client::core::config {
  namespace {

    core_config g_settings{};
    char g_path[MAX_PATH]{};
    bool g_dirty = false;
    bool g_path_ready = false;

    auto parse_bool(const char* value, bool fallback) -> bool {
      if (!value) {
        return fallback;
      }
      if (std::strcmp(value, "1") == 0 || _stricmp(value, "true") == 0 || _stricmp(value, "yes") == 0) {
        return true;
      }
      if (std::strcmp(value, "0") == 0 || _stricmp(value, "false") == 0 || _stricmp(value, "no") == 0) {
        return false;
      }
      return fallback;
    }

    auto parse_int(const char* value, int fallback) -> int {
      if (!value || value[0] == '\0') {
        return fallback;
      }
      return atoi(value);
    }

    auto ensure_path() -> void {
      if (g_path_ready) {
        return;
      }

      g_path[0] = '\0';
      HMODULE h_mod = app_main::get().get_module();
      if (h_mod) {
        GetModuleFileNameA(h_mod, g_path, MAX_PATH);
        char* slash = std::strrchr(g_path, '\\');
        if (!slash) {
          slash = std::strrchr(g_path, '/');
        }
        if (slash) {
          slash[1] = '\0';
        }
        std::strncat(g_path, "ext_client.ini", MAX_PATH - std::strlen(g_path) - 1);
      } else {
        std::strncpy(g_path, "ext_client.ini", sizeof(g_path) - 1);
      }
      g_path_ready = true;
    }

    // =========================================================================
    // Field Table
    // =========================================================================
    enum class field_type : std::uint8_t {
      bool_t,
      int_t,
      float_t,
      string_t,
      hex16_t,
      hex32_t,
    };

    struct field_desc {
      const char* section;
      const char* key;
      void* ptr;
      field_type type;
      std::size_t size;
    };

    static const field_desc g_fields[] = {
      {"general", "save_on_change", &g_settings.general.save_on_change, field_type::bool_t, 0},

      {"graphic", "d3d_force_hardware_vp", &g_settings.graphic.d3d_force_hardware_vp, field_type::bool_t, 0},
      {"graphic", "d3d_force_pure_device", &g_settings.graphic.d3d_force_pure_device, field_type::bool_t, 0},
      {"graphic", "d3d_triple_buffering", &g_settings.graphic.d3d_triple_buffering, field_type::bool_t, 0},
      {"graphic", "d3d_discard_depth_stencil", &g_settings.graphic.d3d_discard_depth_stencil, field_type::bool_t, 0},

      {"welcome_msg", "enabled", &g_settings.welcome_msg.enabled, field_type::bool_t, 0},
      {"welcome_msg", "hide", &g_settings.welcome_msg.hide, field_type::bool_t, 0},
      {"welcome_msg", "text", g_settings.welcome_msg.text, field_type::string_t, sizeof(g_settings.welcome_msg.text)},

      {"title", "enabled", &g_settings.title.enabled, field_type::bool_t, 0},
      {"title", "log_events", &g_settings.title.log_events, field_type::bool_t, 0},
      {"title", "hide_channel_list_button", &g_settings.title.hide_channel_list_button, field_type::bool_t, 0},
      {"title", "replace_login_frame", &g_settings.title.replace_login_frame, field_type::bool_t, 0},
      {"title", "login_frame_path", g_settings.title.login_frame_path, field_type::string_t, sizeof(g_settings.title.login_frame_path)},
      {"title", "eu_login_id_label_adjust_x", &g_settings.title.eu_login_id_label_adjust.x, field_type::float_t, 0},
      {"title", "eu_login_id_label_adjust_y", &g_settings.title.eu_login_id_label_adjust.y, field_type::float_t, 0},
      {"title", "eu_login_id_input_adjust_x", &g_settings.title.eu_login_id_input_adjust.x, field_type::float_t, 0},
      {"title", "eu_login_id_input_adjust_y", &g_settings.title.eu_login_id_input_adjust.y, field_type::float_t, 0},
      {"title", "eu_login_pw_label_adjust_x", &g_settings.title.eu_login_pw_label_adjust.x, field_type::float_t, 0},
      {"title", "eu_login_pw_label_adjust_y", &g_settings.title.eu_login_pw_label_adjust.y, field_type::float_t, 0},
      {"title", "eu_login_pw_input_adjust_x", &g_settings.title.eu_login_pw_input_adjust.x, field_type::float_t, 0},
      {"title", "eu_login_pw_input_adjust_y", &g_settings.title.eu_login_pw_input_adjust.y, field_type::float_t, 0},
      {"title", "eu_login_server_label_adjust_x", &g_settings.title.eu_login_server_label_adjust.x, field_type::float_t, 0},
      {"title", "eu_login_server_label_adjust_y", &g_settings.title.eu_login_server_label_adjust.y, field_type::float_t, 0},
      {"title", "eu_login_server_value_adjust_x", &g_settings.title.eu_login_server_value_adjust.x, field_type::float_t, 0},
      {"title", "eu_login_server_value_adjust_y", &g_settings.title.eu_login_server_value_adjust.y, field_type::float_t, 0},
      {"title", "eu_login_server_button_adjust_x", &g_settings.title.eu_login_server_button_adjust.x, field_type::float_t, 0},
      {"title", "eu_login_server_button_adjust_y", &g_settings.title.eu_login_server_button_adjust.y, field_type::float_t, 0},
      {"title", "logo_y_offset", &g_settings.title.logo_y_offset, field_type::int_t, 0},
      {"title", "override_version_labels", &g_settings.title.override_version_labels, field_type::bool_t, 0},
      {"title", "override_version_label_color", &g_settings.title.override_version_label_color, field_type::bool_t, 0},
      {"title", "version_labels_clip", &g_settings.title.version_labels_clip, field_type::bool_t, 0},
      {"title", "data_version_fmt", g_settings.title.data_version_fmt, field_type::string_t, sizeof(g_settings.title.data_version_fmt)},
      {"title", "exe_version_fmt", g_settings.title.exe_version_fmt, field_type::string_t, sizeof(g_settings.title.exe_version_fmt)},
      {"title", "version_label_color", &g_settings.title.version_label_color, field_type::hex32_t, 0},
      {"title", "version_label_ellipsis_width", &g_settings.title.version_label_ellipsis_width, field_type::int_t, 0},

      {"version_check", "enabled", &g_settings.version_check.enabled, field_type::bool_t, 0},
      {"version_check", "log_events", &g_settings.version_check.log_events, field_type::bool_t, 0},
      {"version_check", "banner_custom_size", &g_settings.version_check.banner_custom_size, field_type::bool_t, 0},
      {"version_check", "banner_width", &g_settings.version_check.banner_width, field_type::int_t, 0},
      {"version_check", "banner_height", &g_settings.version_check.banner_height, field_type::int_t, 0},
      {"version_check", "banner_center", &g_settings.version_check.banner_center, field_type::bool_t, 0},
      {"version_check", "banner_x", &g_settings.version_check.banner_x, field_type::int_t, 0},
      {"version_check", "banner_y", &g_settings.version_check.banner_y, field_type::int_t, 0},
      {"version_check", "banner_cycle", &g_settings.version_check.banner_cycle, field_type::bool_t, 0},
      {"version_check", "banner_cycle_interval_ms", &g_settings.version_check.banner_cycle_interval_ms, field_type::int_t, 0},
      {"version_check", "banner_count", &g_settings.version_check.banner_count, field_type::int_t, 0},
      {"version_check", "banner_path_fmt", g_settings.version_check.banner_path_fmt, field_type::string_t, sizeof(g_settings.version_check.banner_path_fmt)},
      {"version_check", "banner_overlay", &g_settings.version_check.banner_overlay, field_type::bool_t, 0},
      {"version_check", "ensure_minimize_button", &g_settings.version_check.ensure_minimize_button, field_type::bool_t, 0},

      {"net", "enabled", &g_settings.net.enabled, field_type::bool_t, 0},
      {"net", "pause_capture", &g_settings.net.pause_capture, field_type::bool_t, 0},
      {"net", "log_outgoing", &g_settings.net.log_outgoing, field_type::bool_t, 0},
      {"net", "log_incoming", &g_settings.net.log_incoming, field_type::bool_t, 0},
      {"net", "capture_cmsg", &g_settings.net.capture_cmsg, field_type::bool_t, 0},
      {"net", "capture_stream", &g_settings.net.capture_stream, field_type::bool_t, 0},
      {"net", "block_outgoing", &g_settings.net.block_outgoing, field_type::bool_t, 0},
      {"net", "block_incoming", &g_settings.net.block_incoming, field_type::bool_t, 0},
      {"net", "block_opcode_mode", &g_settings.net.block_opcode_mode, field_type::int_t, 0},
      {"net", "block_opcode", &g_settings.net.block_opcode, field_type::hex16_t, 0},
      {"net", "block_opcode_list", g_settings.net.block_opcode_list, field_type::string_t, sizeof(g_settings.net.block_opcode_list)},
      {"net", "edit_outgoing", &g_settings.net.edit_outgoing, field_type::bool_t, 0},
      {"net", "edit_outgoing_opcode", &g_settings.net.edit_outgoing_opcode, field_type::hex16_t, 0},
      {"net", "edit_outgoing_apply_all", &g_settings.net.edit_outgoing_apply_all, field_type::bool_t, 0},
      {"net", "edit_incoming", &g_settings.net.edit_incoming, field_type::bool_t, 0},
      {"net", "edit_incoming_opcode", &g_settings.net.edit_incoming_opcode, field_type::hex16_t, 0},
      {"net", "edit_incoming_apply_all", &g_settings.net.edit_incoming_apply_all, field_type::bool_t, 0},
      {"net", "log_events", &g_settings.net.log_events, field_type::bool_t, 0},
      {"net", "log_to_file", &g_settings.net.log_to_file, field_type::bool_t, 0},
      {"net", "file_path", g_settings.net.file_path, field_type::string_t, sizeof(g_settings.net.file_path)},
      {"net", "max_entries", &g_settings.net.max_entries, field_type::int_t, 0},
      {"net", "show_parsed", &g_settings.net.show_parsed, field_type::bool_t, 0},
      {"net", "show_raw_hex", &g_settings.net.show_raw_hex, field_type::bool_t, 0},
      {"net", "filter_opcode", &g_settings.net.filter_opcode, field_type::int_t, 0},
      {"net", "filter_enabled", &g_settings.net.filter_enabled, field_type::bool_t, 0},
      {"net", "filter_direction", &g_settings.net.filter_direction, field_type::int_t, 0},

      {"interface", "hide_facebook", &g_settings.interface_hide.hide_facebook, field_type::bool_t, 0},
      {"interface", "hide_magic_lamp", &g_settings.interface_hide.hide_magic_lamp, field_type::bool_t, 0},
      {"interface", "hide_daily_login", &g_settings.interface_hide.hide_daily_login, field_type::bool_t, 0},
      {"interface", "hide_survey", &g_settings.interface_hide.hide_survey, field_type::bool_t, 0},
      {"interface", "hide_web_item_alarm", &g_settings.interface_hide.hide_web_item_alarm, field_type::bool_t, 0},
      {"interface", "hide_macro_guide", &g_settings.interface_hide.hide_macro_guide, field_type::bool_t, 0},
      {"interface", "apply_on_startup", &g_settings.interface_hide.apply_on_startup, field_type::bool_t, 0},

      {"target_window", "enabled", &g_settings.target_window.enabled, field_type::bool_t, 0},
      {"target_window", "show_hp_percent", &g_settings.target_window.show_hp_percent, field_type::bool_t, 0},

      {"widgets", "static_only", &g_settings.widgets.static_only, field_type::bool_t, 0},
      {"widgets", "auto_refresh", &g_settings.widgets.auto_refresh, field_type::bool_t, 0},
      {"widgets", "max_depth", &g_settings.widgets.max_depth, field_type::int_t, 0},
      {"widgets", "show_alarm_debug", &g_settings.widgets.show_alarm_debug, field_type::bool_t, 0},
      {"widgets", "deep_scan", &g_settings.widgets.deep_scan, field_type::bool_t, 0},
      {"widgets", "probe_max_id", &g_settings.widgets.probe_max_id, field_type::int_t, 0},

      {"interface_manager", "apply_on_startup", &g_settings.interface_manager.apply_on_startup, field_type::bool_t, 0},
    };

    auto apply_field(const field_desc& fd, const char* value) -> void {
      switch (fd.type) {
        case field_type::bool_t:
          *static_cast<bool*>(fd.ptr) = parse_bool(value, *static_cast<bool*>(fd.ptr));
          break;
        case field_type::int_t:
          *static_cast<int*>(fd.ptr) = parse_int(value, *static_cast<int*>(fd.ptr));
          break;
        case field_type::float_t:
          *static_cast<float*>(fd.ptr) = static_cast<float>(atof(value));
          break;
        case field_type::string_t:
          std::strncpy(static_cast<char*>(fd.ptr), value, fd.size - 1);
          static_cast<char*>(fd.ptr)[fd.size - 1] = '\0';
          break;
        case field_type::hex16_t:
          *static_cast<std::uint16_t*>(fd.ptr) = static_cast<std::uint16_t>(strtoul(value, nullptr, 16));
          break;
        case field_type::hex32_t:
          *static_cast<std::uint32_t*>(fd.ptr) = static_cast<std::uint32_t>(strtoul(value, nullptr, 16));
          break;
      }
    }

    auto save_field(std::ofstream& file, const field_desc& fd) -> void {
      switch (fd.type) {
        case field_type::bool_t:
          file << fd.key << "=" << (*static_cast<bool*>(fd.ptr) ? 1 : 0) << "\n";
          break;
        case field_type::int_t:
          file << fd.key << "=" << *static_cast<int*>(fd.ptr) << "\n";
          break;
        case field_type::float_t:
          file << fd.key << "=" << *static_cast<float*>(fd.ptr) << "\n";
          break;
        case field_type::string_t:
          file << fd.key << "=" << static_cast<char*>(fd.ptr) << "\n";
          break;
        case field_type::hex16_t:
          file << fd.key << "=0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase
               << *static_cast<std::uint16_t*>(fd.ptr) << "\n" << std::dec << std::nouppercase;
          break;
        case field_type::hex32_t:
          file << fd.key << "=0x" << std::setfill('0') << std::setw(8) << std::hex << std::uppercase
               << *static_cast<std::uint32_t*>(fd.ptr) << "\n" << std::dec << std::nouppercase;
          break;
      }
    }

    auto apply_key(const char* section, const char* key, const char* value) -> void {
      if (!section || !key || !value) {
        return;
      }

      for (const auto& fd : g_fields) {
        if (std::strcmp(fd.section, section) == 0 && std::strcmp(fd.key, key) == 0) {
          apply_field(fd, value);
          return;
        }
      }

      if (std::strcmp(section, "interface_manager") == 0 && std::strncmp(key, "hide_", 5) == 0) {
        const int index = atoi(key + 5);
        if (index >= 0 && index < core_config::interface_manager_t::max_hidden) {
          std::string_view sv(value);
          const auto first_colon = sv.find(':');
          if (first_colon != std::string_view::npos) {
            const auto second_colon = sv.find(':', first_colon + 1);
            if (second_colon != std::string_view::npos) {
              const auto type_str = sv.substr(0, first_colon);
              const auto key_str = sv.substr(first_colon + 1, second_colon - first_colon - 1);
              const auto label_str = sv.substr(second_colon + 1);

              auto& rule = g_settings.interface_manager.hidden[index];
              rule.ingame_map = (type_str == "ingame");
              rule.res_key = static_cast<int>(strtol(key_str.data(), nullptr, 16));
              std::strncpy(rule.label, label_str.data(), sizeof(rule.label) - 1);
              rule.label[sizeof(rule.label) - 1] = '\0';

              if (index >= g_settings.interface_manager.hidden_count) {
                g_settings.interface_manager.hidden_count = index + 1;
              }
            }
          }
        }
        return;
      }

      if (std::strcmp(section, "plugins") == 0) {
        ext_client::core::plugin::plugin_manager::get().set_plugin_enabled(key, parse_bool(value, true));
        return;
      }
    }

  } // namespace

  auto data() -> core_config& {
    return g_settings;
  }

  auto path() -> const char* {
    ensure_path();
    return g_path;
  }

  auto load() -> bool {
    ensure_path();

    std::ifstream file(g_path);
    if (!file.is_open()) {
      log_msg("[core_config] failed to open config: %s", g_path);
      return false;
    }

    std::string line;
    std::string current_section;

    while (std::getline(file, line)) {
      // trim spaces
      const auto start = line.find_first_not_of(" \t");
      if (start == std::string::npos || line[start] == ';' || line[start] == '#') {
        continue; // empty or comment
      }

      const auto end = line.find_last_not_of(" \t\r\n");
      std::string_view content(line.data() + start, end - start + 1);

      if (content.front() == '[' && content.back() == ']') {
        current_section = std::string(content.substr(1, content.size() - 2));
        continue;
      }

      const auto eq = content.find('=');
      if (eq != std::string_view::npos && !current_section.empty()) {
        std::string key(content.substr(0, eq));
        std::string value(content.substr(eq + 1));

        // trim key and value
        const auto k_end = key.find_last_not_of(" \t");
        if (k_end != std::string::npos) {
          key.erase(k_end + 1);
        }
        const auto v_start = value.find_first_not_of(" \t");
        if (v_start != std::string::npos) {
          value.erase(0, v_start);
        }

        apply_key(current_section.c_str(), key.c_str(), value.c_str());
      }
    }

    g_dirty = false;
    log_msg("[core_config] loaded %s", g_path);
    return true;
  }

  auto save() -> bool {
    ensure_path();

    std::ofstream file(g_path);
    if (!file.is_open()) {
      log_msg("[core_config] failed to save config: %s", g_path);
      return false;
    }

    const char* prev_section = nullptr;
    for (const auto& fd : g_fields) {
      if (!prev_section || std::strcmp(prev_section, fd.section) != 0) {
        if (prev_section) {
          file << "\n";
        }
        file << "[" << fd.section << "]\n";
        prev_section = fd.section;
      }
      save_field(file, fd);
    }

    {
      const auto& im = g_settings.interface_manager;
      for (int i = 0; i < im.hidden_count; ++i) {
        const auto& rule = im.hidden[i];
        if (rule.res_key == 0) {
          continue;
        }
        file << "hide_" << i << "=" << (rule.ingame_map ? "ingame:" : "iface:") << std::hex << std::showbase << rule.res_key
             << std::dec << ":" << rule.label << "\n";
      }
    }

    file << "\n[plugins]\n";
    for (const auto& plugin : ext_client::core::plugin::plugin_manager::get().get_plugins()) {
      file << plugin.id << "=" << (plugin.enabled ? 1 : 0) << "\n";
    }
    file << "\n";

    g_dirty = false;
    log_msg("[core_config] saved %s", g_path);
    return true;
  }

  auto sync_to_runtime() -> void {
    using namespace ext_client::core::event;
    ext_client::plugins::hud_customizer::apply_quick_hides();
    if (ext_client::core::config::data().interface_manager.apply_on_startup) {
      ext_client::plugins::hud_customizer::apply_saved_hides();
    }
    
    event_manager::get().dispatch(event_type::on_tick, nullptr);

    ext_client::render::render_system::get().apply_from_control();
  }

  auto mark_dirty() -> void {
    g_dirty = true;
    if (g_settings.general.save_on_change) {
      save();
    }
  }

  auto is_dirty() -> bool {
    return g_dirty;
  }

} // namespace ext_client::core::config
