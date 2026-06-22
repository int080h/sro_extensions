#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <string>
#include <cstdint>

#include <string_view>

#include "core/core_main.hpp" // ext_client::packet_direction

// Forward declarations
class cps_version_check;
class cps_character_select;
struct cprocess_msg;
class cmsg_stream_buffer;
class cmsg;
struct IDirect3DDevice9;

namespace ext_client::core::event {

  enum class event_type {
    on_tick,
    on_shutdown,
    on_menu,
    on_menu_toggle,



    // Title / Version Check Screen
    on_version_check_create,
    on_version_check_update,
    on_version_check_msg_recv,
    on_set_child_process,
    on_load_intro_camera,

    // Character Select
    on_char_select_enter,
    on_char_select_msg_recv,
    on_char_select_slot_change,
    on_char_select_update,

    // Notices & Welcome messages
    on_show_notice,

    // Targets
    on_populate_target,
    on_populate_special_mob,
    on_update_special_mob,

    // Assert / bypasses
    on_assert_report,
    on_get_quest_definition,

    // Network / Packets
    on_packet,

    // D3D lifecycle events
    on_d3d_device_created,
    on_d3d_pre_reset,
    on_d3d_post_reset,
    on_d3d_end_scene
  };

  // Event context structures
  struct version_check_create_context {
    cps_version_check* self;
    int process_type;
    char result;
  };

  struct version_check_update_context {
    cps_version_check* self;
  };

  struct version_check_msg_recv_context {
    cps_version_check* self;
    cmsg_stream_buffer* packet;
    int result;
  };

  struct set_child_process_context {
    int process_type;
    int activate;
  };

  struct load_intro_camera_context {
    int a1;
    int a2;
    char a3;
    int result;
  };

  struct char_select_enter_context {
    cps_character_select* self;
    cprocess_msg* msg;
    int result;
  };

  struct char_select_msg_recv_context {
    cps_character_select* self;
    cmsg_stream_buffer* packet;
    int result;
  };

  struct char_select_slot_change_context {
    cps_character_select* self;
    int a2;
    int a3;
    int result;
  };

  struct char_select_update_context {
    cps_character_select* self;
    int result;
  };

  struct show_notice_context {
    void* self;
    const void* msg_obj;
    std::wstring message;
    bool is_blocked;
    bool is_modified;
    std::wstring modified_message;
    int result;
  };

  struct populate_target_context {
    void* self;
    void* target_id;
    int result;
  };

  struct populate_special_mob_context {
    void* self;
    void* target_id;
  };

  struct update_special_mob_context {
    void* self;
    unsigned int result;
  };

  struct assert_report_context {
    int line;
    const char* file;
    const char* msg;
    __int16 flags;
    bool handled;
    bool result;
  };

  struct get_quest_definition_context {
    void* self;
    unsigned int quest_id;
    void* result;
  };

  enum class packet_layer : std::uint8_t { cmsg = 0, stream = 1 };

  struct packet_context {
    void* session = nullptr;
    cmsg* cmsg_msg = nullptr;
    cmsg_stream_buffer* stream_msg = nullptr;
    ext_client::packet_direction direction = ext_client::packet_direction::server_to_client;
    packet_layer layer = packet_layer::cmsg;
    std::uint16_t opcode = 0;
    bool blocked = false;
    bool modified = false;
    int result = 0;
    const char* capture_point = nullptr;
  };

  struct d3d_device_created_context {
    IDirect3DDevice9* device;
  };

  struct d3d_end_scene_context {
    IDirect3DDevice9* device;
  };

  struct menu_draw_context {
    bool wants_capture_mouse = false;
    bool wants_capture_keyboard = false;
    bool menu_visible = false;
  };


  using event_callback = std::function<void(void*)>;

  struct listener_entry {
    std::string plugin_id;
    event_callback callback;
  };

  class event_manager {
  public:
    static auto get() -> event_manager&;

    auto register_event(event_type type, event_callback callback) -> void;
    auto register_plugin_event(event_type type, std::string_view plugin_id, event_callback callback) -> void;
    auto dispatch(event_type type, void* context) -> void;

  private:
    std::mutex m_mutex;
    std::unordered_map<event_type, std::vector<listener_entry>> m_listeners;
  };

  struct event_registrar {
    event_registrar(event_type type, event_callback callback) {
      event_manager::get().register_event(type, callback);
    }
    event_registrar(event_type type, std::string_view plugin_id, event_callback callback) {
      event_manager::get().register_plugin_event(type, plugin_id, callback);
    }
  };

  #define ADD_EVENT_CONCAT2(a, b) a##b
  #define ADD_EVENT_CONCAT(a, b) ADD_EVENT_CONCAT2(a, b)
  #define ADD_EVENT(type, callback) \
    static const ::ext_client::core::event::event_registrar ADD_EVENT_CONCAT(g_registrar_line_, __LINE__)(type, callback)
  #define ADD_PLUGIN_EVENT(plugin_id, type, callback) \
    ::ext_client::core::event::event_manager::get().register_plugin_event(type, plugin_id, callback)

} // namespace ext_client::core::event
