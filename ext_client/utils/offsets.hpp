#pragma once

#include "utils/layout.hpp"
#include <cstddef>
#include <cstdint>

namespace ext_client::offsets {

  // ======================================================================
  // Consolidating cd3d_application.hpp
  // ======================================================================
  namespace cd3d_application {
    inline constexpr std::size_t size = 0x890;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x01068CC4;
      inline constexpr std::size_t method_count = 17;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t enumeration = 0x004;
      inline constexpr std::size_t use_windowed_device = 0x034;
      inline constexpr std::size_t windowed_backbuffer_width = 0x038;
      inline constexpr std::size_t windowed_backbuffer_height = 0x03C;
      inline constexpr std::size_t padapter_info_windowed = 0x040;
      inline constexpr std::size_t windowed_device_type = 0x060;
      inline constexpr std::size_t fullscreen_backbuffer_width = 0x070;
      inline constexpr std::size_t fullscreen_backbuffer_height = 0x074;
      inline constexpr std::size_t padapter_info_fullscreen = 0x078;
      inline constexpr std::size_t fullscreen_device_type = 0x098;
      inline constexpr std::size_t b_windowed = 0x0A0;
      inline constexpr std::size_t b_active = 0x0A1;
      inline constexpr std::size_t b_ready = 0x0A2;
      inline constexpr std::size_t b_has_focus = 0x0A3;
      inline constexpr std::size_t b_device_lost = 0x0A4;
      inline constexpr std::size_t b_minimized = 0x0A5;
      inline constexpr std::size_t b_unknown_a6 = 0x0A6;
      inline constexpr std::size_t b_unknown_a7 = 0x0A7;
      inline constexpr std::size_t b_unknown_a8 = 0x0A8;
      inline constexpr std::size_t d3dpp = 0x0AC;
      inline constexpr std::size_t hwnd = 0x0E4;
      inline constexpr std::size_t hwnd_focus = 0x0E8;
      inline constexpr std::size_t hwnd_device = 0x0EC;
      inline constexpr std::size_t d3d = 0x0F0;
      inline constexpr std::size_t device = 0x0F4;
      inline constexpr std::size_t behavior_flags = 0x248;
      inline constexpr std::size_t window_style = 0x24C;
      inline constexpr std::size_t rc_window = 0x250;
      inline constexpr std::size_t rc_client = 0x260;
      inline constexpr std::size_t stats = 0x270;
      inline constexpr std::size_t unknown_27c = 0x27C;
      inline constexpr std::size_t fps_text = 0x2D6;
      inline constexpr std::size_t fps_text_length = 0x5A;
      inline constexpr std::size_t window_title = 0x330;
      inline constexpr std::size_t creation_width = 0x334;
      inline constexpr std::size_t creation_height = 0x338;
      inline constexpr std::size_t b_unknown_33c = 0x33C;
      inline constexpr std::size_t b_unknown_33d = 0x33D;
      inline constexpr std::size_t b_unknown_33e = 0x33E;
      inline constexpr std::size_t msg_handler = 0x340;
      inline constexpr std::size_t is_initialized = 0x344;
      inline constexpr std::size_t unknown_348 = 0x348;
      inline constexpr std::size_t render_settings = 0x35C;
      inline constexpr std::size_t devmode = 0x4B4;
      inline constexpr std::size_t devmode_size = 0x9C;
      inline constexpr std::size_t gamma_ramp = 0x590;
      inline constexpr std::size_t gamma_ramp_size = 0x300;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t toggle_fullscreen = 0x00D7CAB0;
      inline constexpr std::uint32_t set_menu = 0x00D7B440;
      inline constexpr std::uint32_t build_device_list = 0x00D7C3C0;
      inline constexpr std::uint32_t confirm_device = 0x00D78E80;
      inline constexpr std::uint32_t one_time_scene_init = 0x00D78E90;
      inline constexpr std::uint32_t init_device_objects = 0x00D78EA0;
      inline constexpr std::uint32_t restore_device_objects = 0x00D78EB0;
      inline constexpr std::uint32_t invalidate_device_objects = 0x00D78EC0;
      inline constexpr std::uint32_t delete_device_objects = 0x00D78ED0;
      inline constexpr std::uint32_t final_cleanup = 0x00D78EE0;
      inline constexpr std::uint32_t frame_move = 0x00D78EF0;
      inline constexpr std::uint32_t render = 0x00D78F00;
      inline constexpr std::uint32_t create = 0x00D7CEB0;
      inline constexpr std::uint32_t create_device = 0x00D7CDA0;
      inline constexpr std::uint32_t reset_3d_environment = 0x00D7D0B0;
      inline constexpr std::uint32_t msg_proc = 0x00D7B4D0;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x00D78F10;
    } // namespace functions
  } // namespace cd3d_application

  namespace d3d_adapter_info {
    inline constexpr std::size_t size = 0x45C;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x010698C0;
    }
  } // namespace d3d_adapter_info

  namespace d3d_device_info {
    inline constexpr std::size_t size = 0x140;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x010698C8;
    }
  } // namespace d3d_device_info

  namespace globals {
    inline constexpr std::uint32_t d3d_app = 0x013BAE1C;
  }

  // ======================================================================
  // Consolidating cgfx_video3d.hpp
  // ======================================================================
  namespace cgfx_video3d {
    inline constexpr std::size_t size = 0xB90;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x01068D14;
      inline constexpr std::size_t method_count = 37;

      inline constexpr std::size_t toggle_fullscreen = 0;
      inline constexpr std::size_t set_menu = 1;
      inline constexpr std::size_t build_device_list = 2;
      inline constexpr std::size_t confirm_device = 3;
      inline constexpr std::size_t scalar_deleting_dtor = 16;
      inline constexpr std::size_t create_things = 17;
      inline constexpr std::size_t destroy_things = 18;
      inline constexpr std::size_t init_render = 19;
      inline constexpr std::size_t set_size = 20;
      inline constexpr std::size_t get_size = 21;
      inline constexpr std::size_t get_backbuffer_format = 22;
      inline constexpr std::size_t set_gamma = 23;
      inline constexpr std::size_t begin_scene = 24;
      inline constexpr std::size_t end_scene_pre = 25;
      inline constexpr std::size_t end_scene = 26;
      inline constexpr std::size_t present = 27;
      inline constexpr std::size_t set_format = 28;
      inline constexpr std::size_t get_format = 29;
      inline constexpr std::size_t set_fov = 30;
      inline constexpr std::size_t set_clip_planes = 31;
      inline constexpr std::size_t set_light = 32;
      inline constexpr std::size_t set_material = 33;
      inline constexpr std::size_t set_view_matrix = 34;
      inline constexpr std::size_t set_proj_matrix = 35;
      inline constexpr std::size_t set_world_matrix = 36;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t multisample_quality = 0x024;
      inline constexpr std::size_t b_unknown_2c = 0x02C;
      inline constexpr std::size_t render_target_width = 0x350;
      inline constexpr std::size_t render_target_height = 0x354;
      inline constexpr std::size_t display_frequency = 0x52C;
      inline constexpr std::size_t b_unknown_4b0 = 0x4B0;
      inline constexpr std::size_t b_unknown_4b1 = 0x4B1;
      inline constexpr std::size_t b_unknown_4b2 = 0x4B2;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t toggle_fullscreen = 0x00D78770;
      inline constexpr std::uint32_t confirm_device = 0x00D786F0;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x00D79E20;
      inline constexpr std::uint32_t create_things = 0x00D79210;
      inline constexpr std::uint32_t destroy_things = 0x00D783E0;
      inline constexpr std::uint32_t init_render = 0x00D78520;
      inline constexpr std::uint32_t set_size = 0x00D783F0;
      inline constexpr std::uint32_t get_size = 0x00D78500;
      inline constexpr std::uint32_t get_backbuffer_format = 0x00D78550;
      inline constexpr std::uint32_t set_gamma = 0x00D79D00;
      inline constexpr std::uint32_t begin_scene = 0x00D785A0;
      inline constexpr std::uint32_t end_scene_pre = 0x00D79D90;
      inline constexpr std::uint32_t end_scene = 0x00D785B0;
      inline constexpr std::uint32_t present = 0x00D785D0;
      inline constexpr std::uint32_t set_format = 0x00D78610;
      inline constexpr std::uint32_t get_format = 0x00D78630;
      inline constexpr std::uint32_t set_fov = 0x00D78640;
      inline constexpr std::uint32_t set_clip_planes = 0x00D78650;
      inline constexpr std::uint32_t set_light = 0x00D79A70;
      inline constexpr std::uint32_t set_material = 0x00D79B20;
      inline constexpr std::uint32_t set_view_matrix = 0x00D786A0;
      inline constexpr std::uint32_t set_proj_matrix = 0x00D78670;
      inline constexpr std::uint32_t set_world_matrix = 0x00D78660;
    } // namespace functions
  } // namespace cgfx_video3d

  // ======================================================================
  // Consolidating cclient_net.hpp
  // ======================================================================
  namespace cclient_session {
    inline constexpr std::size_t size = 0x300;

    namespace fields {
      inline constexpr std::size_t connected = 0x000;
      inline constexpr std::size_t connection_id = 0x030;
      inline constexpr std::size_t locale_byte = 0x034;
      inline constexpr std::size_t username = 0x0A0;
      inline constexpr std::size_t password = 0x0B8;
      inline constexpr std::size_t challenge_block = 0x124;
      inline constexpr std::size_t server_name_a = 0x15C;
      inline constexpr std::size_t secondary_user = 0x25C;
      inline constexpr std::size_t gate_host = 0x278;
      inline constexpr std::size_t security_version_a = 0x2F4;
      inline constexpr std::size_t security_version_b = 0x2F6;
      inline constexpr std::size_t security_token = 0x2F8;
      inline constexpr std::size_t security_flag = 0x2FC;
    } // namespace fields

    namespace globals {
      inline constexpr std::uint32_t instance_ptr = 0x014203FC; // Src
    }
  } // namespace cclient_session

  namespace cclient_net {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x0108678C; // ??_7CClientNet@@6B@
      inline constexpr std::size_t method_count = 39;

      inline constexpr std::size_t connect = 0;
      inline constexpr std::size_t disconnect = 1;
      inline constexpr std::size_t send_login = 5;
      inline constexpr std::size_t pump_messages = 6;
      inline constexpr std::size_t is_connected = 7;
      inline constexpr std::size_t send_patch_request = 8;
      inline constexpr std::size_t send_patch_confirm = 9;
      inline constexpr std::size_t alloc_msg_buffer = 10;
      inline constexpr std::size_t free_msg_buffer = 11;
      inline constexpr std::size_t send_msg_buffer = 12;
      inline constexpr std::size_t build_msg = 18;
      inline constexpr std::size_t send_handshake = 19;
      inline constexpr std::size_t send_to_agent = 20;
      inline constexpr std::size_t get_challenge = 24;
      inline constexpr std::size_t verify_challenge = 25;
      inline constexpr std::size_t has_active_socket = 27;
      inline constexpr std::size_t send_keepalive = 33;
      inline constexpr std::size_t handle_security_blob = 35;
      inline constexpr std::size_t set_event_sink = 37;
      inline constexpr std::size_t scalar_deleting_dtor = 38;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t event_sink = 0x004;
      inline constexpr std::size_t engine = 0x008;
      inline constexpr std::size_t process_registry = 0x4E8;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t init_singleton = 0x0045CB70;
      inline constexpr std::uint32_t ctor = 0x0045CB70;
      inline constexpr std::uint32_t dtor = 0x0045CC40;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x0045CC10;
      inline constexpr std::uint32_t connect = 0x0045CCF0;
      inline constexpr std::uint32_t disconnect = 0x0045D1D0;
      inline constexpr std::uint32_t send_login = 0x0045D2C0;
      inline constexpr std::uint32_t pump_messages = 0x0045D5B0;
      inline constexpr std::uint32_t is_connected = 0x0045D800;
      inline constexpr std::uint32_t alloc_msg_buffer = 0x0045D200;
      inline constexpr std::uint32_t send_msg_buffer = 0x0045D250;
      inline constexpr std::uint32_t build_msg = 0x0045D840;
      inline constexpr std::uint32_t send_handshake = 0x0045D8F0;
      inline constexpr std::uint32_t send_to_agent = 0x0045D9A0;
      inline constexpr std::uint32_t send_keepalive = 0x0045DB80;
      inline constexpr std::uint32_t handle_security_blob = 0x0045DC10;
      inline constexpr std::uint32_t set_event_sink = 0x0045B3D0;
      inline constexpr std::uint32_t session_ctor = 0x0045BD00;
    } // namespace functions

    namespace globals {
      inline constexpr std::uint32_t instance_ptr = 0x01420400;  // dword_1420400 -> CClientNet*
      inline constexpr std::uint32_t active_socket = 0x01420404; // dword_1420404 -> IBSNet*
      inline constexpr std::uint32_t storage = 0x01420B00;       // dword_1420B00 blob
      inline constexpr std::uint32_t instance = instance_ptr;    // legacy alias
    } // namespace globals

    namespace packets {
      inline constexpr std::uint16_t login = 0x610A;
      inline constexpr std::uint16_t keepalive = 0x610F;
      inline constexpr std::uint16_t patch_request = 0x2F01;
      inline constexpr std::uint16_t patch_confirm = 0x2F02;
      inline constexpr std::uint16_t agent_auth = 0x2013;
      inline constexpr std::uint16_t gate_auth = 0x6323;
    } // namespace packets
  } // namespace cclient_net

  // ======================================================================
  // Consolidating custom-overridden cps_character_select.hpp
  // ======================================================================
  namespace cps_character_select {
    inline constexpr std::size_t size = 0x240;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x0103298C;
      inline constexpr std::size_t method_count = 41;

      inline constexpr std::size_t get_factory_entry = 0;
      inline constexpr std::size_t get_process_id = 1;
      inline constexpr std::size_t scalar_deleting_dtor = 2;
      inline constexpr std::size_t get_res_map = 3;
      inline constexpr std::size_t gwnd_helper_04 = 4;
      inline constexpr std::size_t on_timer = 5;
      inline constexpr std::size_t on_msg_recv = 6;
      inline constexpr std::size_t insert_child = 7;
      inline constexpr std::size_t remove_child = 8;
      inline constexpr std::size_t remove_child_by_id = 9;
      inline constexpr std::size_t on_create = 10;
      inline constexpr std::size_t on_active = 11;
      inline constexpr std::size_t on_update = 12;
      inline constexpr std::size_t on_render_begin = 13;
      inline constexpr std::size_t on_render = 14;
      inline constexpr std::size_t on_render_end = 15;
      inline constexpr std::size_t run_process = 16;
      inline constexpr std::size_t on_enter = 17;
      inline constexpr std::size_t on_leave = 18;
      inline constexpr std::size_t on_lost_focus = 19;
      inline constexpr std::size_t on_got_focus = 20;
      inline constexpr std::size_t on_size = 21;
      inline constexpr std::size_t on_move = 22;
      inline constexpr std::size_t on_activate = 23;
      inline constexpr std::size_t on_deactivate = 24;
      inline constexpr std::size_t on_close = 25;
      inline constexpr std::size_t on_minimize = 26;
      inline constexpr std::size_t on_restore = 27;
      inline constexpr std::size_t on_maximize = 28;
      inline constexpr std::size_t gwnd_mouse_move = 29;
      inline constexpr std::size_t gwnd_mouse_down = 30;
      inline constexpr std::size_t gwnd_mouse_up = 31;
      inline constexpr std::size_t gwnd_mouse_wheel = 32;
      inline constexpr std::size_t gwnd_key_down = 33;
      inline constexpr std::size_t gwnd_key_up = 34;
      inline constexpr std::size_t gwnd_char = 35;
      inline constexpr std::size_t gwnd_ime = 36;
      inline constexpr std::size_t null_37 = 37;
      inline constexpr std::size_t null_38 = 38;
      inline constexpr std::size_t on_cmd = 39;
      inline constexpr std::size_t on_net_error = 40;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t secondary_vftable = 0x10C;
      inline constexpr std::size_t anim_state = 0x118;
      inline constexpr std::size_t list_refresh_flag = 0x11C;
      inline constexpr std::size_t delete_dialog_mode = 0x11D;
      inline constexpr std::size_t char_count = 0x12C;
      inline constexpr std::size_t page_index = 0x12D;
      inline constexpr std::size_t page_count = 0x12E;
      inline constexpr std::size_t logout_msg_pending = 0x12F;
      inline constexpr std::size_t selected_slot = 0x130;
      inline constexpr std::size_t characters_begin = 0x138;
      inline constexpr std::size_t characters_end = 0x13C;
      inline constexpr std::size_t characters_capacity = 0x140;
      inline constexpr std::size_t rename_wstring = 0x144;
      inline constexpr std::size_t char_objects_begin = 0x148;
      inline constexpr std::size_t char_objects_end = 0x14C;
      inline constexpr std::size_t char_objects_capacity = 0x150;
      inline constexpr std::size_t ui_map_key = 0x154;
      inline constexpr std::size_t ui_map_tree = 0x158;
      inline constexpr std::size_t ui_map_count = 0x15C;
      inline constexpr std::size_t slot_rig_0 = 0x164;
      inline constexpr std::size_t slot_rig_1 = 0x194;
      inline constexpr std::size_t slot_rig_2 = 0x1C4;
      inline constexpr std::size_t slot_rig_extra = 0x1F4;
      inline constexpr std::size_t unity_mode = 0x160;
      inline constexpr std::size_t field_228 = 0x228;
      inline constexpr std::size_t field_564 = 0x234;
      inline constexpr std::size_t field_568 = 0x238;
      inline constexpr std::size_t sec_pw_flag_a = 0x239;
      inline constexpr std::size_t sec_pw_flag_b = 0x23A;
      inline constexpr std::uint32_t autologin_checked = 0x23B;
      inline constexpr std::size_t fade_value = 0x23C;
    } // namespace fields

    namespace globals {
      inline constexpr std::uint32_t factory_entry = 0x0117E85C;
      inline constexpr std::uint32_t process_id = 0x0117E868;
      inline constexpr std::uint32_t factory_process_tag = 0x0117E8A4;
      inline constexpr std::uint32_t selected_slot_index = 0x01152638;
      inline constexpr std::uint32_t slot_camera_begin = 0x0117E840;
      inline constexpr std::uint32_t slot_camera_end = 0x0117E844;
      inline constexpr std::uint32_t ui_anchor_begin = 0x0117E850;
      inline constexpr std::uint32_t ui_anchor_end = 0x0117E854;
      inline constexpr std::uint32_t res_map = 0x01031DEC;
      inline constexpr std::uint32_t pin_required_flag = 0x0117E3D0;
      inline constexpr std::uint32_t process_manager = 0x013BAE00;
      inline constexpr std::uint32_t net_manager = 0x01420400;
    } // namespace globals

    namespace functions {
      inline constexpr std::uint32_t create_instance = 0x00963C00;
      inline constexpr std::uint32_t ctor = 0x00963AB0;
      inline constexpr std::uint32_t dtor = 0x00963950;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x00963BE0;
      inline constexpr std::uint32_t register_factory = 0x00FB26B0;
      inline constexpr std::uint32_t on_create = 0x0095F8A0;
      inline constexpr std::uint32_t on_active = 0x0095C6B0;
      inline constexpr std::uint32_t on_update = 0x00960610;
      inline constexpr std::uint32_t on_msg_recv = 0x00962800;
      inline constexpr std::uint32_t on_render = 0x0095E750;
      inline constexpr std::uint32_t on_enter = 0x0095EB60;
      inline constexpr std::uint32_t on_timer = 0x0095EB10;
      inline constexpr std::uint32_t init_defaults = 0x0095C9B0;
      inline constexpr std::uint32_t request_char_list = 0x00957CC0;
      inline constexpr std::uint32_t handle_char_list = 0x00962170;
      inline constexpr std::uint32_t enter_world_fade = 0x0095D550;
      inline constexpr std::uint32_t show_delete_dialog = 0x0095A6D0;
      inline constexpr std::uint32_t set_unity_panel = 0x009581A0;
      inline constexpr std::uint32_t get_ui_child = 0x009CF790;
      inline constexpr std::uint32_t set_child_process = 0x00D729E0;
      inline constexpr std::uint32_t quit_process = 0x00D72440;
      inline constexpr std::uint32_t process_allocate = 0x00D6A1A0;
      inline constexpr std::uint32_t outer_interface_ctor = 0x00965CB0;
      inline constexpr std::uint32_t on_slot_change = 0x00963270;
    } // namespace functions

    namespace ui_ids {
      inline constexpr int select_button = 1;
      inline constexpr int create_button = 2;
      inline constexpr int delete_button = 3;
      inline constexpr int exit_button = 4;
      inline constexpr int char_name = 9;
      inline constexpr int char_level = 10;
      inline constexpr int char_job = 11;
      inline constexpr int char_str = 12;
      inline constexpr int char_int = 13;
      inline constexpr int char_slot_0 = 14;
      inline constexpr int char_slot_1 = 15;
      inline constexpr int char_slot_2 = 16;
      inline constexpr int char_slot_3 = 17;
      inline constexpr int prev_page = 19;
      inline constexpr int next_page = 20;
      inline constexpr int page_label = 21;
      inline constexpr int china_panel = 22;
      inline constexpr int europe_panel = 23;
      inline constexpr int remain_time = 24;
      inline constexpr int warning_panel = 25;
      inline constexpr int info_panel = 26;
      inline constexpr int change_name = 27;
      inline constexpr int name_duplicate = 28;
      inline constexpr int rotate_control = 37;
      inline constexpr int delete_confirm = 42;
      inline constexpr int delete_title = 44;
      inline constexpr int delete_body = 45;
      inline constexpr int delete_ok = 46;
      inline constexpr int fade_overlay = 200;
      inline constexpr int fade_overlay_alt = 201;
      inline constexpr int msgbox = 500;
      inline constexpr int msgbox_alt = 501;
      inline constexpr int unity_panel = 900;
      inline constexpr int unity_title = 920;
      inline constexpr int unity_body = 921;
      inline constexpr int unity_existing = 922;
      inline constexpr int unity_change = 923;
      inline constexpr int unity_input = 925;
      inline constexpr int unity_close = 929;
      inline constexpr int unity_approve = 940;
      inline constexpr int unity_result = 941;
      inline constexpr int last_logout_label = 29;
    } // namespace ui_ids

    namespace login_phase {
      inline constexpr int idle = 0;
      inline constexpr int last_logout = 2;
      inline constexpr int entering_world = 6;
      inline constexpr int render_disabled = 12;
      inline constexpr int delete_confirm = 15;
      inline constexpr int return_to_login = 14;
    } // namespace login_phase

    namespace packets {
      inline constexpr std::uint16_t msgbox = 0x0FFC;
      inline constexpr std::uint16_t pin_ack = 0x1004;
      inline constexpr std::uint16_t handshake = 0xB001;
      inline constexpr std::uint16_t char_list_notify = 0xB007;
      inline constexpr std::uint16_t unity_name_response = 0xB450;
      inline constexpr std::uint16_t request_char_list = 0x7007;
    } // namespace packets

    namespace char_list_notify {
      inline constexpr int refresh_list = 2;
      inline constexpr int enter_fade = 3;
      inline constexpr int unity_approve = 4;
      inline constexpr int reset_and_enter = 5;
      inline constexpr int clear_slots = 6;
      inline constexpr int sec_pw_state = 9;
      inline constexpr int play_sound = 0x10;
    } // namespace char_list_notify

    namespace wm_user {
      inline constexpr int quit = 0x10;
      inline constexpr int last_logout = 0x201;
      inline constexpr int autologin_check = 0x203;
      inline constexpr int slot_positions = 0x805;
    } // namespace wm_user

    namespace res {
      inline constexpr const char* layout_file = "resinfo\\pscharacterselect.txt";
    }

    namespace s_character_info {
      inline constexpr std::size_t size = 0xA98;
      inline constexpr std::size_t deleted_flag = 0x918;
      inline constexpr std::size_t model_ref = 0x9C;
      inline constexpr std::size_t packed_time = 0x0FC;
    } // namespace s_character_info

    namespace slot_rig {
      inline constexpr std::size_t size = 0x30;
      inline constexpr std::size_t count = 3;
    } // namespace slot_rig
  } // namespace cps_character_select

  // ======================================================================
  // Consolidating globals.hpp
  // ======================================================================
  namespace globals {
    inline constexpr std::uint32_t cg_interface = 0x013BAE3C;
    inline constexpr std::uint32_t interface_under_cursor = 0x013BAFA8; // g_CurrentIfUnderCursor (CGWndBase*)
  } // namespace globals

  // ======================================================================
  // Consolidating cd3d_enumeration.hpp
  // ======================================================================
  namespace cd3d_enumeration {
    inline constexpr std::size_t size = 0x30;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x010698B8;
      inline constexpr std::size_t method_count = 1;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t adapter_list = 0x008;
      inline constexpr std::size_t min_width = 0x010;
      inline constexpr std::size_t min_height = 0x014;
      inline constexpr std::size_t min_format = 0x018;
      inline constexpr std::size_t min_refresh = 0x01C;
      inline constexpr std::size_t depth_bits = 0x020;
      inline constexpr std::size_t multisample = 0x024;
      inline constexpr std::size_t can_windowed = 0x028;
      inline constexpr std::size_t is_alpha = 0x029;
      inline constexpr std::size_t is_stereo = 0x02A;
      inline constexpr std::size_t device_combo = 0x02C;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x00D7DDE0;
    }
  } // namespace cd3d_enumeration

  // ======================================================================
  // Consolidating cps_version_check.hpp
  // ======================================================================
  namespace cps_version_check {
    inline constexpr std::size_t size = 0x110;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x010349CC;
      inline constexpr std::size_t method_count = 41;

      inline constexpr std::size_t get_factory_entry = 0;
      inline constexpr std::size_t get_process_id = 1;
      inline constexpr std::size_t scalar_deleting_dtor = 2;
      inline constexpr std::size_t get_res_map = 3;
      inline constexpr std::size_t gwnd_helper_04 = 4;
      inline constexpr std::size_t on_timer = 5;
      inline constexpr std::size_t on_msg_recv = 6;
      inline constexpr std::size_t insert_child = 7;
      inline constexpr std::size_t remove_child = 8;
      inline constexpr std::size_t remove_child_by_id = 9;
      inline constexpr std::size_t on_create = 10;
      inline constexpr std::size_t on_active = 11;
      inline constexpr std::size_t on_update = 12;
      inline constexpr std::size_t on_render_begin = 13;
      inline constexpr std::size_t on_render = 14;
      inline constexpr std::size_t on_render_end = 15;
      inline constexpr std::size_t run_process = 16;
      inline constexpr std::size_t on_enter = 17;
      inline constexpr std::size_t on_leave = 18;
      inline constexpr std::size_t on_lost_focus = 19;
      inline constexpr std::size_t on_got_focus = 20;
      inline constexpr std::size_t on_size = 21;
      inline constexpr std::size_t on_move = 22;
      inline constexpr std::size_t on_activate = 23;
      inline constexpr std::size_t on_deactivate = 24;
      inline constexpr std::size_t on_close = 25;
      inline constexpr std::size_t on_minimize = 26;
      inline constexpr std::size_t on_restore = 27;
      inline constexpr std::size_t on_maximize = 28;
      inline constexpr std::size_t gwnd_mouse_move = 29;
      inline constexpr std::size_t gwnd_mouse_down = 30;
      inline constexpr std::size_t gwnd_mouse_up = 31;
      inline constexpr std::size_t gwnd_mouse_wheel = 32;
      inline constexpr std::size_t gwnd_key_down = 33;
      inline constexpr std::size_t gwnd_key_up = 34;
      inline constexpr std::size_t gwnd_char = 35;
      inline constexpr std::size_t gwnd_ime = 36;
      inline constexpr std::size_t null_37 = 37;
      inline constexpr std::size_t null_38 = 38;
      inline constexpr std::size_t on_cmd = 39;
      inline constexpr std::size_t on_net_error = 40;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t child_process_list = 0x06C;
      inline constexpr std::size_t wnd_child_list = 0x07C;
      inline constexpr std::size_t net_state = 0x084;
      inline constexpr std::size_t load_thread = 0x0A0;
      inline constexpr std::size_t res_loader = 0x0E0;
      inline constexpr std::size_t cursor_id = 0x0E4;
      inline constexpr std::size_t field_e8 = 0x0E8;
      inline constexpr std::size_t gfx_child = 0x0F0;
      inline constexpr std::size_t field_f4 = 0x0F4;
      inline constexpr std::size_t timer_value = 0x0F8;
      inline constexpr std::size_t field_fc = 0x0FC;
      inline constexpr std::size_t msg_list_head = 0x100;
      inline constexpr std::size_t msg_list_ptr = 0x104;
      inline constexpr std::size_t msg_list_count = 0x108;
      inline constexpr std::size_t data_load_started = 0x10C;
    } // namespace fields

    namespace globals {
      inline constexpr std::uint32_t cg_interface = 0x0117DCC0;
      inline constexpr std::uint32_t process_manager = 0x013BAE00;
      inline constexpr std::uint32_t version_check_active = 0x0117E154;
      inline constexpr std::uint32_t version_error_code = 0x0117E8A0;
      inline constexpr std::uint32_t version_error_tag = 0x0117E89C;
      inline constexpr std::uint32_t factory_entry = 0x0117EBC8;
      inline constexpr std::uint32_t process_id = 0x0117EBD4;
      inline constexpr std::uint32_t intro_viewer_object = 0x01199114;
      inline constexpr std::uint32_t intro_active_region = 0x011991B4;
      inline constexpr std::uint32_t intro_pending_render_count = 0x0119B580;
      inline constexpr std::uint32_t intro_object_manager = 0x0119B828;
      inline constexpr std::uint32_t intro_region_manager = 0x013BAE40;
      inline constexpr std::uint32_t intro_collision_manager = 0x013B7390;
      inline constexpr std::uint32_t net_manager = 0x01420400;
      inline constexpr std::uint32_t loading_flag = 0x01145F48;
    } // namespace globals

    namespace functions {
      inline constexpr std::uint32_t create_instance = 0x00978C10;
      inline constexpr std::uint32_t ctor = 0x00978B90;
      inline constexpr std::uint32_t delete_instance = 0x00978B70;
      inline constexpr std::uint32_t register_factory = 0x00FB3600;
      inline constexpr std::uint32_t on_create = 0x00978DE0;
      inline constexpr std::uint32_t on_update = 0x00978CA0;
      inline constexpr std::uint32_t on_msg_recv = 0x00978D20;
      inline constexpr std::uint32_t on_active = 0x00978BD0;
      inline constexpr std::uint32_t get_child_wnd = 0x00978BF0;
      inline constexpr std::uint32_t connect_gateway = 0x00942690; // CPS_ConnectGateway
      inline constexpr std::uint32_t load_game_textdata = 0x00943C10;
      inline constexpr std::uint32_t get_intro_renderer = 0x00B523F0;
      inline constexpr std::uint32_t intro_render_stage_callback = 0x0094D050;
      inline constexpr std::uint32_t load_intro_camera = 0x0093EDE0;
      inline constexpr std::uint32_t run_intro_area_spawner = 0x00502690;
      inline constexpr std::uint32_t reset_intro_object_list = 0x004F95F0;
      inline constexpr std::uint32_t clear_intro_objects_for_script = 0x004F9530;
      inline constexpr std::uint32_t clone_game_string = 0x004E0F00;
      inline constexpr std::uint32_t create_intro_objects = 0x004FC930;
      inline constexpr std::uint32_t intro_random_position = 0x004EBA40;
      inline constexpr std::uint32_t intro_allocate_object = 0x00B47CF0;
      inline constexpr std::uint32_t gfx_resource_manager_fn = 0x013B4504;
      inline constexpr std::uint32_t register_object_render = 0x00BC36A0;
      inline constexpr std::uint32_t flush_render_manager = 0x00B2DF30;
      inline constexpr std::uint32_t refresh_object_visibility = 0x00B30270;
      inline constexpr std::uint32_t finalize_object_render = 0x00B33680;
      inline constexpr std::uint32_t activate_intro_region = 0x004DF6E0;
      inline constexpr std::uint32_t register_object_scene = 0x00B22140;
      inline constexpr std::uint32_t register_object_visibility = 0x00B25570;
      inline constexpr std::uint32_t map_render_category = 0x00B284B0;
      inline constexpr std::uint32_t flush_pending_render = 0x00B301E0;
      inline constexpr std::uint32_t set_object_visibility_state = 0x00B319B0;
    } // namespace functions

    namespace intro_renderer {
      // iSRO CPSMission::OnCreate; VSRO CPS_LoadGameTextData uses +0xA0 on a different layout.
      inline constexpr std::size_t vtbl_register_stage_callback = 0xA8;
      inline constexpr std::size_t vtbl_set_region = 0x04;

      namespace globals {
        inline constexpr std::uint32_t instance = 0x013AB480;
      } // namespace globals
    } // namespace intro_renderer

    namespace intro_object {
      inline constexpr std::size_t sight_range = 0x5F4;
      inline constexpr std::size_t sight_fade = 0x5F8;
      inline constexpr std::size_t render_weight = 0xC4;
      inline constexpr std::size_t scene_registered_flag = 0x70; // dword 0x1C bit 0; B25040 blocks B25570 when set
      inline constexpr std::size_t compound_holder = 0x9C;
      inline constexpr std::size_t compound_outer = 0x834;
      inline constexpr std::size_t render_register_key = 0x3C;
      inline constexpr std::size_t render_category_byte = 0x3C5;
      inline constexpr std::size_t region_copy_word = 0x7C;
      inline constexpr std::size_t camera_skip_flags = 0xD4;
      inline constexpr std::size_t region_word = 0xFC;
    } // namespace intro_object

    namespace functions {
      inline constexpr std::uint32_t set_child_process = 0x00D729E0;
      inline constexpr std::uint32_t resize_main_window = 0x0093F260;
      inline constexpr std::uint32_t set_version_active = 0x00949B80;
      inline constexpr std::uint32_t quit_process = 0x00D72440;
      inline constexpr std::uint32_t create_outer_wnd = 0x00D62410;
      inline constexpr std::uint32_t free_heap_block = 0x00404F70;
      inline constexpr std::uint32_t process_allocate = 0x00D6A1A0;
      inline constexpr std::uint32_t outer_interface_ctor = 0x00965CB0;
    } // namespace functions

    namespace net_state {
      inline constexpr int ready_for_textdata = 5;
    }

    namespace packets {
      inline constexpr std::uint16_t version_check_response = 0x0FF1;
      inline constexpr std::uint16_t version_check_notify = 0x0FF2;
      inline constexpr std::uint16_t login_handshake = 0x0FF3;
      inline constexpr std::uint16_t version_check_msgbox = 0x0FFC;
      inline constexpr std::uint16_t captcha_begin = 0x1002;
      inline constexpr std::uint16_t captcha_end = 0x1003;
      inline constexpr std::uint16_t gateway_sync = 0x1007;
    } // namespace packets

    namespace version_result {
      inline constexpr int success = 1;
      inline constexpr int show_message_box = 4092; // 0x0FFC
    } // namespace version_result

    namespace error_tags {
      inline constexpr int connect_failed = 5105;
      inline constexpr int content_fail = 5107;
    } // namespace error_tags
  } // namespace cps_version_check

  // ======================================================================
  // Consolidating custom-overridden cps_title.hpp
  // ======================================================================
  namespace cps_title {
    inline constexpr std::size_t size = 0x218;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x010344F4;
      inline constexpr std::size_t method_count = 41;

      inline constexpr std::size_t get_factory_entry = 0;
      inline constexpr std::size_t get_process_id = 1;
      inline constexpr std::size_t scalar_deleting_dtor = 2;
      inline constexpr std::size_t get_res_map = 3;
      inline constexpr std::size_t gwnd_helper_04 = 4;
      inline constexpr std::size_t on_timer = 5;
      inline constexpr std::size_t on_msg_recv = 6;
      inline constexpr std::size_t insert_child = 7;
      inline constexpr std::size_t remove_child = 8;
      inline constexpr std::size_t remove_child_by_id = 9;
      inline constexpr std::size_t on_create = 10;
      inline constexpr std::size_t on_active = 11;
      inline constexpr std::size_t on_update = 12;
      inline constexpr std::size_t on_render_begin = 13;
      inline constexpr std::size_t on_render = 14;
      inline constexpr std::size_t on_render_end = 15;
      inline constexpr std::size_t run_process = 16;
      inline constexpr std::size_t on_cmd = 17;
      inline constexpr std::size_t on_leave = 18;
      inline constexpr std::size_t on_lost_focus = 19;
      inline constexpr std::size_t on_got_focus = 20;
      inline constexpr std::size_t on_size = 21;
      inline constexpr std::size_t on_move = 22;
      inline constexpr std::size_t on_activate = 23;
      inline constexpr std::size_t on_deactivate = 24;
      inline constexpr std::size_t on_close = 25;
      inline constexpr std::size_t on_minimize = 26;
      inline constexpr std::size_t on_restore = 27;
      inline constexpr std::size_t on_maximize = 28;
      inline constexpr std::size_t gwnd_mouse_move = 29;
      inline constexpr std::size_t gwnd_mouse_down = 30;
      inline constexpr std::size_t gwnd_mouse_up = 31;
      inline constexpr std::size_t gwnd_mouse_wheel = 32;
      inline constexpr std::size_t gwnd_key_down = 33;
      inline constexpr std::size_t gwnd_key_up = 34;
      inline constexpr std::size_t gwnd_char = 35;
      inline constexpr std::size_t on_ime = 36;
      inline constexpr std::size_t null_37 = 37;
      inline constexpr std::size_t null_38 = 38;
      inline constexpr std::size_t on_sys_cmd = 39;
      inline constexpr std::size_t on_net_error = 40;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t child_process_list = 0x06C;
      inline constexpr std::size_t wnd_child_list = 0x07C;
      inline constexpr std::size_t net_state = 0x084;
      inline constexpr std::size_t load_thread = 0x0A0;
      inline constexpr std::size_t res_loader = 0x0E0;
      inline constexpr std::size_t login_phase = 0x0E4;
      inline constexpr std::size_t field_e8 = 0x0E8;
      inline constexpr std::size_t gfx_child = 0x0F0;
      inline constexpr std::size_t field_f4 = 0x0F4;
      inline constexpr std::size_t timer_value = 0x0F8;
      inline constexpr std::size_t field_fc = 0x0FC;
      inline constexpr std::size_t msg_list_head = 0x100;
      inline constexpr std::size_t msg_list_ptr = 0x104;
      inline constexpr std::size_t msg_list_count = 0x108;
      inline constexpr std::size_t res_ui_root = 0x0B0;
      inline constexpr std::size_t camera_controller = 0x0E0;
      inline constexpr std::size_t login_state = 0x0E4;
      inline constexpr std::size_t login_mode = 0x0E8;
      inline constexpr std::size_t channel_list_base = 0x10C;
      inline constexpr std::size_t channel_list_ptr = 0x110;
      inline constexpr std::size_t channel_list_count = 0x114;
      inline constexpr std::size_t channel_name = 0x11C;
      inline constexpr std::size_t channel_wstr_buf = 0x120;
      inline constexpr std::size_t channel_wstr_len = 0x130;
      inline constexpr std::size_t channel_wstr_cap = 0x134;
      inline constexpr std::size_t flag_312 = 0x138;
      inline constexpr std::size_t flag_313 = 0x139;
      inline constexpr std::size_t flag_314 = 0x13A;
      inline constexpr std::size_t login_timer = 0x13C;
      inline constexpr std::size_t fade_timer = 0x140;
      inline constexpr std::size_t anim_timer = 0x144;
      inline constexpr std::size_t server_browser = 0x148;
      inline constexpr std::size_t server_list_base = 0x184;
      inline constexpr std::size_t server_list_ptr = 0x188;
      inline constexpr std::size_t server_list_count = 0x18C;
      inline constexpr std::size_t server_anim_index = 0x190;
      inline constexpr std::size_t server_wstr_buf = 0x198;
      inline constexpr std::size_t server_wstr_len = 0x1A8;
      inline constexpr std::size_t server_wstr_cap = 0x1AC;
      inline constexpr std::size_t server_subobj = 0x1B0;
      inline constexpr std::size_t server_subobj_ptr = 0x1B4;
      inline constexpr std::size_t block_detail_wnd = 0x1BC;
      inline constexpr std::size_t block_detail_visible = 0x1C0;
      inline constexpr std::uint32_t captcha_active = 0x1C1;
      inline constexpr std::size_t unity_panel = 0x1C4;
      inline constexpr std::size_t event_list_base = 0x1DC;
      inline constexpr std::size_t event_list_ptr = 0x1E0;
      inline constexpr std::size_t event_list_count = 0x1E4;
      inline constexpr std::size_t unity_server_flag = 0x1E4;
      inline constexpr std::size_t promo_list_base = 0x1E8;
      inline constexpr std::size_t promo_list_ptr = 0x1EC;
      inline constexpr std::size_t promo_list_count = 0x1F0;
      inline constexpr std::size_t news_list_base = 0x1F4;
      inline constexpr std::size_t news_list_ptr = 0x1F8;
      inline constexpr std::size_t news_list_count = 0x1FC;
      inline constexpr std::size_t hide_server_list = 0x200;
      inline constexpr std::size_t field_204 = 0x204;
      inline constexpr std::size_t autologin_dialog = 0x210;
    } // namespace fields

    namespace globals {
      inline constexpr std::uint32_t factory_entry = 0x0117EBA0;
      inline constexpr std::uint32_t process_id = 0x0117EBAC;
      inline constexpr std::uint32_t channel_index = 0x0117E918;
      inline constexpr std::uint32_t active_instance = 0x0117E91C;
      inline constexpr std::uint32_t net_manager = 0x01420400;
      inline constexpr std::uint32_t process_manager = 0x013BAE00;
      inline constexpr std::uint32_t unity_flag = 0x0117E910;
    } // namespace globals

    namespace functions {
      inline constexpr std::uint32_t create_instance = 0x00976CE0;
      inline constexpr std::uint32_t ctor = 0x00976750;
      inline constexpr std::uint32_t dtor = 0x00976210;
      inline constexpr std::uint32_t delete_instance = 0x00967A60;
      inline constexpr std::uint32_t register_factory = 0x00FB3390;
      inline constexpr std::uint32_t on_create = 0x009777C0;
      inline constexpr std::uint32_t on_update = 0x00971D40;
      inline constexpr std::uint32_t on_msg_recv = 0x00974C40;
      inline constexpr std::uint32_t on_active = 0x00967EA0;
      inline constexpr std::uint32_t on_timer = 0x00972A00;
      inline constexpr std::uint32_t do_login = 0x0096A3A0;
      inline constexpr std::uint32_t show_login_form = 0x00971BC0;
      inline constexpr std::uint32_t get_ui_child = 0x009CF790;
      inline constexpr std::uint32_t set_child_process = 0x00D729E0;
      inline constexpr std::uint32_t update_server_ui = 0x0096CCC0;
      inline constexpr std::uint32_t update_selected_server_ui = 0x00970160;
    } // namespace functions

    namespace ui_ids {
      inline constexpr int screen_up = 1;
      inline constexpr int screen_down = 2;
      inline constexpr int border_screen_up = 3;
      inline constexpr int border_screen_down = 4;
      inline constexpr int title_text = 5;
      inline constexpr int big_logo = 7;
      inline constexpr int logo = 8;
      inline constexpr int login_edit = 9;
      inline constexpr int password_edit = 10;
      inline constexpr int login_button = 11;
      inline constexpr int exit_button = 12;
      inline constexpr int server_combo = 13;
      inline constexpr int channel_combo = 14;
      inline constexpr int channel_info = 15;
      inline constexpr int slider = 54;
      inline constexpr int channel_list_button = 46;
      inline constexpr int server_list_button = 44;
      inline constexpr int channel_list_popup = 60;
      inline constexpr int unity_server = 70;
      inline constexpr int msgbox = 500;
      inline constexpr int error_dialog = -1879048193;
    } // namespace ui_ids

    namespace login_phase {
      inline constexpr int idle = 0;
      inline constexpr int clicked = 2;
    } // namespace login_phase

    namespace packets {
      inline constexpr std::uint16_t auth_response = 0x0FF1;
      inline constexpr std::uint16_t auth_notify = 0x0FF2;
      inline constexpr std::uint16_t login_response = 0x0FF3;
      inline constexpr std::uint16_t login_msgbox = 0x0FFC;
      inline constexpr std::uint16_t captcha_begin = 0x1002;
      inline constexpr std::uint16_t captcha_end = 0x1003;
      inline constexpr std::uint16_t gateway_ping = 0x1007;
      inline constexpr std::uint16_t autologin_step_b = 0x100B;
      inline constexpr std::uint16_t autologin_step_c = 0x100C;
      inline constexpr std::uint16_t secondary_pw = 0x100E;
      inline constexpr std::uint16_t secondary_pw_ack = 0x100F;
      inline constexpr std::uint16_t server_list_request = 0x7007;
      inline constexpr std::uint16_t server_list_begin = 0xA106;
      inline constexpr std::uint16_t server_list_end = 0xA107;
    } // namespace packets

    namespace res {
      inline constexpr const char* layout_file = "resinfo\\pstitle.txt";
    }
  } // namespace cps_title

  // ======================================================================
  // Consolidating cps_quit.hpp
  // ======================================================================
  namespace cps_quit {
    namespace globals {
      inline constexpr std::uint32_t factory_entry = 0x0117E7DC;
    }
    namespace functions {
      inline constexpr std::uint32_t ctor = 0x0094E040;
      inline constexpr std::uint32_t create_instance = 0x0094E1C0;
      inline constexpr std::uint32_t on_create = 0x0094E080;
      inline constexpr std::uint32_t on_update = 0x0094E130;
      inline constexpr std::uint32_t register_factory = 0x00FB1E90;
    }
  } // namespace cps_quit

  // ======================================================================
  // Consolidating cps_mission.hpp
  // ======================================================================
  namespace cps_mission {
    inline constexpr std::size_t size = 0x170;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x0102F3B4; // ??_7CPSMission@@6B@
      inline constexpr std::size_t method_count = 41;
    } // namespace vtable

    namespace globals {
      inline constexpr std::uint32_t factory_entry = 0x0117E7BC;
      inline constexpr std::uint32_t process_id = 0x0117E7C8;
      inline constexpr std::uint32_t active_instance = 0x0117E7B8;
    } // namespace globals

    namespace functions {
      inline constexpr std::uint32_t create_instance = 0x0094DFB0;
      inline constexpr std::uint32_t ctor = 0x0094DDB0;
      inline constexpr std::uint32_t dtor = 0x0094DC10;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x0094DF90;
      inline constexpr std::uint32_t on_create = 0x0094D450;
    } // namespace functions

    namespace res {
      inline constexpr const char* layout_file = "resinfo\\psmission.txt";
    }
  } // namespace cps_mission

  // ======================================================================
  // Consolidating cgame_text.hpp
  // ======================================================================
  namespace cgame_text {
    namespace globals {
      inline constexpr std::uint32_t address = 0x0117EDA8;
    }
    namespace functions {
      inline constexpr std::uint32_t get_game_text = 0x009E4F00;
    }
  } // namespace cgame_text

  // ======================================================================
  // Consolidating cobj.hpp
  // ======================================================================
  namespace cobj {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FE2654; // ??_7cobj@@6B@
      inline constexpr std::size_t method_count = 3;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t region_begin = 0x000;
      inline constexpr std::size_t region_end = 0x00C; // cobj_child::field_0c
    } // namespace fields
  } // namespace cobj

  // ======================================================================
  // Consolidating cobj_child.hpp
  // ======================================================================
  namespace cobj_child {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FE2A54; // ??_7cobj_child@@6B@
      inline constexpr std::size_t method_count = 24;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t region_begin = 0x000;
      inline constexpr std::size_t field_0c = 0x00C;
      inline constexpr std::size_t list_node = 0x014;
      inline constexpr std::size_t region_end = 0x020; // CGWndBase slice
    } // namespace fields
  } // namespace cobj_child

  // ======================================================================
  // Consolidating cgwnd_base.hpp
  // ======================================================================
  namespace cgwnd_base {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x0106874C; // ??_7CGWndBase@@6B@
      inline constexpr std::size_t method_count = 25;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t region_begin = 0x020;
      inline constexpr std::size_t region_end = 0x02C; // CGWnd::control_id
    } // namespace fields
  } // namespace cgwnd_base

  // ======================================================================
  // Consolidating process_manager.hpp
  // ======================================================================
  namespace process_manager {
    inline constexpr std::uint32_t address = 0x013BAE00;

    namespace fields {
      inline constexpr std::size_t script_outer_parent = 0x024; // passed to CGWnd_CreateOuter for /script
      inline constexpr std::size_t active_child = 0x028; // m_pProcessActive
    }

    namespace functions {
      inline constexpr std::uint32_t quit_process = 0x00D72440;
      inline constexpr std::uint32_t set_child_process = 0x00D729E0;
    } // namespace functions
  } // namespace process_manager

  // ======================================================================
  // Consolidating cprocess.hpp
  // ======================================================================
  namespace cprocess {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x01068A2C; // ??_7CProcess@@6B@
      inline constexpr std::size_t method_count = 40;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t region_begin = 0x084;
      inline constexpr std::size_t net_state = 0x084;
      inline constexpr std::size_t thread_list_head = 0x088;
      inline constexpr std::size_t thread_list_size = 0x094;
      inline constexpr std::size_t msg_queue_head = 0x0A4;
      inline constexpr std::size_t msg_queue_size = 0x0AC;
      inline constexpr std::size_t region_end = 0x0B0;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x00D745E0;
      inline constexpr std::uint32_t dtor = 0x00D744B0;
    } // namespace functions
  } // namespace cprocess

  // ======================================================================
  // Consolidating cps_silkroad.hpp
  // ======================================================================
  namespace cps_silkroad {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x0102EFFC; // ??_7CPSilkroad@@6B@
      inline constexpr std::size_t method_count = 41;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t region_begin = 0x0B0;
      inline constexpr std::size_t res_ui_root = 0x0B0;
      inline constexpr std::size_t res_loader = 0x0E0;
      inline constexpr std::size_t login_phase = 0x0E4;
      inline constexpr std::size_t login_mode = 0x0E8;
      inline constexpr std::size_t region_end = 0x0F0;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x00949A60;
      inline constexpr std::uint32_t dtor = 0x00949AE0;
    } // namespace functions
  } // namespace cps_silkroad

  // ======================================================================
  // Consolidating cps_outer_interface.hpp
  // ======================================================================
  namespace cps_outer_interface {
    inline constexpr std::size_t derived_region_begin = 0x10C;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x01032E34; // ??_7CPSOuterInterface@@6B@
      inline constexpr std::size_t method_count = 41;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t child_process_list = 0x06C;
      inline constexpr std::size_t wnd_child_list = 0x07C;
      inline constexpr std::size_t net_state = 0x084;
      inline constexpr std::size_t load_thread = 0x0A0;
      inline constexpr std::size_t gfx_child = 0x0F0;
      inline constexpr std::size_t field_f4 = 0x0F4;
      inline constexpr std::size_t timer_value = 0x0F8;
      inline constexpr std::size_t field_fc = 0x0FC;
      inline constexpr std::size_t msg_list_head = 0x100;
      inline constexpr std::size_t msg_list_ptr = 0x104;
      inline constexpr std::size_t msg_list_count = 0x108;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x00965CB0;
      inline constexpr std::uint32_t dtor = 0x00965BC0;
    } // namespace functions
  } // namespace cps_outer_interface

  // ======================================================================
  // Consolidating custom-overridden cgwnd.hpp
  // ======================================================================
  namespace cgwnd {
    inline constexpr std::size_t size = 0x84;

    namespace vtable {
      inline constexpr std::uint32_t cgwnd = 0x0106850C;
      inline constexpr std::uint32_t cgwnd_base = 0x0106874C;
      inline constexpr std::size_t method_count = 38;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t region_begin = 0x02C;
      inline constexpr std::size_t control_id = 0x02C;
      inline constexpr std::size_t parent = 0x030;
      inline constexpr std::size_t unique_id = 0x034;
      inline constexpr std::size_t create_flags = unique_id;
      inline constexpr std::size_t initialized = 0x03C;
      inline constexpr std::size_t rect_x = 0x040;
      inline constexpr std::size_t rect_y = 0x044;
      inline constexpr std::size_t rect_w = 0x048;
      inline constexpr std::size_t rect_h = 0x04C;
      inline constexpr std::size_t visible = 0x061;
      inline constexpr std::size_t user_data = 0x068;
      inline constexpr std::size_t heap_res_descriptor = 0x06C;
      inline constexpr std::size_t child_list = 0x07C;
      inline constexpr std::size_t child_count = 0x080;
    } // namespace fields

    namespace globals {
      inline constexpr std::uint32_t manager = 0x013BAEE0;
      inline constexpr std::uint32_t version_label_res = 0x01179970;
      inline constexpr std::uint32_t client_config = 0x0117E3D8;
    } // namespace globals

    namespace client_config_fields {
      inline constexpr std::size_t selected_server = 0x4C;
      inline constexpr std::size_t data_version = 0x10C;
    } // namespace client_config_fields

    namespace screen_size {
      inline constexpr std::size_t width = 36;
      inline constexpr std::size_t height = 40;
    } // namespace screen_size

    namespace functions {
      inline constexpr std::uint32_t create_outer_wnd = 0x00D62410;
      inline constexpr std::uint32_t set_position = 0x00D6BEE0;
      inline constexpr std::uint32_t set_size = 0x00D6CBD0;
      inline constexpr std::uint32_t set_visible = 0x00D6A960;
      inline constexpr std::uint32_t set_text_color = 0x00B13630;
      inline constexpr std::uint32_t compare_res_type = 0x00D62670;
      inline constexpr std::uint32_t get_client_config = 0x004DE120;
      inline constexpr std::uint32_t get_screen_size = 0x004DE150;
      inline constexpr std::uint32_t set_anim = 0x00956180;
    } // namespace functions
  } // namespace cgwnd

  // ======================================================================
  // Consolidating custom-overridden cif_static.hpp
  // ======================================================================
  namespace cif_static {
    inline constexpr std::size_t size = 0x398;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FF406C;
      inline constexpr std::uint32_t secondary = 0x00FF4024;
      inline constexpr std::uint32_t loading_banner = 0x00FF46BC;
      inline constexpr std::size_t method_count = 53;

      inline constexpr std::size_t set_visible = 23;
      inline constexpr std::size_t update_layout = 42;
      inline constexpr std::size_t set_text = 45;
      inline constexpr std::size_t set_text_fmt = 49;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t font = 0x084;
      inline constexpr std::size_t align_h = 0x088;
      inline constexpr std::size_t align_v = 0x08C;
      inline constexpr std::size_t text_color_state = 0x090;
      inline constexpr std::size_t text_wstring = 0x1A4;
      inline constexpr std::size_t line_buffer = 0x0D8;
      inline constexpr std::size_t font_width = 0x0E4;
      inline constexpr std::size_t font_height = 0x0E6;
      inline constexpr std::size_t set_text_mode = 0x374;
      inline constexpr std::size_t text_flags = 0x378;
      inline constexpr std::size_t saved_rect_w = 0x37C;
      inline constexpr std::size_t text_bounds_x = 0x384;
      inline constexpr std::size_t text_bounds_y = 0x388;
      inline constexpr std::size_t text_bounds_w = 0x38C;
      inline constexpr std::size_t text_bounds_h = 0x390;
      inline constexpr std::size_t image_subobject = 0x084;
      inline constexpr std::size_t image_texture_path = 0x168;
    } // namespace fields

    namespace cif_image_renderer {
      namespace vtable {
        inline constexpr std::size_t set_texture = 13;
      }
      namespace fields {
        inline constexpr std::size_t texture = 0xE0;
        inline constexpr std::size_t path = 0xE4;
      } // namespace fields
    } // namespace cif_image_renderer

    namespace res {
      inline constexpr std::uint32_t version_label = cgwnd::globals::version_label_res;
    }

    namespace functions {
      inline constexpr std::uint32_t set_visible = 0x0072F4B0;
      inline constexpr std::uint32_t set_text = 0x00729570;
      inline constexpr std::uint32_t set_text_fmt = 0x007294D0;
      inline constexpr std::uint32_t set_texture = 0x007320B0;
    } // namespace functions

    inline constexpr std::size_t subobject_offset = 0x084;
  } // namespace cif_static

  // ======================================================================
  // Consolidating cif_wnd.hpp
  // ======================================================================
  namespace cif_wnd {
    inline constexpr std::size_t size = 0x394;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FF5910; // ??_7CIFWnd@@6B@ (CIFWnd::ctor)
    }

    namespace fields {
      inline constexpr std::size_t ui_res_map = 0x1C4; // sub_9D0190(this+113) in CIFWnd::ctor
      inline constexpr std::size_t derived_begin = 0x394;
    } // namespace fields
  } // namespace cif_wnd

  // ======================================================================
  // Consolidating cif_decorated_static.hpp
  // ======================================================================
  namespace cif_decorated_static {
    inline constexpr std::size_t size = 0x434;

    namespace fields {
      // sub_709210 path storage (DWORD index 261-262 region).
      inline constexpr std::size_t texture_path = 0x418;
      inline constexpr std::size_t texture_path_alt = 0x414;
      inline constexpr std::size_t inner_static = 0x084; // refresh uses *slot + 132
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t set_texture = 0x00709210; // CIFDecoratedStatic_SetTexture
    }

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FF01B4;   // ??_7CIFDecoratedStatic@@6B@
      inline constexpr std::uint32_t secondary = 0x00FF016C; // ??_7CIFDecoratedStatic@@6B@_0
    } // namespace vtable

    namespace res {
      inline constexpr std::uint32_t descriptor = 0x01179970; // dword_1179970
    }
  } // namespace cif_decorated_static

  // ======================================================================
  // Consolidating cif_facebook_link_alram.hpp
  // ======================================================================
  namespace cif_facebook_link_alram {
    inline constexpr std::size_t size = ext_client::offsets::cif_decorated_static::size; // 0x434

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FCCE2C;   // ??_7CIFFacebookLinkAlram@@6B@
      inline constexpr std::uint32_t secondary = 0x00FCCDE4; // ??_7CIFFacebookLinkAlram@@6B@_0
      inline constexpr std::size_t set_visible = 23;
      inline constexpr std::size_t on_wnd_message = 19;
      inline constexpr std::size_t open_link_dialog = 29;
    } // namespace vtable

    namespace fields {
      // CIFStatic MI subobject; *(this+0x84) is secondary vtable ptr.
      inline constexpr std::size_t static_subobject = ext_client::offsets::cif_static::subobject_offset;
      inline constexpr std::size_t decor_mode = 0x39C;     // sub_7086B0 stores a2 here
      inline constexpr std::size_t decor_timer_id = 0x398; // sub_708960: *(this+0xE6) dword
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x0055D9C0;            // CIFFacebookLinkAlram::CIFFacebookLinkAlram
      inline constexpr std::uint32_t create_instance = 0x0055DB30; // CProcess_Allocate(0x434)
      inline constexpr std::uint32_t on_create = 0x0055DE40;
      inline constexpr std::uint32_t on_wnd_message = 0x0055DCE0;
      inline constexpr std::uint32_t open_link_dialog = 0x0055DBC0;
      inline constexpr std::uint32_t decor_on_create = 0x00708A20; // CIFDecoratedStatic::OnCreate
      inline constexpr std::uint32_t set_decor_mode = 0x007086B0;  // *(this+0x39C)=a2
      inline constexpr std::uint32_t set_timer_id = 0x00708960;    // *(this+0x398)=a2
      inline constexpr std::uint32_t post_create_msg = 0x0072F670; // OR flags + send 0x400B
    } // namespace functions

    namespace res {
      inline constexpr std::uint32_t link_dialog = 0x011797F0; // unk_11797F0
    }

    namespace messages {
      inline constexpr int show_event = 0x4009;  // tooltip UIIT_STT_FACEBOOK_EVENT
      inline constexpr int open_link = 0x400A;   // open confirm dialog
      inline constexpr int post_create = 0x400B; // sent from on_create via sub_72F670
    } // namespace messages
  } // namespace cif_facebook_link_alram

  // ======================================================================
  // Consolidating cif_daily_login_alram.hpp
  // ======================================================================
  namespace cif_daily_login_alram {
    inline constexpr std::size_t size = ext_client::offsets::cif_decorated_static::size;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FCC7A4;   // ??_7CIFDailyLoginAlram@@6B@
      inline constexpr std::uint32_t secondary = 0x00FCC75C; // ??_7CIFDailyLoginAlram@@6B@_0
    } // namespace vtable

    namespace functions {
      inline constexpr std::uint32_t on_create = 0x0055CB70;
    }
  } // namespace cif_daily_login_alram

  // ======================================================================
  // Consolidating cif_magic_lamp_alram.hpp
  // ======================================================================
  namespace cif_magic_lamp_alram {
    inline constexpr std::size_t size = ext_client::offsets::cif_decorated_static::size;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FCD714;   // ??_7CIFMagicLampAlram@@6B@
      inline constexpr std::uint32_t secondary = 0x00FCD6CC; // ??_7CIFMagicLampAlram@@6B@_0
    } // namespace vtable

    namespace functions {
      inline constexpr std::uint32_t on_create = 0x005606E0;
      inline constexpr std::uint32_t on_wnd_message = 0x00560600;
    } // namespace functions
  } // namespace cif_magic_lamp_alram

  // ======================================================================
  // Consolidating calarm_entry.hpp
  // ======================================================================
  namespace calarm_entry {
    inline constexpr std::size_t size = 0x1F8;
    inline constexpr std::size_t stride = 0x1F8;

    namespace fields {
      inline constexpr std::size_t active = 0x028;
      inline constexpr std::size_t type = 0x098;
    } // namespace fields

    namespace types {
      inline constexpr int facebook = 6;
      inline constexpr int magic_lamp = 7;
      inline constexpr int daily_login = 8;
      // RIGID remaps strip promos (slot debug: 6=62, 7=46); still use sub_739A80 type>6 branch.
      inline constexpr int rigid_facebook = 62;
      inline constexpr int rigid_magic_lamp = 46;
    } // namespace types

    namespace functions {
      inline constexpr std::uint32_t get_type = 0x009D5C50;     // calarm_entry::type
      inline constexpr std::uint32_t ref_data_ptr = 0x009D5BF0; // calarm_entry::ref_data
    } // namespace functions
  } // namespace calarm_entry

  // ======================================================================
  // Consolidating calarm_ref_data.hpp
  // ======================================================================
  namespace calarm_ref_data {
    inline constexpr std::size_t icon_path = 0x15C; // std::string, used by sub_908BC0 / strip refresh
  }

  // ======================================================================
  // Consolidating calarm_data.hpp
  // ======================================================================
  namespace calarm_data {
    inline constexpr std::size_t entries_begin = 0x2450;

    namespace functions {
      inline constexpr std::uint32_t entry_at = 0x0078BAD0; // calarm_data::entry
    }
  } // namespace calarm_data

  // ======================================================================
  // Consolidating calarm_store.hpp
  // ======================================================================
  namespace calarm_store {
    namespace fields {
      inline constexpr std::size_t alarm_data = 0x7C8; // *(store + 498)
    }

    namespace functions {
      inline constexpr std::uint32_t from_interface = 0x008831F0; // CGInterface::alarm_store
      inline constexpr std::uint32_t alarm_data_ptr = 0x00781E50; // calarm_store::alarm_data
    } // namespace functions
  } // namespace calarm_store

  // ======================================================================
  // Consolidating calarm_guide_mgr_wnd.hpp
  // ======================================================================
  namespace calarm_guide_mgr_wnd {
    inline constexpr std::size_t size = 0x5CC;
    inline constexpr std::size_t slot_count = 8;
    // MSVC std::string is 24 bytes but alarm icon path banks are spaced 28 bytes apart.
    inline constexpr std::size_t icon_path_stride = 0x1C;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FF58C4; // ??_7CAlramGuideMgrWnd@@6B@
    }

    namespace guide_ids {
      inline constexpr int magic_lamp = 267;  // GUIDE_MagicLampAlram / case 0x10B in sub_7390B0
      inline constexpr int facebook = 278;    // GUIDE_FacebookLinkAlram / case 0x116 in sub_7390B0
      inline constexpr int daily_login = 280; // GUIDE_DailyLoginAlram / case 0x118 in sub_7390B0
      inline constexpr int macro = 261;       // macro strip button / case 0x105 in sub_7390B0
    } // namespace guide_ids

    namespace fields {
      inline constexpr std::size_t icon_paths_yellow = 0x394; // yellow path per slot (+ slot * stride)
      inline constexpr std::size_t icon_paths_red = 0x474;    // red/active path per slot (+ slot * stride)
      inline constexpr std::size_t guide_icon_count = 0x374;  // ++ on GetGuide create (sub_7390B0)
      inline constexpr std::size_t guide_list = 0x378;        // std::list<CGWnd*> head
      inline constexpr std::size_t guide_list_head = 0x37C;   // sentinel node ptr (_Myhead)
      inline constexpr std::size_t alarm_slots = 0x554;
      inline constexpr std::size_t effect_widgets = 0x578;    // m_effect_widgets[3]
      inline constexpr std::size_t alarm_status = 0x584;
      inline constexpr std::size_t slot_flags = 0x58C;
      inline constexpr std::size_t slot_extra_state = 0x5AC; // *(this+363+i) in sub_739A80
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t on_create = 0x00739BD0;
      inline constexpr std::uint32_t get_guide = 0x007390B0;        // CAlramGuideMgrWnd::GetGuide
      inline constexpr std::uint32_t remove_guide = 0x007393D0;     // list erase + destroy by UniqueID
      inline constexpr std::uint32_t update_positions = 0x00738DF0; // CAlramGuideMgrWnd::UpdatePositions
      inline constexpr std::uint32_t update_alarm_state = 0x00739A80;
      inline constexpr std::uint32_t refresh_slot_icons = 0x00739940;
      inline constexpr std::uint32_t set_effect_state = 0x007394A0;  // sub_7394A0 (1-based fx index)
      inline constexpr std::uint32_t show_promo_effect = 0x007394D0; // sub_7394D0 (slot 6/7 -> fx 2/3)
    } // namespace functions

    namespace res {
      inline constexpr std::uint32_t descriptor = 0x01179A58; // unk_1179A58
    }
  } // namespace calarm_guide_mgr_wnd

  // ======================================================================
  // Consolidating cg_interface.hpp
  // ======================================================================
  namespace cg_interface {
    // CGInterface::ctor sub_8748D0, dtor sub_873770 (via scalar dtor sub_875190).
    inline constexpr std::size_t size = 0x9C8;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x0101011C;   // ??_7CGInterface@@6B@
      inline constexpr std::uint32_t secondary = 0x010100D4; // ??_7CGInterface@@6B@_0 (GIGInterface @ +0x84)

      namespace primary {
        inline constexpr std::size_t scalar_deleting_dtor = 2;
        inline constexpr std::size_t on_cmd = 13;         // sub_875CC0
        inline constexpr std::size_t on_wnd_message = 17; // sub_871040
        inline constexpr std::size_t set_visible = 23;
      } // namespace primary
    } // namespace vtable

    // get_ui_child(res_map @ +0x374, id, add_base_key) — sub_9CF790
    namespace child_ids {
      inline constexpr int main_hud = 1;
      inline constexpr int alarm_store = 25;
      inline constexpr int alarm_guide_mgr = 0x9D;    // 157
      inline constexpr int guide_host = 0x9E;         // 158 — promo / guide container
      inline constexpr int alarm_guide_mgr_alt = 885; // cheap lookup alias (calarm mgr)
      // Top-level windows (IDA xrefs on CGInterface method cluster 0x870000..0x890000)
      inline constexpr int wnd_02 = 2;
      inline constexpr int wnd_03 = 3;
      inline constexpr int wnd_0b = 11;
      inline constexpr int wnd_0f = 15;
      inline constexpr int wnd_10 = 16;
      inline constexpr int wnd_13 = 19;
      inline constexpr int wnd_14 = 20;
      inline constexpr int wnd_16 = 22;
      inline constexpr int wnd_17 = 23;
      inline constexpr int wnd_18 = 24;
      inline constexpr int wnd_1a = 26;
      inline constexpr int wnd_1b = 27;
      inline constexpr int wnd_1c = 28;
      inline constexpr int wnd_1e = 30;
      inline constexpr int wnd_1f = 31;
      inline constexpr int wnd_20 = 32;
      inline constexpr int wnd_21 = 33;
      inline constexpr int wnd_22 = 34;
      inline constexpr int wnd_23 = 35;
      inline constexpr int wnd_24 = 36;
      inline constexpr int wnd_27 = 39;
      inline constexpr int wnd_28 = 40;
      inline constexpr int wnd_29 = 41;
      inline constexpr int wnd_2a = 42;
      inline constexpr int wnd_2b = 43;
      inline constexpr int wnd_2c = 44;
      inline constexpr int wnd_2d = 45;
      inline constexpr int wnd_2f = 47;
      inline constexpr int wnd_31 = 49;
      inline constexpr int wnd_32 = 50;
      inline constexpr int wnd_36 = 54;
      inline constexpr int wnd_37 = 55;
      inline constexpr int wnd_38 = 56;
      inline constexpr int wnd_39 = 57;
      inline constexpr int wnd_3a = 58;
      inline constexpr int wnd_3b = 59;
      inline constexpr int wnd_3c = 60;
      inline constexpr int wnd_3d = 61;
      inline constexpr int wnd_3e = 62;
      inline constexpr int wnd_3f = 63;
      inline constexpr int wnd_41 = 65;
      inline constexpr int wnd_42 = 66;
      inline constexpr int wnd_43 = 67;
      inline constexpr int wnd_44 = 68;
      inline constexpr int wnd_47 = 71;
      inline constexpr int wnd_48 = 72;
      inline constexpr int wnd_49 = 73;
      inline constexpr int wnd_4c = 76;
      inline constexpr int wnd_4d = 77;
      inline constexpr int wnd_4e = 78;
      inline constexpr int wnd_53 = 83;
      inline constexpr int wnd_61 = 97;
      inline constexpr int wnd_62 = 98;
      inline constexpr int wnd_63 = 99;
      inline constexpr int wnd_64 = 100;
      inline constexpr int wnd_65 = 101;
      inline constexpr int wnd_66 = 102;
      inline constexpr int wnd_68 = 104;
      inline constexpr int wnd_78 = 120;
      inline constexpr int wnd_79 = 121;
      inline constexpr int wnd_7a = 122;
      inline constexpr int wnd_7b = 123;
      inline constexpr int wnd_82 = 130;
      inline constexpr int wnd_83 = 131;
      inline constexpr int wnd_84 = 132;
      inline constexpr int wnd_85 = 133;
      inline constexpr int wnd_86 = 134;
      inline constexpr int wnd_87 = 135;
      inline constexpr int wnd_8c = 140;
      inline constexpr int wnd_91 = 145;
      inline constexpr int wnd_97 = 151;
      inline constexpr int wnd_99 = 153;
      inline constexpr int wnd_9a = 154;
      inline constexpr int wnd_9b = 155;
      inline constexpr int wnd_9c = 156;
      inline constexpr int wnd_a7 = 167;
      inline constexpr int wnd_10e = 270;
      inline constexpr int wnd_12c = 300;
      inline constexpr int wnd_12d = 301;
      inline constexpr int wnd_12e = 302;
      inline constexpr int wnd_12f = 303;
      inline constexpr int wnd_130 = 304;
      inline constexpr int wnd_13a = 314;
      inline constexpr int wnd_19a = 410;

      inline constexpr int known[] = {
        main_hud,
        wnd_02,
        wnd_03,
        wnd_0b,
        wnd_0f,
        wnd_10,
        wnd_13,
        wnd_14,
        wnd_16,
        wnd_17,
        wnd_18,
        alarm_store,
        wnd_1a,
        wnd_1b,
        wnd_1c,
        wnd_1e,
        wnd_1f,
        wnd_20,
        wnd_21,
        wnd_22,
        wnd_23,
        wnd_24,
        wnd_27,
        wnd_28,
        wnd_29,
        wnd_2a,
        wnd_2b,
        wnd_2c,
        wnd_2d,
        wnd_2f,
        wnd_31,
        wnd_32,
        wnd_36,
        wnd_37,
        wnd_38,
        wnd_39,
        wnd_3a,
        wnd_3b,
        wnd_3c,
        wnd_3d,
        wnd_3e,
        wnd_3f,
        wnd_41,
        wnd_42,
        wnd_43,
        wnd_44,
        wnd_47,
        wnd_48,
        wnd_49,
        wnd_4c,
        wnd_4d,
        wnd_4e,
        wnd_53,
        wnd_61,
        wnd_62,
        wnd_63,
        wnd_64,
        wnd_65,
        wnd_66,
        wnd_68,
        wnd_78,
        wnd_79,
        wnd_7a,
        wnd_7b,
        wnd_82,
        wnd_83,
        wnd_84,
        wnd_85,
        wnd_86,
        wnd_87,
        wnd_8c,
        wnd_91,
        wnd_97,
        wnd_99,
        wnd_9a,
        wnd_9b,
        wnd_9c,
        alarm_guide_mgr,
        guide_host,
        wnd_a7,
        wnd_10e,
        wnd_12c,
        wnd_12d,
        wnd_12e,
        wnd_12f,
        wnd_130,
        wnd_13a,
        wnd_19a,
        alarm_guide_mgr_alt,
      };
      inline constexpr std::size_t known_count = sizeof(known) / sizeof(known[0]);
    } // namespace child_ids

    // Guide IDs passed to sub_7390B0 / sub_7393D0 on child guide_host (0x9E).
    namespace guide_ids {
      inline constexpr unsigned facebook = 0x116;      // 278
      inline constexpr unsigned magic_lamp = 0x10B;    // 267
      inline constexpr unsigned daily_login = 0x118;   // 280
      inline constexpr unsigned web_item_alarm = 0xce; // 206
      inline constexpr unsigned macro = 0x105;         // 261
    } // namespace guide_ids

    namespace fields {
      inline constexpr std::size_t gig_interface_vftable = 0x084;
      inline constexpr std::size_t ui_res_map = 0x374; // dword 221 — NOT cif_wnd::ui_res_map (+0x1C4)
      inline constexpr std::size_t conquer_str_list = 0x468;
      inline constexpr std::size_t conquer_str_count = 0x46C;
      inline constexpr std::size_t vector_12x2_a = 0x480;
      inline constexpr std::size_t vector_12x2_b = 0x498;
      inline constexpr std::size_t hash_map_4b0_embed = 0x4B0;
      inline constexpr std::size_t hash_map_4b4_sentinel = 0x4B4;
      inline constexpr std::size_t hash_map_4b0_count = 0x4B8;
      inline constexpr std::size_t field_580 = 0x580;
      inline constexpr std::size_t field_610 = 0x610;
      inline constexpr std::size_t field_628 = 0x628;
      inline constexpr std::size_t field_630 = 0x630;
      inline constexpr std::size_t field_634 = 0x634;
      inline constexpr std::size_t hash_map_66c_embed = 0x66C;
      inline constexpr std::size_t hash_map_670_sentinel = 0x670;
      inline constexpr std::size_t hash_map_66c_count = 0x674;
      inline constexpr std::size_t hash_map_6a0_embed = 0x6A0;
      inline constexpr std::size_t hash_map_6a4_sentinel = 0x6A4;
      inline constexpr std::size_t hash_map_6a0_count = 0x6A8;
      inline constexpr std::size_t alarm_embed = 0x6B0;
      inline constexpr std::size_t alarm_embed_size = 0x68;
      inline constexpr std::size_t hash_map_718_embed = 0x718;
      inline constexpr std::size_t hash_map_71c_sentinel = 0x71C;
      inline constexpr std::size_t hash_map_718_count = 0x720;
      inline constexpr std::size_t field_730 = 0x730;
      inline constexpr std::size_t alarm_guide_mgr_popup = 0x738; // dword 0x1CE
      inline constexpr std::size_t field_74c = 0x74C;
      inline constexpr std::size_t embed_754 = 0x754;
      inline constexpr std::size_t embed_754_size = 0x3C;
      inline constexpr std::size_t popup_widget_790 = 0x790;
      inline constexpr std::size_t popup_widget_79c = 0x79C;
      inline constexpr std::size_t hash_map_7a0_embed = 0x7A0;
      inline constexpr std::size_t hash_map_7a4_sentinel = 0x7A4;
      inline constexpr std::size_t hash_map_7a0_count = 0x7A8;
      inline constexpr std::size_t field_7d0 = 0x7D0;
      inline constexpr std::size_t field_7d4 = 0x7D4;
      inline constexpr std::size_t field_7d6 = 0x7D6;
      inline constexpr std::size_t hash_map_7d8_embed = 0x7D8;
      inline constexpr std::size_t hash_map_7dc_sentinel = 0x7DC;
      inline constexpr std::size_t hash_map_7d8_count = 0x7E0;
      inline constexpr std::size_t modal_wnd = 0x808;
      inline constexpr std::size_t modal_mode = 0x80C;
      inline constexpr std::size_t field_810_widget = 0x810;
      inline constexpr std::size_t hash_map_814_embed = 0x814;
      inline constexpr std::size_t hash_map_818_sentinel = 0x818;
      inline constexpr std::size_t hash_map_818_count = 0x81C;
      inline constexpr std::size_t hash_map_824_embed = 0x824;
      inline constexpr std::size_t hash_map_828_sentinel = 0x828;
      inline constexpr std::size_t hash_map_824_count = 0x82C;
      inline constexpr std::size_t field_834 = 0x834;
      inline constexpr std::size_t mouse_drag_flag_a = 0x854;
      inline constexpr std::size_t mouse_drag_flag_b = 0x855;
      inline constexpr std::size_t mouse_drag_origin = 0x858;
      inline constexpr std::size_t mouse_drag_origin_y = 0x85C;
      inline constexpr std::size_t hash_map_870_embed = 0x870;
      inline constexpr std::size_t hash_map_874_sentinel = 0x874;
      inline constexpr std::size_t hash_map_870_count = 0x878;
      inline constexpr std::size_t float_a = 0x894;
      inline constexpr std::size_t float_b = 0x898;
      inline constexpr std::size_t field_89c = 0x89C;
      inline constexpr std::size_t field_934 = 0x934;
      inline constexpr std::size_t field_96c = 0x96C;
      inline constexpr std::size_t hash_map_980_embed = 0x980;
      inline constexpr std::size_t hash_map_984_sentinel = 0x984;
      inline constexpr std::size_t hash_map_980_count = 0x988;

      // Legacy alias kept for calarm_store (this + 221 dwords).
      inline constexpr std::size_t alarm_store_res_map = ui_res_map;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x008748D0;
      inline constexpr std::uint32_t dtor = 0x00873770;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x00875190;
      inline constexpr std::uint32_t on_cmd = 0x00875CC0;
      inline constexpr std::uint32_t on_wnd_message = 0x00871040;
      inline constexpr std::uint32_t alarm_store_child = 0x008831F0;         // get_ui_child(25)
      inline constexpr std::uint32_t alarm_guide_mgr_child = 0x008840C0;     // get_ui_child(0x9D)
      inline constexpr std::uint32_t show_magic_lamp_guide = 0x008844C0;     // guide_host + guide_ids::magic_lamp
      inline constexpr std::uint32_t show_daily_login_guide = 0x008844F0;    // guide_host + guide_ids::daily_login
      inline constexpr std::uint32_t show_facebook_guide = 0x00884720;       // guide_host + guide_ids::facebook
      inline constexpr std::uint32_t show_web_item_alarm_guide = 0x00883500; // guide_host + guide_ids::web_item_alarm
      inline constexpr std::uint32_t show_macro_guide = 0x00884170;          // guide_host + guide_ids::macro
    } // namespace functions
  } // namespace cg_interface

  // ======================================================================
  // Consolidating ingame_ui_map.hpp
  // ======================================================================
  namespace ingame_ui_map {
    namespace globals {
      inline constexpr std::uint32_t address = 0x01420408; // dword_1420408
    }

    namespace res_ids {
      inline constexpr int sro_ingame_info = 0x34;  // sub_68DD00 → sub_401340 / sub_4016F0
      inline constexpr int sro_ingame_start = 0x35; // sub_68DD00 → sub_4016F0; show when +0x7A0==1
    } // namespace res_ids

    namespace functions {
      inline constexpr std::uint32_t find = 0x004016F0; // sub_4016F0(char* map, int key)
      inline constexpr std::uint32_t prep = 0x00401340; // sub_401340 — loads panel; avoid on hide-restore
    } // namespace functions
  } // namespace ingame_ui_map

  // ======================================================================
  // Consolidating cnif_sro_ingame_start.hpp
  // ======================================================================
  namespace cnif_sro_ingame_start {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FE9514;   // ??_7CNIFSroInGameStart@@6B@
      inline constexpr std::uint32_t secondary = 0x00FE94CC; // ??_7CNIFSroInGameStart@@6B@_0
    } // namespace vtable

    namespace windows {
      inline constexpr unsigned survey = 0x7586;
    }

    // Aliases — always use ingame_ui_map::res_ids as source of truth.
    namespace res_ids {
      inline constexpr int start_panel = ingame_ui_map::res_ids::sro_ingame_start;
      inline constexpr int info_panel = ingame_ui_map::res_ids::sro_ingame_info;
    } // namespace res_ids

    namespace fields {
      inline constexpr std::size_t show_survey = 0x7A0; // ctor 0; sub_68DD00 shows start_panel when ==1
    }

    // CreateOuter / UniqueID on survey children (NIFSroInGame res layout).
    namespace child_ids {
      inline constexpr int survey_button = 3; // JOIN_SURVEY label on CNIFSroInGameStart
    }

    namespace functions {
      inline constexpr std::uint32_t get_child_by_unique_id = 0x00407E50; // CGWnd child list walk
      inline constexpr std::uint32_t ui_res_map_find = ingame_ui_map::functions::find;
      inline constexpr std::uint32_t set_visible_impl = 0x00407550;       // sub_407550 — CNIFSroInGameStart::SetVisible
      inline constexpr std::uint32_t try_show_panel = 0x0068DD00;         // prep 0x34; show 0x35 if +0x7A0==1
      inline constexpr std::uint32_t arm_show_survey = 0x0068DE90;        // CNIFSroInGameStart: sets +0x7A0=1
      inline constexpr std::uint32_t on_create = 0x0068E320;              // sets JOIN_SURVEY label + shows button
      inline constexpr std::uint32_t onclick = 0x0068E1D0;                // opens CNIFSroInGameInfo window
      inline constexpr std::uint32_t apply_layout = 0x0068DEE0;           // positions near bottom center
    } // namespace functions

    namespace globals {
      inline constexpr std::uint32_t ui_res_map = ingame_ui_map::globals::address;
    }
  } // namespace cnif_sro_ingame_start

  // ======================================================================
  // Consolidating cnif_sro_ingame_info.hpp
  // ======================================================================
  namespace cnif_sro_ingame_info {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FE93FC;   // ??_7CNIFSroInGameInfo@@6B@
      inline constexpr std::uint32_t secondary = 0x00FE93B4; // ??_7CNIFSroInGameInfo@@6B@_0
    } // namespace vtable

    namespace child_ids {
      inline constexpr int survey_icon = 7; // floating icon; enabled in sub_68E6C0 when visible
    }

    namespace functions {
      inline constexpr std::uint32_t set_visible = 0x0068E6C0; // child 7 + sub_407550
    }
  } // namespace cnif_sro_ingame_info

  // ======================================================================
  // Consolidating cnif_button.hpp
  // ======================================================================
  namespace cnif_button {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x010834B4;   // ??_7CNIFButton@@6B@
      inline constexpr std::uint32_t secondary = 0x01083584; // ??_7CNIFButton@@6B@_0
    } // namespace vtable

    namespace functions {
      inline constexpr std::uint32_t set_visible = 0x0040F230; // CNIFButton::SetVisible
    }
  } // namespace cnif_button

  // ======================================================================
  // Consolidating cif_button.hpp
  // ======================================================================
  namespace cif_button {
    inline constexpr std::size_t size = 0x3E8;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FF48D4; // ??_7CIFButton@@6B@
      inline constexpr std::uint32_t secondary = 0x00FF488C; // ??_7CIFButton@@6B@_0
      inline constexpr std::size_t method_count = 56;
    } // namespace vtable

    namespace functions {
      inline constexpr std::uint32_t create_instance = 0x00733190;
      inline constexpr std::uint32_t ctor = 0x00732F10;
      inline constexpr std::uint32_t set_visible = 0x00732AA0; // forwards to CIFStatic_SetVisible
    } // namespace functions
  } // namespace cif_button

  // ======================================================================
  // Consolidating cif_edit.hpp
  // ======================================================================
  namespace cif_edit {
    inline constexpr std::size_t size = 0xB4B0;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FF042C; // ??_7CIFEdit@@6B@
      inline constexpr std::uint32_t secondary = 0x00FF03E4; // ??_7CIFEdit@@6B@_0
      inline constexpr std::size_t method_count = 54;

      inline constexpr std::size_t set_text = 45;
    } // namespace vtable

    namespace functions {
      inline constexpr std::uint32_t create_instance = 0x0070AD10;
      inline constexpr std::uint32_t ctor = 0x0070AB70;
      inline constexpr std::uint32_t set_text = 0x0070ADB0;
    } // namespace functions
  } // namespace cif_edit

  // ======================================================================
  // Consolidating msvc9_stl.hpp
  // ======================================================================
  namespace msvc9_stl {
    namespace functions {
      inline constexpr std::uint32_t wstring_assign = 0x004093A0;    // basic_string<wchar_t>::assign
      inline constexpr std::uint32_t wstring_resize = 0x00409BE0;
      inline constexpr std::uint32_t string_assign = 0x0040B940;     // basic_string<char>::assign
      inline constexpr std::uint32_t string_clear = 0x00405EA0;
      inline constexpr std::uint32_t heap_alloc = 0x0040A1D0;        // game wchar heap helper
      inline constexpr std::uint32_t heap_alloc_raw = 0x0040A040;    // generic malloc wrapper
      inline constexpr std::uint32_t heap_free = 0x00404F70;
      inline constexpr std::uint32_t ui_res_map_ctor = 0x009D0190;   // CResIDManager ctor; sentinel @ map+4
      inline constexpr std::uint32_t ui_res_map_find = 0x009CF790;   // get_ui_child; returns node[4]
      inline constexpr std::uint32_t map_lower_bound = 0x004F8230;   // key=node[3], isnil=byte21
      inline constexpr std::uint32_t child_list_alloc = 0x00864310;  // CGWndBase empty list head
      inline constexpr std::uint32_t child_list_insert = 0x00409090; // list node {prev,next,widget}
      inline constexpr std::uint32_t map_insert_node = 0x004F5FF0;
      inline constexpr std::uint32_t map_tree_clear = 0x0096A900;
      inline constexpr std::uint32_t map_tree_node_alloc = 0x004EFD50;
    } // namespace functions
  } // namespace msvc9_stl

  // ======================================================================
  // Consolidating ui_res_document.hpp
  // ======================================================================
  namespace ui_res_document {
    namespace res_ui_manager {
      inline constexpr std::size_t parsed_document = 0x0C; // CResIDManager / CPS+0xB0
    }

    // stdext::hash_map — canonical layout in ext_client::msvc9::stdext_hash_map (sub_925B00 / sub_9CF8B0).
    namespace hash_map {
      inline constexpr std::size_t list = 0x04;
      inline constexpr std::size_t bucket_begin = 0x14;
      inline constexpr std::size_t bucket_end = 0x18;
      inline constexpr std::size_t element_count = 0x24;
      inline constexpr std::size_t node_name = 0x08;    // pair.first (basic_string<char>)
      inline constexpr std::size_t node_section = 0x24; // pair.second (Section*)
    } // namespace hash_map

    namespace section {
      inline constexpr std::size_t child_list = 0x04;
    }

    namespace list_node {
      inline constexpr std::size_t next = 0x00; // sub_9CFB30: *node
      inline constexpr std::size_t descriptor = 0x08;
    } // namespace list_node

    namespace descriptor {
      inline constexpr std::size_t size = 0x120;
      inline constexpr std::size_t type_wstr = 0x04;
      inline constexpr std::size_t res_id = 0x1C;
      inline constexpr std::size_t rect_x = 0x48;
      inline constexpr std::size_t rect_y = 0x4C;
      inline constexpr std::size_t rect_w = 0x50;
      inline constexpr std::size_t rect_h = 0x54;
      inline constexpr std::size_t ddj_vector = 0x68;
    } // namespace descriptor

    namespace string_vector {
      inline constexpr std::size_t begin = 0x64; // sub_D702A0: *(this + 0x19)
      inline constexpr std::size_t end = 0x68;   // *(this + 0x1A)
    } // namespace string_vector
  } // namespace ui_res_document

  // ======================================================================
  // Consolidating cmsg.hpp
  // ======================================================================
  namespace cmsg {
    namespace fields {
      inline constexpr std::size_t data_ptr = 0x1034;
      inline constexpr std::size_t read_cursor = 0x103C;
      inline constexpr std::size_t write_cursor = 0x103E;
      inline constexpr std::size_t ref_count = 0x1044;
      inline constexpr std::size_t capacity = 0x104C;
      inline constexpr std::size_t opcode_ptr = 0x1050;
      inline constexpr std::size_t size_ptr = 0x1054;
      inline constexpr std::size_t sec_count_ptr = 0x1058;
      inline constexpr std::size_t crc_ptr = 0x105C;
      inline constexpr std::size_t recv_handler = 0x1060;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t write_payload = 0x00462470;
      inline constexpr std::uint32_t dispatch_handler = 0x00DA4B30; // jmp [session+0x22C]; hook here for CMsg S->C
      inline constexpr std::uint32_t on_msg_received = 0x00DA7220;  // handshake handler (session+0x22C before game)
      inline constexpr std::uint32_t recv_assemble = 0x00DA6070;
      inline constexpr std::uint32_t stream_to_wire = 0x00941400;
      inline constexpr std::uint32_t send_from_buffer = 0x00941600; // SendMsg(stream -> CMsg -> wire)
    } // namespace functions
  } // namespace cmsg

  // ======================================================================
  // Consolidating cmsg_stream_buffer.hpp
  // ======================================================================
  namespace cmsg_stream_buffer {
    inline constexpr std::size_t object_size = 0x1C;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x00FC0DC8; // ??_7CMsgStreamBuffer@@6B@
      inline constexpr std::size_t method_count = 1;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t read_cursor = 0x004;
      inline constexpr std::size_t total_bytes = 0x008;
      inline constexpr std::size_t mode_flag = 0x00C;
      inline constexpr std::size_t node_primary = 0x010;
      inline constexpr std::size_t node_active = 0x014;
      inline constexpr std::size_t msg_id = 0x018;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x0049AFB0;
      inline constexpr std::uint32_t dtor = 0x0049B3B0;
      inline constexpr std::uint32_t read_bytes = 0x00499710;
      inline constexpr std::uint32_t write_bytes = 0x0049B010;
      inline constexpr std::uint32_t from_wire = 0x0092F470;
      inline constexpr std::uint32_t release = 0x0092F0A0;
      inline constexpr std::uint32_t recv_pump = 0x00930B80;
      inline constexpr std::uint32_t dispatch_to_process = 0x00D73DC0;
    } // namespace functions

    namespace globals {
      inline constexpr std::uint32_t net_manager = 0x01420400;
      inline constexpr std::uint32_t send_queue = 0x0117DCA8;
      inline constexpr std::uint32_t defer_send_flag = 0x0117DB80; // byte_117DB80
    } // namespace globals
  } // namespace cmsg_stream_buffer

  // ======================================================================
  // Consolidating cgobj.hpp
  // ======================================================================
  namespace cgobj {
    namespace vtable {
      inline constexpr std::uint32_t address = 0x01068ADC; // ??_7cgobj@@6B@
      inline constexpr std::size_t method_count = 7;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t region_end = 0x020; // cobj_child prefix
    }

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x00D74760;
      inline constexpr std::uint32_t base_dtor = 0x00D74780;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x00D74820;
      inline constexpr std::uint32_t get_factory_entry = 0x006CD5D0;
      inline constexpr std::uint32_t get_process_id = 0x006CD5E0;
    } // namespace functions
  } // namespace cgobj

  // ======================================================================
  // Consolidating cnet_process.hpp
  // ======================================================================
  namespace cnet_process {
    namespace fields {
      inline constexpr std::size_t handler_map = 0x024;
      inline constexpr std::size_t handler_map_size = 0x024; // bytes before scratch (phase types)
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t get_type_info = 0x00D5A2A0;
      inline constexpr std::uint32_t add_ref = 0x0040A600;
      inline constexpr std::uint32_t release = 0x0040A5F0;
      inline constexpr std::uint32_t query_interface = 0x0040A5E0;
      inline constexpr std::uint32_t dispatch_to_process = 0x00D73DC0;
    } // namespace functions
  } // namespace cnet_process

  // ======================================================================
  // Consolidating cnet_process_third.hpp
  // ======================================================================
  namespace cnet_process_third {
    inline constexpr std::size_t size = 0x04C;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x0103A544; // ??_7CNetProcessThird@@6B@
      inline constexpr std::size_t method_count = 7;

      inline constexpr std::size_t get_factory_entry = 0;
      inline constexpr std::size_t get_process_id = 1;
      inline constexpr std::size_t scalar_deleting_dtor = 2;
      inline constexpr std::size_t get_type_info = 3;
      inline constexpr std::size_t add_ref = 4;
      inline constexpr std::size_t release = 5;
      inline constexpr std::size_t dispatch_msg = 6;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t handler_map = cnet_process::fields::handler_map;
      inline constexpr std::size_t scratch_msg = 0x048;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x009C02F0;
      inline constexpr std::uint32_t dtor = 0x009AFF10;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x009B5E30;
      inline constexpr std::uint32_t dispatch_msg = 0x009BB950;
      inline constexpr std::uint32_t lookup_opcode = 0x009BB160;
      inline constexpr std::uint32_t register_handlers = 0x009BF570;
      inline constexpr std::uint32_t get_factory_entry = 0x00632240;
      inline constexpr std::uint32_t get_process_id = 0x00632250;
    } // namespace functions
  } // namespace cnet_process_third

  // ======================================================================
  // Consolidating cnet_process_second.hpp
  // ======================================================================
  namespace cnet_process_second {
    inline constexpr std::size_t size = 0x04C;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x010393DC; // ??_7CNetProcessSecond@@6B@
      inline constexpr std::size_t method_count = 7;

      inline constexpr std::size_t get_factory_entry = 0;
      inline constexpr std::size_t get_process_id = 1;
      inline constexpr std::size_t scalar_deleting_dtor = 2;
      inline constexpr std::size_t get_type_info = 3;
      inline constexpr std::size_t add_ref = 4;
      inline constexpr std::size_t release = 5;
      inline constexpr std::size_t dispatch_msg = 6;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t handler_map = cnet_process::fields::handler_map;
      inline constexpr std::size_t scratch_msg = 0x048;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x009AAA60;
      inline constexpr std::uint32_t dtor = 0x009A1F10;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x009A5100;
      inline constexpr std::uint32_t dispatch_msg = 0x009A6B50;
      inline constexpr std::uint32_t lookup_opcode = 0x009A6480;
      inline constexpr std::uint32_t get_factory_entry = 0x00632240;
      inline constexpr std::uint32_t get_process_id = 0x00632250;
    } // namespace functions
  } // namespace cnet_process_second

  // ======================================================================
  // Consolidating cnet_process_in.hpp
  // ======================================================================
  namespace cnet_process_in {
    inline constexpr std::size_t size = 0x080;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x01034B98; // ??_7CNetProcessIn@@6B@ (7 slots + RTTI)
      inline constexpr std::size_t method_count = 7;

      inline constexpr std::size_t get_factory_entry = 0;
      inline constexpr std::size_t get_process_id = 1;
      inline constexpr std::size_t scalar_deleting_dtor = 2;
      inline constexpr std::size_t get_type_info = 3;
      inline constexpr std::size_t add_ref = 4;
      inline constexpr std::size_t release = 5;
      inline constexpr std::size_t query_interface = 6;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t handler_map = 0x024;
      inline constexpr std::size_t handler_map_size = 0x024;
      inline constexpr std::size_t handler_registry = 0x048;
      inline constexpr std::size_t handler_node = 0x04C;
      inline constexpr std::size_t scratch_msg = 0x054;
      inline constexpr std::size_t name_flag = 0x05C;
      inline constexpr std::size_t secondary_obj = 0x078;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x0097D9B0;
      inline constexpr std::uint32_t dtor = 0x0097AC10;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x0097AFA0;
      inline constexpr std::uint32_t get_factory_entry = 0x006CD5D0;
      inline constexpr std::uint32_t get_process_id = 0x006CD5E0;
    } // namespace functions
  } // namespace cnet_process_in

  // ======================================================================
  // Consolidating cnet_engine.hpp
  // ======================================================================
  namespace cnet_engine {
    inline constexpr std::size_t size = 0x4E0;

    namespace vtable {
      inline constexpr std::uint32_t address = 0x0106A72C; // ??_7CNetEngine@@6B@
      inline constexpr std::size_t method_count = 42;

      inline constexpr std::size_t query_interface = 0;
      inline constexpr std::size_t add_ref = 1;
      inline constexpr std::size_t release = 2;
      inline constexpr std::size_t initialize = 3;
      inline constexpr std::size_t create_passive_socket = 4;
      inline constexpr std::size_t connect = 5;
      inline constexpr std::size_t bind = 6;
      inline constexpr std::size_t get_host_by_name = 7;
      inline constexpr std::size_t create_active_socket = 8;
      inline constexpr std::size_t listen = 9;
      inline constexpr std::size_t accept = 10;
      inline constexpr std::size_t close_socket = 13;
      inline constexpr std::size_t get_local_address = 15;
      inline constexpr std::size_t send = 17;
      inline constexpr std::size_t alloc_msg_pool = 18;
      inline constexpr std::size_t free_msg_pool = 19;
      inline constexpr std::size_t send_pool_buffer = 20;
      inline constexpr std::size_t alloc_msg = 21;
      inline constexpr std::size_t free_msg = 22;
      inline constexpr std::size_t connect_ex = 28;
      inline constexpr std::size_t set_socket_opt = 35;
      inline constexpr std::size_t scalar_deleting_dtor = 40;
      inline constexpr std::size_t is_connected = 41;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t ref_count = 0x004;
      inline constexpr std::size_t com_ptr = 0x194;
      inline constexpr std::size_t socket_manager = 0x4D0;
      inline constexpr std::size_t socket_manager_tail = 0x4D4;
    } // namespace fields

    namespace functions {
      inline constexpr std::uint32_t ctor = 0x00D95560;
      inline constexpr std::uint32_t dtor = 0x00D95370;
      inline constexpr std::uint32_t scalar_deleting_dtor = 0x00D956B0;
      inline constexpr std::uint32_t query_interface = 0x00D8A700;
      inline constexpr std::uint32_t connect = 0x00D8A790;
      inline constexpr std::uint32_t create_passive_socket = 0x00D95850;
      inline constexpr std::uint32_t create_active_socket = 0x00D94B70;
      inline constexpr std::uint32_t send = 0x00D90EC0;
      inline constexpr std::uint32_t send_pool_buffer = 0x00D90F20; // vtable+0x50; all IBSMsg pool -> wire
      inline constexpr std::uint32_t alloc_msg_pool = 0x00D95910;   // vtable+0x48; simple CMsg pool alloc
      inline constexpr std::uint32_t free_msg_pool = 0x00D93F80;    // vtable+0x4C
      inline constexpr std::uint32_t alloc_msg = 0x00D90FB0;
      inline constexpr std::uint32_t free_msg = 0x00D910C0;
    } // namespace functions

    namespace globals {
      inline constexpr std::uint32_t instance = 0x013BEAB4;
    }
  } // namespace cnet_engine

  // ======================================================================
  // Consolidating custom-overridden cif_notify.hpp
  // ======================================================================
  namespace cif_notify {
    inline constexpr std::size_t size = 0x7E0;

    namespace fields {
      inline constexpr std::size_t static_text = 0x7CC;
      inline constexpr std::size_t duration = 0x7D0;
      inline constexpr std::size_t is_active = 0x7D4;
      inline constexpr std::size_t y_position = 0x7DC;
    } // namespace fields

    namespace vtable {
      inline constexpr std::size_t set_visible = 23;
    }

    namespace functions {
      inline constexpr std::uint32_t set_bg_color = 0x008A0BA0;
      inline constexpr std::uint32_t show_message = 0x008A0D40;
      inline constexpr std::uint32_t show_notice = 0x0085D7B0;
    } // namespace functions
  } // namespace cif_notify

  // ======================================================================
  // SWorld SDK offsets
  // ======================================================================
  namespace sworld {
    inline constexpr std::size_t size = 0x7D70;

    namespace fields {
      inline constexpr std::size_t cell_limit = 0x23DB;
      inline constexpr std::size_t sight_range = 0x2782;
      inline constexpr std::size_t options = 0x2F20;
      inline constexpr std::size_t options_count = 51;
    } // namespace fields

    namespace globals {
      inline constexpr std::uint32_t address = 0x013AB480;
      inline constexpr std::uint32_t details_fade_distance = 0x001155260;
    }

    namespace functions {
      inline constexpr std::uint32_t set_graphics_option = 0x00B841E0;
    }
  } // namespace sworld

  // ======================================================================
  // Quest offsets
  // ======================================================================
  namespace quest {
    namespace functions {
      inline constexpr std::uint32_t get_quest_definition = 0x00A75820;
    } // namespace functions
  } // namespace quest

  // ======================================================================
  // BSLib offsets
  // ======================================================================
  namespace bslib {
    namespace functions {
      inline constexpr std::uint32_t assert_report = 0x00D898A0;
    } // namespace functions
  } // namespace bslib

  // ======================================================================
  // ci_charactor SDK offsets
  // ======================================================================
  namespace ci_charactor {
    namespace fields {
      inline constexpr std::size_t compound_obj = 0x004;
      inline constexpr std::size_t position = 0x07C;
      inline constexpr std::size_t class_link = 0x378;
      inline constexpr std::size_t hp = 0x554;
      inline constexpr std::size_t mp = 0x558;
      inline constexpr std::size_t max_hp = 0x55C;
      inline constexpr std::size_t max_mp = 0x560;
      inline constexpr std::size_t state_619 = 0x619;
      inline constexpr std::size_t idle_timer = 0x61C;
      inline constexpr std::size_t state_mask = 0x768;
    } // namespace fields

    namespace vtable {
      inline constexpr std::size_t set_hp = 42;
      inline constexpr std::size_t set_mp = 43;
      inline constexpr std::size_t set_max_hp = 46;
      inline constexpr std::size_t set_max_mp = 47;
      inline constexpr std::size_t get_state_619 = 50;
      inline constexpr std::size_t set_state_619 = 51;
    } // namespace vtable

    namespace functions {
      inline constexpr std::uint32_t play_emote = 0x00B2A9B0;
    } // namespace functions
  } // namespace ci_charactor

  // ======================================================================
  // cic_player SDK offsets
  // ======================================================================
  namespace cic_player {
    namespace fields {
      inline constexpr std::size_t name = 0x8CC;
      inline constexpr std::size_t guild_name = 0x8FC;
      inline constexpr std::size_t level = 0xA14;
      inline constexpr std::size_t exp = 0xA18;
      inline constexpr std::size_t sp = 0xA20;
      inline constexpr std::size_t strength = 0xA24;
      inline constexpr std::size_t intelligence = 0xA26;
      inline constexpr std::size_t attribute_points = 0xA28;
      inline constexpr std::size_t gamemaster = 0x3A58;
    } // namespace fields

    namespace globals {
      inline constexpr std::uint32_t local_player = 0x01199114;
      inline constexpr std::uint32_t gold = 0x0119B610;
    } // namespace globals
  } // namespace cic_player

  // ======================================================================
  // CCompoundObj SDK offsets
  // ======================================================================
  namespace ccompoundobj {
    namespace vtable {
      inline constexpr std::size_t set_alpha = 2;
      inline constexpr std::size_t set_world_matrix = 3;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t name = 0x054;
      inline constexpr std::size_t alpha = 0x07C;
      inline constexpr std::size_t alpha_mode = 0x080;
      inline constexpr std::size_t forward_vector = 0x0A0;
      inline constexpr std::size_t world_matrix = 0x0AC;
      inline constexpr std::size_t path = 0x0D4;
    } // namespace fields
  } // namespace ccompoundobj

  // ======================================================================
  // cic_script_obj SDK offsets
  // ======================================================================
  namespace cic_script_obj {
    namespace vtable {
      inline constexpr std::size_t play_animation = 54;
    } // namespace vtable

    namespace functions {
      inline constexpr std::uint32_t get_anim_id = 0x00B493D0;
    } // namespace functions
  } // namespace cic_script_obj

  // ======================================================================
  // cic_deco_character SDK offsets
  // ======================================================================
  namespace cic_deco_character {
    namespace vtable {
      inline constexpr std::size_t play_animation = 54;
    } // namespace vtable

    namespace fields {
      inline constexpr std::size_t sight_range = 0x5F4;
      inline constexpr std::size_t sight_fade = 0x5F8;
    } // namespace fields
  } // namespace cic_deco_character

} // namespace ext_client::offsets

