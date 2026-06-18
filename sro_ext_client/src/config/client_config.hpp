#pragma once

#include "utils/vectorf.h"

#include <cstdint>

namespace ext_client::config {

  struct settings {
    settings() {}

    struct interface_hide_t {
      bool hide_facebook = true;
      bool hide_magic_lamp = true;
      bool hide_daily_login = true;
      bool hide_survey = true;
      bool hide_web_item_alarm = true;
      bool hide_macro_guide = true;
      bool apply_on_startup = true;
    } interface_hide;

    struct hidden_widget_t {
      int res_key = 0;
      bool ingame_map = false;
      char label[64]{};
    };

    struct interface_manager_t {
      static constexpr int max_hidden = 32;
      hidden_widget_t hidden[max_hidden]{};
      int hidden_count = 0;
      bool apply_on_startup = true;
    } interface_manager;

    struct widgets_t {
      bool static_only = false;
      bool auto_refresh = false;
      int max_depth = 0;
      bool show_alarm_debug = false;
      bool deep_scan = false;
      int probe_max_id = 256;
    } widgets;

    struct general_t {
      bool save_on_change = true;
    } general;

    struct welcome_msg_t {
      bool enabled = true;
      char text[256] = "Welcome back, we hope you enjoy your stay.";
    } welcome_msg;

    struct title_t {
      bool enabled = true;
      bool log_events = true;
      bool skip_on_create = false;
      bool auto_login_on_create = false;
      bool block_login_packets = false;
      bool block_process_transition = false;
      bool suppress_captcha = false;
      bool hide_channel_list_button = true;
      bool replace_login_frame = true;
      char login_frame_path[128] = "interface\\outer\\login_window_eu_located.ddj";
      vector2f eu_login_id_label_adjust{65.f, 10.f};
      vector2f eu_login_id_input_adjust{80.f, 9.f};
      vector2f eu_login_pw_label_adjust{65.f, 15.f};
      vector2f eu_login_pw_input_adjust{52.f, 14.f};
      vector2f eu_login_server_label_adjust{65.f, -8.f};
      vector2f eu_login_server_value_adjust{20.f, -10.f};
      vector2f eu_login_server_button_adjust{-10.f, -10.f};
      int logo_y_offset = 32;
      bool override_version_labels = true;
      bool override_version_label_color = true;
      bool version_labels_clip = true;
      char data_version_fmt[128] = "Client Data Version %d.%03d";
      char exe_version_fmt[128] = "Client Exe Version %.3f";
      std::uint32_t version_label_color = 0xFFFFFFFF;
      int version_label_ellipsis_width = 80;
    } title;

    struct version_check_t {
      bool enabled = true;
      bool log_events = true;
      bool bypass_gateway_connect = false;
      bool force_version_result = false;
      int forced_version_result = 1;
      bool skip_textdata_load = false;
      bool block_process_transition = false;
      bool skip_loading_splash = false;
      bool banner_custom_size = true;
      int banner_width = 400;
      int banner_height = 148;
      bool banner_center = true;
      int banner_x = 0;
      int banner_y = 0;
      bool banner_cycle = true;
      int banner_cycle_interval_ms = 750;
      int banner_count = 10;
      char banner_path_fmt[128] = "interface\\loading\\start_loading_%02d.ddj";
      bool banner_overlay = true;
    } version_check;

    struct packet_t {
      bool enabled = true;
      bool log_events = false;
      bool edit_outgoing = false;
      std::uint16_t edit_outgoing_opcode = 0;
      bool edit_outgoing_apply_all = false;
      bool edit_incoming = false;
      std::uint16_t edit_incoming_opcode = 0;
      bool edit_incoming_apply_all = false;
    } packet;

    struct net_manager_t {
      bool enabled = true;
      bool log_outgoing = true;
      bool log_incoming = true;
      bool capture_cmsg = true;
      bool capture_stream = true;
      bool log_to_file = false;
      bool pause_capture = false;
      bool block_outgoing = false;
      bool block_incoming = false;
      int block_opcode_mode = 0;
      std::uint16_t block_opcode = 0;
      char block_opcode_list[128]{};
      int max_entries = 2048;
      char file_path[260] = "packets.log";
    } net_manager;

    struct graphic_t {
      bool custom_sight_range = false;
      int sight_range_level = 6;
      int sight_range_percent = 136;
      bool d3d_force_hardware_vp = true;
      bool d3d_force_pure_device = false;
      bool d3d_triple_buffering = true;
      bool d3d_discard_depth_stencil = true;
    } graphic;

    struct combat_overlay_t {
      bool enabled = true;
      bool show_hp_percent = true;
      bool show_dps = true;
      bool show_party_dps = false;
      bool reset_on_target_death = true;
      bool reset_on_target_change = true;
      int combat_timeout_sec = 8;
      int skill_cast_attack_type = -1;
      float dps_window_x = -1.0f;
      float dps_window_y = -1.0f;
    } combat_overlay;
  };

  auto data() -> settings&;
  auto path() -> const char*;
  auto load() -> bool;
  auto save() -> bool;
  auto sync_to_runtime() -> void;
  auto mark_dirty() -> void;
  [[nodiscard]] auto is_dirty() -> bool;

} // namespace ext_client::config
