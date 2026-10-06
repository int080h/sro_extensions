#pragma once

#include "utils/vectorf.hpp"

#include <cstdint>
#include <memory>

namespace ext_client::core::config {

  struct core_config {
    core_config() {}

    // =========================================================================
    // 1. General Settings
    // =========================================================================
    struct general_t {
      bool save_on_change = true;
    } general;

    // =========================================================================
    // 2. Graphics Settings
    // =========================================================================
    struct graphic_t {
      bool d3d_force_hardware_vp = true;
      bool d3d_force_pure_device = false;
      bool d3d_triple_buffering = true;
      bool d3d_discard_depth_stencil = true;
    } graphic;

    // =========================================================================
    // 3. Welcome Message Settings
    // =========================================================================
    struct welcome_msg_t {
      bool enabled = true;
      bool hide = false;
      char text[256] = "Welcome back, we hope you enjoy your stay.";
    } welcome_msg;

    // =========================================================================
    // 4. Title / Login Screen Settings
    // =========================================================================
    struct title_t {
      bool enabled = true;
      bool log_events = true;

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

    // =========================================================================
    // 5. Version Check Phase Settings
    // =========================================================================
    struct version_check_t {
      bool enabled = true;
      bool log_events = true;

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
      bool ensure_minimize_button = true;
    } version_check;

    // =========================================================================
    // 6. Network & Packets Settings
    // =========================================================================
    struct net_config_t {
      // capture
      bool enabled = true;
      bool pause_capture = false;
      bool log_outgoing = true;
      bool log_incoming = true;
      bool capture_cmsg = true;
      bool capture_stream = true;

      // blocking
      bool block_outgoing = false;
      bool block_incoming = false;
      int block_opcode_mode = 0;
      std::uint16_t block_opcode = 0;
      char block_opcode_list[128]{};

      // overrides
      bool edit_outgoing = false;
      std::uint16_t edit_outgoing_opcode = 0;
      bool edit_outgoing_apply_all = false;
      bool edit_incoming = false;
      std::uint16_t edit_incoming_opcode = 0;
      bool edit_incoming_apply_all = false;

      // logging
      bool log_events = false;
      bool log_to_file = false;
      char file_path[260] = "packets.log";
      int max_entries = 2048;

      // UI
      bool show_parsed = true;
      bool show_raw_hex = true;
      int filter_opcode = 0;
      bool filter_enabled = false;
      int filter_direction = 0; // 0=all, 1=in, 2=out

      int filter_opcode_mode = 0;     // 0=off, 1=single, 2=list, 3=name search
      char filter_opcode_list[256]{}; // comma-separated hex opcodes for mode 2
      char filter_opcode_name[64]{};  // text search for mode 3
      int filter_category = 0;        // 0=all, 1..=category
      char filter_search_text[64]{};  // general search (opcode/name/payload)
      int filter_layer = 0;           // 0=all, 1=cmsg, 2=stream
      bool filter_massive_only = false;
      bool filter_blocked_only = false;
      bool filter_modified_only = false;
      int min_payload_size = 0; // 0 = no min
      int max_payload_size = 0; // 0 = no max

      // UI options
      bool auto_scroll = true;
      bool show_timestamps = true;
      bool show_stats_bar = true;
      int sort_column = 0; // 0=tick, 1=dir, 2=layer, 3=opcode, 4=size
      bool sort_ascending = true;

      // Session
      char session_path[260] = "packet_session.bin";
    } net;

    // =========================================================================
    // 7. HUD Quick Hides Settings
    // =========================================================================
    struct interface_hide_t {
      bool hide_facebook = true;
      bool hide_magic_lamp = true;
      bool hide_daily_login = false;
      bool hide_survey = false;
      bool hide_web_item_alarm = false;
      bool hide_macro_guide = false;
      bool apply_on_startup = true;
    } interface_hide;

    // =========================================================================
    // 8. HUD Target Window HP Overlay & Enhancements Settings
    // =========================================================================
    struct target_window_t {
      bool enabled = true;
      bool show_hp_percent = true;
      bool show_distance = true;
      bool show_tot = true;
      bool show_target_details = true;
      bool show_equipment_inspector = false;
    } target_window;

    // =========================================================================
    // 9. HUD Custom Hides Manager Settings
    // =========================================================================
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

    // =========================================================================
    // 10. HUD Widget Debugger Settings
    // =========================================================================
    struct widgets_t {
      bool static_only = false;
      bool auto_refresh = false;
      int max_depth = 0;
      bool show_alarm_debug = false;
      bool deep_scan = false;
      int probe_max_id = 256;
    } widgets;

    // =========================================================================
    // 11. 3D World HUD / ESP Settings
    // =========================================================================
    struct hud_esp_t {
      bool enabled = true;
      bool show_target_indicator = true;
      bool show_target_snapline = false;
      bool show_target_info = true;
      bool show_monster_esp = true;
      bool show_monster_hp_bar = true;
      bool show_unique_alert = true;
      bool show_boss_radar = true;
      bool show_radar_compass_widget = false;
      bool show_player_esp = true;
      bool show_self_esp = false;
      bool show_item_drop_esp = true;
      bool show_npc_esp = false;
      bool show_pet_esp = true;
      bool show_telemetry_hud = false;
      bool show_local_player_ring = false;
      bool show_mesh_boxes = false;
      bool show_skeleton = false;
      bool show_spine_only = false;
      int min_rarity = 0; // 0=all, 1=champion+, 2=giant+, 3=unique only
      int max_distance_meters = 80;
      float target_ring_radius = 18.0f;
      float skeleton_thickness = 1.5f;
      float mesh_box_thickness = 1.2f;
      float overhead_offset_y = 36.0f;
    } hud_esp;
  };

  using config_data = core_config;

  auto data() -> core_config &;
  auto runtime() -> std::shared_ptr<const core_config>;
  auto publish_runtime() -> void;
  auto flush_pending() -> void;
  auto path() -> const char *;
  auto load() -> bool;
  auto save() -> bool;
  auto sync_to_runtime() -> void;
  auto mark_dirty() -> void;
  [[nodiscard]] auto is_dirty() -> bool;

} // namespace ext_client::core::config
