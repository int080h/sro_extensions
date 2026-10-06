#include "pch.hpp"
#include "core/config.hpp"

#include "core/app.hpp"
#include "core/event_bus.hpp"
#include "render/render_system.hpp"
#include "core/plugin_manager.hpp"
#include "utils/log.hpp"
#include "utils/string.hpp"

#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <string>
#include <string_view>

using ext_client::utils::log_msg;

namespace ext_client::core::config {
  namespace {

    core_config g_settings{};
    char g_path[MAX_PATH]{};
    std::atomic<bool> g_dirty{false};
    std::atomic<std::uint64_t> g_revision{1};
    std::atomic<DWORD> g_last_change{0};
    std::uint64_t g_published_revision = 0;
    std::atomic<std::shared_ptr<const core_config>> g_runtime{std::make_shared<const core_config>()};
    std::mutex g_save_mutex;
    bool g_path_ready = false;

    auto parse_bool(const char *value, bool fallback) -> bool {
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

    auto parse_int(const char *value, int fallback) -> int {
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
        char *slash = std::strrchr(g_path, '\\');
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
      const char *section;
      const char *key;
      void *ptr;
      field_type type;
      std::size_t size;
    };

#define FIELD_BOOL(section, key, member) {section, key, &member, field_type::bool_t, 0}
#define FIELD_INT(section, key, member) {section, key, &member, field_type::int_t, 0}
#define FIELD_FLOAT(section, key, member) {section, key, &member, field_type::float_t, 0}
#define FIELD_STRING(section, key, member) {section, key, member, field_type::string_t, sizeof(member)}
#define FIELD_HEX16(section, key, member) {section, key, &member, field_type::hex16_t, 0}
#define FIELD_HEX32(section, key, member) {section, key, &member, field_type::hex32_t, 0}
#define FIELD_FLOAT2(section, prefix, member)                                                                          \
  {section, prefix "_x", &member.x, field_type::float_t, 0}, {section, prefix "_y", &member.y, field_type::float_t, 0}

    static const field_desc g_fields[] = {
        FIELD_BOOL("general", "save_on_change", g_settings.general.save_on_change),

        FIELD_BOOL("graphic", "d3d_force_hardware_vp", g_settings.graphic.d3d_force_hardware_vp),
        FIELD_BOOL("graphic", "d3d_force_pure_device", g_settings.graphic.d3d_force_pure_device),
        FIELD_BOOL("graphic", "d3d_triple_buffering", g_settings.graphic.d3d_triple_buffering),
        FIELD_BOOL("graphic", "d3d_discard_depth_stencil", g_settings.graphic.d3d_discard_depth_stencil),

        FIELD_BOOL("welcome_msg", "enabled", g_settings.welcome_msg.enabled),
        FIELD_BOOL("welcome_msg", "hide", g_settings.welcome_msg.hide),
        FIELD_STRING("welcome_msg", "text", g_settings.welcome_msg.text),

        FIELD_BOOL("title", "enabled", g_settings.title.enabled),
        FIELD_BOOL("title", "log_events", g_settings.title.log_events),
        FIELD_BOOL("title", "hide_channel_list_button", g_settings.title.hide_channel_list_button),
        FIELD_BOOL("title", "replace_login_frame", g_settings.title.replace_login_frame),
        FIELD_STRING("title", "login_frame_path", g_settings.title.login_frame_path),
        FIELD_FLOAT2("title", "eu_login_id_label_adjust", g_settings.title.eu_login_id_label_adjust),
        FIELD_FLOAT2("title", "eu_login_id_input_adjust", g_settings.title.eu_login_id_input_adjust),
        FIELD_FLOAT2("title", "eu_login_pw_label_adjust", g_settings.title.eu_login_pw_label_adjust),
        FIELD_FLOAT2("title", "eu_login_pw_input_adjust", g_settings.title.eu_login_pw_input_adjust),
        FIELD_FLOAT2("title", "eu_login_server_label_adjust", g_settings.title.eu_login_server_label_adjust),
        FIELD_FLOAT2("title", "eu_login_server_value_adjust", g_settings.title.eu_login_server_value_adjust),
        FIELD_FLOAT2("title", "eu_login_server_button_adjust", g_settings.title.eu_login_server_button_adjust),
        FIELD_INT("title", "logo_y_offset", g_settings.title.logo_y_offset),
        FIELD_BOOL("title", "override_version_labels", g_settings.title.override_version_labels),
        FIELD_BOOL("title", "override_version_label_color", g_settings.title.override_version_label_color),
        FIELD_BOOL("title", "version_labels_clip", g_settings.title.version_labels_clip),
        FIELD_STRING("title", "data_version_fmt", g_settings.title.data_version_fmt),
        FIELD_STRING("title", "exe_version_fmt", g_settings.title.exe_version_fmt),
        FIELD_HEX32("title", "version_label_color", g_settings.title.version_label_color),
        FIELD_INT("title", "version_label_ellipsis_width", g_settings.title.version_label_ellipsis_width),

        FIELD_BOOL("version_check", "enabled", g_settings.version_check.enabled),
        FIELD_BOOL("version_check", "log_events", g_settings.version_check.log_events),
        FIELD_BOOL("version_check", "banner_custom_size", g_settings.version_check.banner_custom_size),
        FIELD_INT("version_check", "banner_width", g_settings.version_check.banner_width),
        FIELD_INT("version_check", "banner_height", g_settings.version_check.banner_height),
        FIELD_BOOL("version_check", "banner_center", g_settings.version_check.banner_center),
        FIELD_INT("version_check", "banner_x", g_settings.version_check.banner_x),
        FIELD_INT("version_check", "banner_y", g_settings.version_check.banner_y),
        FIELD_BOOL("version_check", "banner_cycle", g_settings.version_check.banner_cycle),
        FIELD_INT("version_check", "banner_cycle_interval_ms", g_settings.version_check.banner_cycle_interval_ms),
        FIELD_INT("version_check", "banner_count", g_settings.version_check.banner_count),
        FIELD_STRING("version_check", "banner_path_fmt", g_settings.version_check.banner_path_fmt),
        FIELD_BOOL("version_check", "banner_overlay", g_settings.version_check.banner_overlay),
        FIELD_BOOL("version_check", "ensure_minimize_button", g_settings.version_check.ensure_minimize_button),

        FIELD_BOOL("net", "enabled", g_settings.net.enabled),
        FIELD_BOOL("net", "pause_capture", g_settings.net.pause_capture),
        FIELD_BOOL("net", "log_outgoing", g_settings.net.log_outgoing),
        FIELD_BOOL("net", "log_incoming", g_settings.net.log_incoming),
        FIELD_BOOL("net", "capture_cmsg", g_settings.net.capture_cmsg),
        FIELD_BOOL("net", "capture_stream", g_settings.net.capture_stream),
        FIELD_BOOL("net", "block_outgoing", g_settings.net.block_outgoing),
        FIELD_BOOL("net", "block_incoming", g_settings.net.block_incoming),
        FIELD_INT("net", "block_opcode_mode", g_settings.net.block_opcode_mode),
        FIELD_HEX16("net", "block_opcode", g_settings.net.block_opcode),
        FIELD_STRING("net", "block_opcode_list", g_settings.net.block_opcode_list),
        FIELD_BOOL("net", "edit_outgoing", g_settings.net.edit_outgoing),
        FIELD_HEX16("net", "edit_outgoing_opcode", g_settings.net.edit_outgoing_opcode),
        FIELD_BOOL("net", "edit_outgoing_apply_all", g_settings.net.edit_outgoing_apply_all),
        FIELD_BOOL("net", "edit_incoming", g_settings.net.edit_incoming),
        FIELD_HEX16("net", "edit_incoming_opcode", g_settings.net.edit_incoming_opcode),
        FIELD_BOOL("net", "edit_incoming_apply_all", g_settings.net.edit_incoming_apply_all),
        FIELD_BOOL("net", "log_events", g_settings.net.log_events),
        FIELD_BOOL("net", "log_to_file", g_settings.net.log_to_file),
        FIELD_STRING("net", "file_path", g_settings.net.file_path),
        FIELD_INT("net", "max_entries", g_settings.net.max_entries),
        FIELD_BOOL("net", "show_parsed", g_settings.net.show_parsed),
        FIELD_BOOL("net", "show_raw_hex", g_settings.net.show_raw_hex),
        FIELD_INT("net", "filter_opcode", g_settings.net.filter_opcode),
        FIELD_BOOL("net", "filter_enabled", g_settings.net.filter_enabled),
        FIELD_INT("net", "filter_direction", g_settings.net.filter_direction),
        FIELD_INT("net", "filter_opcode_mode", g_settings.net.filter_opcode_mode),
        FIELD_STRING("net", "filter_opcode_list", g_settings.net.filter_opcode_list),
        FIELD_STRING("net", "filter_opcode_name", g_settings.net.filter_opcode_name),
        FIELD_INT("net", "filter_category", g_settings.net.filter_category),
        FIELD_STRING("net", "filter_search_text", g_settings.net.filter_search_text),
        FIELD_INT("net", "filter_layer", g_settings.net.filter_layer),
        FIELD_BOOL("net", "filter_massive_only", g_settings.net.filter_massive_only),
        FIELD_BOOL("net", "filter_blocked_only", g_settings.net.filter_blocked_only),
        FIELD_BOOL("net", "filter_modified_only", g_settings.net.filter_modified_only),
        FIELD_INT("net", "min_payload_size", g_settings.net.min_payload_size),
        FIELD_INT("net", "max_payload_size", g_settings.net.max_payload_size),
        FIELD_BOOL("net", "auto_scroll", g_settings.net.auto_scroll),
        FIELD_BOOL("net", "show_timestamps", g_settings.net.show_timestamps),
        FIELD_BOOL("net", "show_stats_bar", g_settings.net.show_stats_bar),
        FIELD_INT("net", "sort_column", g_settings.net.sort_column),
        FIELD_BOOL("net", "sort_ascending", g_settings.net.sort_ascending),
        FIELD_STRING("net", "session_path", g_settings.net.session_path),

        FIELD_BOOL("interface", "hide_facebook", g_settings.interface_hide.hide_facebook),
        FIELD_BOOL("interface", "hide_magic_lamp", g_settings.interface_hide.hide_magic_lamp),
        FIELD_BOOL("interface", "hide_daily_login", g_settings.interface_hide.hide_daily_login),
        FIELD_BOOL("interface", "hide_survey", g_settings.interface_hide.hide_survey),
        FIELD_BOOL("interface", "hide_web_item_alarm", g_settings.interface_hide.hide_web_item_alarm),
        FIELD_BOOL("interface", "hide_macro_guide", g_settings.interface_hide.hide_macro_guide),
        FIELD_BOOL("interface", "apply_on_startup", g_settings.interface_hide.apply_on_startup),

        FIELD_BOOL("target_window", "enabled", g_settings.target_window.enabled),
        FIELD_BOOL("target_window", "show_hp_percent", g_settings.target_window.show_hp_percent),
        FIELD_BOOL("target_window", "show_distance", g_settings.target_window.show_distance),
        FIELD_BOOL("target_window", "show_tot", g_settings.target_window.show_tot),
        FIELD_BOOL("target_window", "show_target_details", g_settings.target_window.show_target_details),
        FIELD_BOOL("target_window", "show_equipment_inspector", g_settings.target_window.show_equipment_inspector),

        FIELD_BOOL("widgets", "static_only", g_settings.widgets.static_only),
        FIELD_BOOL("widgets", "auto_refresh", g_settings.widgets.auto_refresh),
        FIELD_INT("widgets", "max_depth", g_settings.widgets.max_depth),
        FIELD_BOOL("widgets", "show_alarm_debug", g_settings.widgets.show_alarm_debug),
        FIELD_BOOL("widgets", "deep_scan", g_settings.widgets.deep_scan),
        FIELD_INT("widgets", "probe_max_id", g_settings.widgets.probe_max_id),

        FIELD_BOOL("interface_manager", "apply_on_startup", g_settings.interface_manager.apply_on_startup),

        FIELD_BOOL("hud_esp", "enabled", g_settings.hud_esp.enabled),
        FIELD_BOOL("hud_esp", "show_target_indicator", g_settings.hud_esp.show_target_indicator),
        FIELD_BOOL("hud_esp", "show_target_snapline", g_settings.hud_esp.show_target_snapline),
        FIELD_BOOL("hud_esp", "show_target_info", g_settings.hud_esp.show_target_info),
        FIELD_BOOL("hud_esp", "show_monster_esp", g_settings.hud_esp.show_monster_esp),
        FIELD_BOOL("hud_esp", "show_monster_hp_bar", g_settings.hud_esp.show_monster_hp_bar),
        FIELD_BOOL("hud_esp", "show_unique_alert", g_settings.hud_esp.show_unique_alert),
        FIELD_BOOL("hud_esp", "show_boss_radar", g_settings.hud_esp.show_boss_radar),
        FIELD_BOOL("hud_esp", "show_radar_compass_widget", g_settings.hud_esp.show_radar_compass_widget),
        FIELD_BOOL("hud_esp", "show_player_esp", g_settings.hud_esp.show_player_esp),
        FIELD_BOOL("hud_esp", "show_self_esp", g_settings.hud_esp.show_self_esp),
        FIELD_BOOL("hud_esp", "show_item_drop_esp", g_settings.hud_esp.show_item_drop_esp),
        FIELD_BOOL("hud_esp", "show_npc_esp", g_settings.hud_esp.show_npc_esp),
        FIELD_BOOL("hud_esp", "show_pet_esp", g_settings.hud_esp.show_pet_esp),
        FIELD_BOOL("hud_esp", "show_telemetry_hud", g_settings.hud_esp.show_telemetry_hud),
        FIELD_BOOL("hud_esp", "show_local_player_ring", g_settings.hud_esp.show_local_player_ring),
        FIELD_BOOL("hud_esp", "show_mesh_boxes", g_settings.hud_esp.show_mesh_boxes),
        FIELD_BOOL("hud_esp", "show_skeleton", g_settings.hud_esp.show_skeleton),
        FIELD_BOOL("hud_esp", "show_spine_only", g_settings.hud_esp.show_spine_only),
        FIELD_INT("hud_esp", "min_rarity", g_settings.hud_esp.min_rarity),
        FIELD_INT("hud_esp", "max_distance_meters", g_settings.hud_esp.max_distance_meters),
        FIELD_FLOAT("hud_esp", "target_ring_radius", g_settings.hud_esp.target_ring_radius),
        FIELD_FLOAT("hud_esp", "skeleton_thickness", g_settings.hud_esp.skeleton_thickness),
        FIELD_FLOAT("hud_esp", "mesh_box_thickness", g_settings.hud_esp.mesh_box_thickness),
        FIELD_FLOAT("hud_esp", "overhead_offset_y", g_settings.hud_esp.overhead_offset_y),
    };

    auto apply_field(const field_desc &fd, const char *value) -> void {
      switch (fd.type) {
      case field_type::bool_t:
        *static_cast<bool *>(fd.ptr) = parse_bool(value, *static_cast<bool *>(fd.ptr));
        break;
      case field_type::int_t:
        *static_cast<int *>(fd.ptr) = parse_int(value, *static_cast<int *>(fd.ptr));
        break;
      case field_type::float_t:
        *static_cast<float *>(fd.ptr) = static_cast<float>(atof(value));
        break;
      case field_type::string_t:
        std::strncpy(static_cast<char *>(fd.ptr), value, fd.size - 1);
        static_cast<char *>(fd.ptr)[fd.size - 1] = '\0';
        break;
      case field_type::hex16_t:
        *static_cast<std::uint16_t *>(fd.ptr) = static_cast<std::uint16_t>(strtoul(value, nullptr, 16));
        break;
      case field_type::hex32_t:
        *static_cast<std::uint32_t *>(fd.ptr) = static_cast<std::uint32_t>(strtoul(value, nullptr, 16));
        break;
      }
    }

    auto save_field(std::ostream &file, const field_desc &fd) -> void {
      switch (fd.type) {
      case field_type::bool_t:
        file << fd.key << "=" << (*static_cast<bool *>(fd.ptr) ? 1 : 0) << "\n";
        break;
      case field_type::int_t:
        file << fd.key << "=" << *static_cast<int *>(fd.ptr) << "\n";
        break;
      case field_type::float_t:
        file << fd.key << "=" << *static_cast<float *>(fd.ptr) << "\n";
        break;
      case field_type::string_t:
        file << fd.key << "=" << static_cast<char *>(fd.ptr) << "\n";
        break;
      case field_type::hex16_t:
        file << fd.key << "=0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase
             << *static_cast<std::uint16_t *>(fd.ptr) << "\n"
             << std::dec << std::nouppercase;
        break;
      case field_type::hex32_t:
        file << fd.key << "=0x" << std::setfill('0') << std::setw(8) << std::hex << std::uppercase
             << *static_cast<std::uint32_t *>(fd.ptr) << "\n"
             << std::dec << std::nouppercase;
        break;
      }
    }

    auto apply_key(const char *section, const char *key, const char *value) -> void {
      if (!section || !key || !value) {
        return;
      }

      for (const auto &fd : g_fields) {
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

              auto &rule = g_settings.interface_manager.hidden[index];
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

  auto data() -> core_config & {
    return g_settings;
  }

  auto path() -> const char * {
    std::lock_guard lock(event::dispatch_mutex());
    ensure_path();
    return g_path;
  }

  auto load() -> bool {
    std::lock_guard lock(event::dispatch_mutex());
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
    ++g_revision;
    publish_runtime();
    log_msg("[core_config] loaded %s", g_path);
    return true;
  }

  auto runtime() -> std::shared_ptr<const core_config> {
    return g_runtime.load();
  }
  auto publish_runtime() -> void {
    std::lock_guard lock(event::dispatch_mutex());
    const auto revision = g_revision.load();
    if (revision == g_published_revision)
      return;
    g_runtime.store(std::make_shared<const core_config>(g_settings));
    g_published_revision = revision;
  }
  auto save() -> bool {
    std::lock_guard save_lock(g_save_mutex);
    std::ostringstream file;
    std::uint64_t revision;
    {
      std::lock_guard lock(event::dispatch_mutex());
      ensure_path();
      revision = g_revision.load();
      const char *prev_section = nullptr;
      for (const auto &fd : g_fields) {
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
        const auto &im = g_settings.interface_manager;
        for (int i = 0; i < im.hidden_count; ++i) {
          const auto &rule = im.hidden[i];
          if (rule.res_key == 0) {
            continue;
          }
          file << "hide_" << i << "=" << (rule.ingame_map ? "ingame:" : "iface:") << std::hex << std::showbase
               << rule.res_key << std::dec << ":" << rule.label << "\n";
        }
      }

      file << "\n[plugins]\n";
      for (const auto &plugin : ext_client::core::plugin::plugin_manager::get().get_plugins()) {
        file << plugin.id << "=" << (plugin.enabled ? 1 : 0) << "\n";
      }
      file << "\n";
    }
    const std::string temporary = std::string(g_path) + ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    const auto contents = file.str();
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.flush();
    const bool written = output.good();
    output.close();
    if (!written || !MoveFileExA(temporary.c_str(), g_path, MOVEFILE_REPLACE_EXISTING)) {
      log_msg("[core_config] failed to save config: %s", g_path);
      return false;
    }
    {
      std::lock_guard lock(event::dispatch_mutex());
      if (revision == g_revision.load())
        g_dirty.store(false);
    }
    log_msg("[core_config] saved %s", g_path);
    return true;
  }
  auto flush_pending() -> void {
    if (g_dirty.load() && runtime()->general.save_on_change && GetTickCount() - g_last_change.load() >= 500)
      save();
  }

  auto sync_to_runtime() -> void {
    using namespace ext_client::core::event;
    publish_runtime();
    TRIGGER_EVENT(EVENT_ON_CONFIG_SYNC);
    TRIGGER_EVENT(EVENT_ON_TICK);
  }

  auto mark_dirty() -> void {
    g_last_change.store(GetTickCount());
    ++g_revision;
    g_dirty.store(true);
  }

  auto is_dirty() -> bool {
    return g_dirty.load();
  }
} // namespace ext_client::core::config
