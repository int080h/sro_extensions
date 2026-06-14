#include "config/client_config.hpp"

#include "core/ext_client.hpp"
#include "hooks/interface_hide_hook.hpp"
#include "hooks/title_hook.hpp"
#include "utils/log.hpp"
#include "utils/string.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <iomanip>

using ext_client::utils::log_msg;

#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ext_client::config {
  namespace {

    settings g_settings{};
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
      HMODULE h_mod = ext_client::core::get().get_module();
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

    auto apply_key(const char* section, const char* key, const char* value) -> void {
      if (!section || !key || !value) {
        return;
      }

      if (std::strcmp(section, "promo") == 0 || std::strcmp(section, "interface") == 0) {
        auto& iface = g_settings.interface_hide;
        if (std::strcmp(key, "hide_facebook") == 0) {
          iface.hide_facebook = parse_bool(value, iface.hide_facebook);
        } else if (std::strcmp(key, "hide_magic_lamp") == 0) {
          iface.hide_magic_lamp = parse_bool(value, iface.hide_magic_lamp);
        } else if (std::strcmp(key, "hide_daily_login") == 0) {
          iface.hide_daily_login = parse_bool(value, iface.hide_daily_login);
        } else if (std::strcmp(key, "hide_survey") == 0) {
          iface.hide_survey = parse_bool(value, iface.hide_survey);
        } else if (std::strcmp(key, "hide_web_item_alarm") == 0) {
          iface.hide_web_item_alarm = parse_bool(value, iface.hide_web_item_alarm);
        } else if (std::strcmp(key, "hide_macro_guide") == 0) {
          iface.hide_macro_guide = parse_bool(value, iface.hide_macro_guide);
        } else if (std::strcmp(key, "apply_on_startup") == 0) {
          iface.apply_on_startup = parse_bool(value, iface.apply_on_startup);
        }
        return;
      }

      if (std::strcmp(section, "widgets") == 0) {
        if (std::strcmp(key, "static_only") == 0) {
          g_settings.widgets.static_only = parse_bool(value, g_settings.widgets.static_only);
        } else if (std::strcmp(key, "auto_refresh") == 0) {
          g_settings.widgets.auto_refresh = parse_bool(value, g_settings.widgets.auto_refresh);
        } else if (std::strcmp(key, "max_depth") == 0) {
          g_settings.widgets.max_depth = parse_int(value, g_settings.widgets.max_depth);
        } else if (std::strcmp(key, "show_alarm_debug") == 0) {
          g_settings.widgets.show_alarm_debug = parse_bool(value, g_settings.widgets.show_alarm_debug);
        } else if (std::strcmp(key, "deep_scan") == 0) {
          g_settings.widgets.deep_scan = parse_bool(value, g_settings.widgets.deep_scan);
        } else if (std::strcmp(key, "probe_max_id") == 0) {
          g_settings.widgets.probe_max_id = parse_int(value, g_settings.widgets.probe_max_id);
        }
        return;
      }

      if (std::strcmp(section, "general") == 0) {
        if (std::strcmp(key, "save_on_change") == 0) {
          g_settings.general.save_on_change = parse_bool(value, g_settings.general.save_on_change);
        }
        return;
      }

      if (std::strcmp(section, "graphic") == 0) {
        auto& g = g_settings.graphic;
        if (std::strcmp(key, "custom_sight_range") == 0) {
          g.custom_sight_range = parse_bool(value, g.custom_sight_range);
        } else if (std::strcmp(key, "sight_range_level") == 0) {
          g.sight_range_level = parse_int(value, g.sight_range_level);
        } else if (std::strcmp(key, "sight_range_percent") == 0) {
          g.sight_range_percent = parse_int(value, g.sight_range_percent);
        } else if (std::strcmp(key, "d3d_force_hardware_vp") == 0) {
          g.d3d_force_hardware_vp = parse_bool(value, g.d3d_force_hardware_vp);
        } else if (std::strcmp(key, "d3d_force_pure_device") == 0) {
          g.d3d_force_pure_device = parse_bool(value, g.d3d_force_pure_device);
        } else if (std::strcmp(key, "d3d_triple_buffering") == 0) {
          g.d3d_triple_buffering = parse_bool(value, g.d3d_triple_buffering);
        } else if (std::strcmp(key, "d3d_discard_depth_stencil") == 0) {
          g.d3d_discard_depth_stencil = parse_bool(value, g.d3d_discard_depth_stencil);
        }
        return;
      }

      if (std::strcmp(section, "welcome_msg") == 0) {
        auto& wm = g_settings.welcome_msg;
        if (std::strcmp(key, "enabled") == 0) {
          wm.enabled = parse_bool(value, wm.enabled);
        } else if (std::strcmp(key, "text") == 0) {
          std::strncpy(wm.text, value, sizeof(wm.text) - 1);
          wm.text[sizeof(wm.text) - 1] = '\0';
        }
        return;
      }

      if (std::strcmp(section, "title") == 0) {
        if (std::strcmp(key, "enabled") == 0) {
          g_settings.title.enabled = parse_bool(value, g_settings.title.enabled);
        } else if (std::strcmp(key, "log_events") == 0) {
          g_settings.title.log_events = parse_bool(value, g_settings.title.log_events);
        } else if (std::strcmp(key, "skip_on_create") == 0) {
          g_settings.title.skip_on_create = parse_bool(value, g_settings.title.skip_on_create);
        } else if (std::strcmp(key, "auto_login_on_create") == 0) {
          g_settings.title.auto_login_on_create = parse_bool(value, g_settings.title.auto_login_on_create);
        } else if (std::strcmp(key, "block_login_packets") == 0) {
          g_settings.title.block_login_packets = parse_bool(value, g_settings.title.block_login_packets);
        } else if (std::strcmp(key, "block_process_transition") == 0) {
          g_settings.title.block_process_transition = parse_bool(value, g_settings.title.block_process_transition);
        } else if (std::strcmp(key, "suppress_captcha") == 0) {
          g_settings.title.suppress_captcha = parse_bool(value, g_settings.title.suppress_captcha);
        } else if (std::strcmp(key, "hide_channel_list_button") == 0) {
          g_settings.title.hide_channel_list_button = parse_bool(value, g_settings.title.hide_channel_list_button);
        } else if (std::strcmp(key, "replace_login_frame") == 0) {
          g_settings.title.replace_login_frame = parse_bool(value, g_settings.title.replace_login_frame);
        } else if (std::strcmp(key, "login_frame_path") == 0) {
          std::strncpy(g_settings.title.login_frame_path, value, sizeof(g_settings.title.login_frame_path) - 1);
          g_settings.title.login_frame_path[sizeof(g_settings.title.login_frame_path) - 1] = '\0';
        } else if (std::strcmp(key, "eu_login_id_label_x_adjust") == 0) {
          g_settings.title.eu_login_id_label_adjust.x = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_id_label_adjust.x)));
        } else if (std::strcmp(key, "eu_login_id_label_y_adjust") == 0) {
          g_settings.title.eu_login_id_label_adjust.y = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_id_label_adjust.y)));
        } else if (std::strcmp(key, "eu_login_id_input_x_adjust") == 0) {
          g_settings.title.eu_login_id_input_adjust.x = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_id_input_adjust.x)));
        } else if (std::strcmp(key, "eu_login_id_input_y_adjust") == 0) {
          g_settings.title.eu_login_id_input_adjust.y = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_id_input_adjust.y)));
        } else if (std::strcmp(key, "eu_login_pw_label_x_adjust") == 0) {
          g_settings.title.eu_login_pw_label_adjust.x = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_pw_label_adjust.x)));
        } else if (std::strcmp(key, "eu_login_pw_label_y_adjust") == 0) {
          g_settings.title.eu_login_pw_label_adjust.y = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_pw_label_adjust.y)));
        } else if (std::strcmp(key, "eu_login_pw_input_x_adjust") == 0) {
          g_settings.title.eu_login_pw_input_adjust.x = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_pw_input_adjust.x)));
        } else if (std::strcmp(key, "eu_login_pw_input_y_adjust") == 0) {
          g_settings.title.eu_login_pw_input_adjust.y = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_pw_input_adjust.y)));
        } else if (std::strcmp(key, "eu_login_server_label_x_adjust") == 0) {
          g_settings.title.eu_login_server_label_adjust.x = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_server_label_adjust.x)));
        } else if (std::strcmp(key, "eu_login_server_label_y_adjust") == 0) {
          g_settings.title.eu_login_server_label_adjust.y = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_server_label_adjust.y)));
        } else if (std::strcmp(key, "eu_login_server_value_x_adjust") == 0) {
          g_settings.title.eu_login_server_value_adjust.x = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_server_value_adjust.x)));
        } else if (std::strcmp(key, "eu_login_server_value_y_adjust") == 0) {
          g_settings.title.eu_login_server_value_adjust.y = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_server_value_adjust.y)));
        } else if (std::strcmp(key, "eu_login_server_button_x_adjust") == 0) {
          g_settings.title.eu_login_server_button_adjust.x = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_server_button_adjust.x)));
        } else if (std::strcmp(key, "eu_login_server_button_y_adjust") == 0) {
          g_settings.title.eu_login_server_button_adjust.y = static_cast<float>(parse_int(value, static_cast<int>(g_settings.title.eu_login_server_button_adjust.y)));
        } else if (std::strcmp(key, "logo_y_offset") == 0) {
          g_settings.title.logo_y_offset = parse_int(value, g_settings.title.logo_y_offset);
          if (g_settings.title.logo_y_offset < 0) {
            g_settings.title.logo_y_offset = 0;
          }
        } else if (std::strcmp(key, "override_version_labels") == 0) {
          g_settings.title.override_version_labels = parse_bool(value, g_settings.title.override_version_labels);
        } else if (std::strcmp(key, "data_version_fmt") == 0) {
          std::strncpy(g_settings.title.data_version_fmt, value, sizeof(g_settings.title.data_version_fmt) - 1);
          g_settings.title.data_version_fmt[sizeof(g_settings.title.data_version_fmt) - 1] = '\0';
        } else if (std::strcmp(key, "exe_version_fmt") == 0) {
          std::strncpy(g_settings.title.exe_version_fmt, value, sizeof(g_settings.title.exe_version_fmt) - 1);
          g_settings.title.exe_version_fmt[sizeof(g_settings.title.exe_version_fmt) - 1] = '\0';
        } else if (std::strcmp(key, "override_version_label_color") == 0) {
          g_settings.title.override_version_label_color = parse_bool(value, g_settings.title.override_version_label_color);
        } else if (std::strcmp(key, "version_label_color") == 0) {
          g_settings.title.version_label_color = static_cast<std::uint32_t>(std::strtoul(value, nullptr, 16));
        } else if (std::strcmp(key, "version_labels_clip") == 0) {
          g_settings.title.version_labels_clip = parse_bool(value, g_settings.title.version_labels_clip);
        } else if (std::strcmp(key, "version_label_ellipsis_width") == 0) {
          g_settings.title.version_label_ellipsis_width = parse_int(value, g_settings.title.version_label_ellipsis_width);
        }
        return;
      }

      if (std::strcmp(section, "version_check") == 0) {
        if (std::strcmp(key, "enabled") == 0) {
          g_settings.version_check.enabled = parse_bool(value, g_settings.version_check.enabled);
        } else if (std::strcmp(key, "log_events") == 0) {
          g_settings.version_check.log_events = parse_bool(value, g_settings.version_check.log_events);
        } else if (std::strcmp(key, "bypass_gateway_connect") == 0) {
          g_settings.version_check.bypass_gateway_connect = parse_bool(value, g_settings.version_check.bypass_gateway_connect);
        } else if (std::strcmp(key, "force_version_result") == 0) {
          g_settings.version_check.force_version_result = parse_bool(value, g_settings.version_check.force_version_result);
        } else if (std::strcmp(key, "forced_version_result") == 0) {
          g_settings.version_check.forced_version_result = parse_int(value, g_settings.version_check.forced_version_result);
        } else if (std::strcmp(key, "skip_textdata_load") == 0) {
          g_settings.version_check.skip_textdata_load = parse_bool(value, g_settings.version_check.skip_textdata_load);
        } else if (std::strcmp(key, "block_process_transition") == 0) {
          g_settings.version_check.block_process_transition = parse_bool(value, g_settings.version_check.block_process_transition);
        } else if (std::strcmp(key, "skip_loading_splash") == 0) {
          g_settings.version_check.skip_loading_splash = parse_bool(value, g_settings.version_check.skip_loading_splash);
        } else if (std::strcmp(key, "banner_custom_size") == 0) {
          g_settings.version_check.banner_custom_size = parse_bool(value, g_settings.version_check.banner_custom_size);
        } else if (std::strcmp(key, "banner_width") == 0) {
          g_settings.version_check.banner_width = parse_int(value, g_settings.version_check.banner_width);
        } else if (std::strcmp(key, "banner_height") == 0) {
          g_settings.version_check.banner_height = parse_int(value, g_settings.version_check.banner_height);
        } else if (std::strcmp(key, "banner_center") == 0) {
          g_settings.version_check.banner_center = parse_bool(value, g_settings.version_check.banner_center);
        } else if (std::strcmp(key, "banner_x") == 0) {
          g_settings.version_check.banner_x = parse_int(value, g_settings.version_check.banner_x);
        } else if (std::strcmp(key, "banner_y") == 0) {
          g_settings.version_check.banner_y = parse_int(value, g_settings.version_check.banner_y);
        } else if (std::strcmp(key, "banner_cycle") == 0) {
          g_settings.version_check.banner_cycle = parse_bool(value, g_settings.version_check.banner_cycle);
        } else if (std::strcmp(key, "banner_cycle_interval_ms") == 0) {
          g_settings.version_check.banner_cycle_interval_ms = parse_int(value, g_settings.version_check.banner_cycle_interval_ms);
        } else if (std::strcmp(key, "banner_count") == 0) {
          g_settings.version_check.banner_count = parse_int(value, g_settings.version_check.banner_count);
        } else if (std::strcmp(key, "banner_path_fmt") == 0) {
          std::strncpy(g_settings.version_check.banner_path_fmt, value, sizeof(g_settings.version_check.banner_path_fmt) - 1);
          g_settings.version_check.banner_path_fmt[sizeof(g_settings.version_check.banner_path_fmt) - 1] = '\0';
        } else if (std::strcmp(key, "banner_overlay") == 0) {
          g_settings.version_check.banner_overlay = parse_bool(value, g_settings.version_check.banner_overlay);
        }
        return;
      }

      if (std::strcmp(section, "packet") == 0) {
        auto& p = g_settings.packet;
        if (std::strcmp(key, "enabled") == 0) {
          p.enabled = parse_bool(value, p.enabled);
        } else if (std::strcmp(key, "log_events") == 0) {
          p.log_events = parse_bool(value, p.log_events);
        } else if (std::strcmp(key, "edit_outgoing") == 0) {
          p.edit_outgoing = parse_bool(value, p.edit_outgoing);
        } else if (std::strcmp(key, "edit_outgoing_opcode") == 0) {
          p.edit_outgoing_opcode = static_cast<std::uint16_t>(std::strtoul(value, nullptr, 16));
        } else if (std::strcmp(key, "edit_outgoing_apply_all") == 0) {
          p.edit_outgoing_apply_all = parse_bool(value, p.edit_outgoing_apply_all);
        } else if (std::strcmp(key, "edit_incoming") == 0) {
          p.edit_incoming = parse_bool(value, p.edit_incoming);
        } else if (std::strcmp(key, "edit_incoming_opcode") == 0) {
          p.edit_incoming_opcode = static_cast<std::uint16_t>(std::strtoul(value, nullptr, 16));
        } else if (std::strcmp(key, "edit_incoming_apply_all") == 0) {
          p.edit_incoming_apply_all = parse_bool(value, p.edit_incoming_apply_all);
        }
        return;
      }

      if (std::strcmp(section, "net_manager") == 0) {
        auto& nm = g_settings.net_manager;
        if (std::strcmp(key, "enabled") == 0) {
          nm.enabled = parse_bool(value, nm.enabled);
        } else if (std::strcmp(key, "log_outgoing") == 0) {
          nm.log_outgoing = parse_bool(value, nm.log_outgoing);
        } else if (std::strcmp(key, "log_incoming") == 0) {
          nm.log_incoming = parse_bool(value, nm.log_incoming);
        } else if (std::strcmp(key, "capture_cmsg") == 0) {
          nm.capture_cmsg = parse_bool(value, nm.capture_cmsg);
        } else if (std::strcmp(key, "capture_stream") == 0) {
          nm.capture_stream = parse_bool(value, nm.capture_stream);
        } else if (std::strcmp(key, "log_to_file") == 0) {
          nm.log_to_file = parse_bool(value, nm.log_to_file);
        } else if (std::strcmp(key, "pause_capture") == 0) {
          nm.pause_capture = parse_bool(value, nm.pause_capture);
        } else if (std::strcmp(key, "block_outgoing") == 0) {
          nm.block_outgoing = parse_bool(value, nm.block_outgoing);
        } else if (std::strcmp(key, "block_incoming") == 0) {
          nm.block_incoming = parse_bool(value, nm.block_incoming);
        } else if (std::strcmp(key, "block_opcode_mode") == 0) {
          nm.block_opcode_mode = parse_int(value, nm.block_opcode_mode);
        } else if (std::strcmp(key, "block_opcode") == 0) {
          nm.block_opcode = static_cast<std::uint16_t>(std::strtoul(value, nullptr, 16));
        } else if (std::strcmp(key, "block_opcode_list") == 0) {
          std::strncpy(nm.block_opcode_list, value, sizeof(nm.block_opcode_list) - 1);
          nm.block_opcode_list[sizeof(nm.block_opcode_list) - 1] = '\0';
        } else if (std::strcmp(key, "max_entries") == 0) {
          nm.max_entries = parse_int(value, nm.max_entries);
        } else if (std::strcmp(key, "file_path") == 0) {
          std::strncpy(nm.file_path, value, sizeof(nm.file_path) - 1);
          nm.file_path[sizeof(nm.file_path) - 1] = '\0';
        }
        return;
      }
    }

  } // namespace

  auto data() -> settings& {
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
      log_msg("[client_config] no config at %s (using defaults)", g_path);
      return false;
    }

    std::string line;
    std::string section = "general";
    int loaded = 0;

    while (std::getline(file, line)) {
      std::string_view trimmed_line = ext_client::utils::string::trim(line);
      if (trimmed_line.empty() || trimmed_line[0] == ';' || trimmed_line[0] == '#') {
        continue;
      }
      if (trimmed_line[0] == '[') {
        const auto end = trimmed_line.find(']');
        if (end == std::string_view::npos) {
          continue;
        }
        section = std::string(trimmed_line.substr(1, end - 1));
        continue;
      }

      const auto eq = trimmed_line.find('=');
      if (eq == std::string_view::npos) {
        continue;
      }

      std::string key = std::string(ext_client::utils::string::trim(trimmed_line.substr(0, eq)));
      std::string value = std::string(ext_client::utils::string::trim(trimmed_line.substr(eq + 1)));

      apply_key(section.c_str(), key.c_str(), value.c_str());
      ++loaded;
    }

    g_dirty = false;
    log_msg("[client_config] loaded %d keys from %s", loaded, g_path);
    return loaded > 0;
  }

  auto save() -> bool {
    ensure_path();

    std::ofstream file(g_path);
    if (!file.is_open()) {
      log_msg("[client_config] failed to write %s", g_path);
      return false;
    }

    const auto& s = g_settings;
    file << "; ext_client settings\n\n";

    file << "[interface]\n";
    file << "hide_facebook=" << (s.interface_hide.hide_facebook ? 1 : 0) << "\n";
    file << "hide_magic_lamp=" << (s.interface_hide.hide_magic_lamp ? 1 : 0) << "\n";
    file << "hide_daily_login=" << (s.interface_hide.hide_daily_login ? 1 : 0) << "\n";
    file << "hide_survey=" << (s.interface_hide.hide_survey ? 1 : 0) << "\n";
    file << "hide_web_item_alarm=" << (s.interface_hide.hide_web_item_alarm ? 1 : 0) << "\n";
    file << "hide_macro_guide=" << (s.interface_hide.hide_macro_guide ? 1 : 0) << "\n";
    file << "apply_on_startup=" << (s.interface_hide.apply_on_startup ? 1 : 0) << "\n\n";

    file << "[widgets]\n";
    file << "static_only=" << (s.widgets.static_only ? 1 : 0) << "\n";
    file << "auto_refresh=" << (s.widgets.auto_refresh ? 1 : 0) << "\n";
    file << "max_depth=" << s.widgets.max_depth << "\n";
    file << "show_alarm_debug=" << (s.widgets.show_alarm_debug ? 1 : 0) << "\n";
    file << "deep_scan=" << (s.widgets.deep_scan ? 1 : 0) << "\n";
    file << "probe_max_id=" << s.widgets.probe_max_id << "\n\n";

    file << "[general]\n";
    file << "save_on_change=" << (s.general.save_on_change ? 1 : 0) << "\n\n";

    file << "[graphic]\n";
    file << "custom_sight_range=" << (s.graphic.custom_sight_range ? 1 : 0) << "\n";
    file << "sight_range_level=" << s.graphic.sight_range_level << "\n";
    file << "sight_range_percent=" << s.graphic.sight_range_percent << "\n";
    file << "d3d_force_hardware_vp=" << (s.graphic.d3d_force_hardware_vp ? 1 : 0) << "\n";
    file << "d3d_force_pure_device=" << (s.graphic.d3d_force_pure_device ? 1 : 0) << "\n";
    file << "d3d_triple_buffering=" << (s.graphic.d3d_triple_buffering ? 1 : 0) << "\n";
    file << "d3d_discard_depth_stencil=" << (s.graphic.d3d_discard_depth_stencil ? 1 : 0) << "\n\n";

    file << "[welcome_msg]\n";
    file << "enabled=" << (s.welcome_msg.enabled ? 1 : 0) << "\n";
    file << "text=" << s.welcome_msg.text << "\n\n";

    file << "[title]\n";
    file << "enabled=" << (s.title.enabled ? 1 : 0) << "\n";
    file << "log_events=" << (s.title.log_events ? 1 : 0) << "\n";
    file << "skip_on_create=" << (s.title.skip_on_create ? 1 : 0) << "\n";
    file << "auto_login_on_create=" << (s.title.auto_login_on_create ? 1 : 0) << "\n";
    file << "block_login_packets=" << (s.title.block_login_packets ? 1 : 0) << "\n";
    file << "block_process_transition=" << (s.title.block_process_transition ? 1 : 0) << "\n";
    file << "suppress_captcha=" << (s.title.suppress_captcha ? 1 : 0) << "\n";
    file << "hide_channel_list_button=" << (s.title.hide_channel_list_button ? 1 : 0) << "\n";
    file << "replace_login_frame=" << (s.title.replace_login_frame ? 1 : 0) << "\n";
    file << "login_frame_path=" << s.title.login_frame_path << "\n";
    file << "eu_login_id_label_x_adjust=" << static_cast<int>(s.title.eu_login_id_label_adjust.x) << "\n";
    file << "eu_login_id_label_y_adjust=" << static_cast<int>(s.title.eu_login_id_label_adjust.y) << "\n";
    file << "eu_login_id_input_x_adjust=" << static_cast<int>(s.title.eu_login_id_input_adjust.x) << "\n";
    file << "eu_login_id_input_y_adjust=" << static_cast<int>(s.title.eu_login_id_input_adjust.y) << "\n";
    file << "eu_login_pw_label_x_adjust=" << static_cast<int>(s.title.eu_login_pw_label_adjust.x) << "\n";
    file << "eu_login_pw_label_y_adjust=" << static_cast<int>(s.title.eu_login_pw_label_adjust.y) << "\n";
    file << "eu_login_pw_input_x_adjust=" << static_cast<int>(s.title.eu_login_pw_input_adjust.x) << "\n";
    file << "eu_login_pw_input_y_adjust=" << static_cast<int>(s.title.eu_login_pw_input_adjust.y) << "\n";
    file << "eu_login_server_label_x_adjust=" << static_cast<int>(s.title.eu_login_server_label_adjust.x) << "\n";
    file << "eu_login_server_label_y_adjust=" << static_cast<int>(s.title.eu_login_server_label_adjust.y) << "\n";
    file << "eu_login_server_value_x_adjust=" << static_cast<int>(s.title.eu_login_server_value_adjust.x) << "\n";
    file << "eu_login_server_value_y_adjust=" << static_cast<int>(s.title.eu_login_server_value_adjust.y) << "\n";
    file << "eu_login_server_button_x_adjust=" << static_cast<int>(s.title.eu_login_server_button_adjust.x) << "\n";
    file << "eu_login_server_button_y_adjust=" << static_cast<int>(s.title.eu_login_server_button_adjust.y) << "\n";
    file << "logo_y_offset=" << s.title.logo_y_offset << "\n";
    file << "override_version_labels=" << (s.title.override_version_labels ? 1 : 0) << "\n";
    file << "data_version_fmt=" << s.title.data_version_fmt << "\n";
    file << "exe_version_fmt=" << s.title.exe_version_fmt << "\n";
    file << "override_version_label_color=" << (s.title.override_version_label_color ? 1 : 0) << "\n";
    file << "version_label_color=0x" << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << s.title.version_label_color
         << "\n";
    file << std::dec << std::nouppercase;
    file << "version_labels_clip=" << (s.title.version_labels_clip ? 1 : 0) << "\n";
    file << "version_label_ellipsis_width=" << s.title.version_label_ellipsis_width << "\n\n";

    file << "[version_check]\n";
    file << "enabled=" << (s.version_check.enabled ? 1 : 0) << "\n";
    file << "log_events=" << (s.version_check.log_events ? 1 : 0) << "\n";
    file << "bypass_gateway_connect=" << (s.version_check.bypass_gateway_connect ? 1 : 0) << "\n";
    file << "force_version_result=" << (s.version_check.force_version_result ? 1 : 0) << "\n";
    file << "forced_version_result=" << s.version_check.forced_version_result << "\n";
    file << "skip_textdata_load=" << (s.version_check.skip_textdata_load ? 1 : 0) << "\n";
    file << "block_process_transition=" << (s.version_check.block_process_transition ? 1 : 0) << "\n";
    file << "skip_loading_splash=" << (s.version_check.skip_loading_splash ? 1 : 0) << "\n";
    file << "banner_custom_size=" << (s.version_check.banner_custom_size ? 1 : 0) << "\n";
    file << "banner_width=" << s.version_check.banner_width << "\n";
    file << "banner_height=" << s.version_check.banner_height << "\n";
    file << "banner_center=" << (s.version_check.banner_center ? 1 : 0) << "\n";
    file << "banner_x=" << s.version_check.banner_x << "\n";
    file << "banner_y=" << s.version_check.banner_y << "\n";
    file << "banner_cycle=" << (s.version_check.banner_cycle ? 1 : 0) << "\n";
    file << "banner_cycle_interval_ms=" << s.version_check.banner_cycle_interval_ms << "\n";
    file << "banner_count=" << s.version_check.banner_count << "\n";
    file << "banner_path_fmt=" << s.version_check.banner_path_fmt << "\n";
    file << "banner_overlay=" << (s.version_check.banner_overlay ? 1 : 0) << "\n\n";

    file << "[packet]\n";
    file << "enabled=" << (s.packet.enabled ? 1 : 0) << "\n";
    file << "log_events=" << (s.packet.log_events ? 1 : 0) << "\n";
    file << "edit_outgoing=" << (s.packet.edit_outgoing ? 1 : 0) << "\n";
    file << "edit_outgoing_opcode=0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << s.packet.edit_outgoing_opcode
         << "\n";
    file << std::dec << std::nouppercase;
    file << "edit_outgoing_apply_all=" << (s.packet.edit_outgoing_apply_all ? 1 : 0) << "\n";
    file << "edit_incoming=" << (s.packet.edit_incoming ? 1 : 0) << "\n";
    file << "edit_incoming_opcode=0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << s.packet.edit_incoming_opcode
         << "\n";
    file << std::dec << std::nouppercase;
    file << "edit_incoming_apply_all=" << (s.packet.edit_incoming_apply_all ? 1 : 0) << "\n\n";

    file << "[net_manager]\n";
    file << "enabled=" << (s.net_manager.enabled ? 1 : 0) << "\n";
    file << "log_outgoing=" << (s.net_manager.log_outgoing ? 1 : 0) << "\n";
    file << "log_incoming=" << (s.net_manager.log_incoming ? 1 : 0) << "\n";
    file << "capture_cmsg=" << (s.net_manager.capture_cmsg ? 1 : 0) << "\n";
    file << "capture_stream=" << (s.net_manager.capture_stream ? 1 : 0) << "\n";
    file << "log_to_file=" << (s.net_manager.log_to_file ? 1 : 0) << "\n";
    file << "pause_capture=" << (s.net_manager.pause_capture ? 1 : 0) << "\n";
    file << "block_outgoing=" << (s.net_manager.block_outgoing ? 1 : 0) << "\n";
    file << "block_incoming=" << (s.net_manager.block_incoming ? 1 : 0) << "\n";
    file << "block_opcode_mode=" << s.net_manager.block_opcode_mode << "\n";
    file << "block_opcode=0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << s.net_manager.block_opcode << "\n";
    file << std::dec << std::nouppercase;
    file << "block_opcode_list=" << s.net_manager.block_opcode_list << "\n";
    file << "max_entries=" << s.net_manager.max_entries << "\n";
    file << "file_path=" << s.net_manager.file_path << "\n\n";

    g_dirty = false;
    log_msg("[client_config] saved %s", g_path);
    return true;
  }

  auto sync_to_runtime() -> void {
    ext_client::hooks::interface_hide::apply_from_control();
    ext_client::hooks::title::apply_from_control();
  }

  auto mark_dirty() -> void {
    g_dirty = true;
    if (g_settings.general.save_on_change) {
      save();
    }
  }

} // namespace ext_client::config
