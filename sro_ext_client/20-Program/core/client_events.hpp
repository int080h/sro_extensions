#pragma once

#include <cstdint>
#include <string>
#include <functional>

namespace ext_client {
  enum class packet_direction : std::uint8_t {
    client_to_server = 0,
    server_to_client = 1,
  };
} // namespace ext_client

class cps_version_check;
class cps_character_select;
struct cprocess_msg;
class cmsg_stream_buffer;
class cmsg;
struct IDirect3DDevice9;

namespace ext_client::render::menu {
  class menu_builder;
}

namespace ext_client::core::event {

  enum class event_type : int {
    on_tick = 0,
    on_shutdown,
    on_menu,
    on_menu_overlay,
    on_menu_toggle,

    on_version_check_create,
    on_version_check_update,
    on_version_check_msg_recv,
    on_set_child_process,
    on_load_intro_camera,

    on_char_select_enter,
    on_char_select_msg_recv,
    on_char_select_slot_change,
    on_char_select_update,

    on_show_notice,

    on_populate_target,
    on_populate_special_mob,
    on_update_special_mob,

    on_assert_report,
    on_get_quest_definition,

    on_packet,

    on_config_sync,

    on_d3d_device_created,
    on_d3d_pre_reset,
    on_d3d_post_reset,
    on_d3d_end_scene,
    on_background_tick,
  };

  struct version_check_create_context {
    cps_version_check *self;
    int process_type;
    char result;
  };

  struct version_check_update_context {
    cps_version_check *self;
  };

  struct version_check_msg_recv_context {
    cps_version_check *self;
    cmsg_stream_buffer *packet;
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
    cps_character_select *self;
    cprocess_msg *msg;
    int result;
  };

  struct char_select_msg_recv_context {
    cps_character_select *self;
    cmsg_stream_buffer *packet;
    int result;
  };

  struct char_select_slot_change_context {
    cps_character_select *self;
    int a2;
    int a3;
    int result;
  };

  struct char_select_update_context {
    cps_character_select *self;
    int result;
  };

  struct show_notice_context {
    void *self;
    const void *msg_obj;
    std::wstring message;
    bool is_blocked;
    bool is_modified;
    std::wstring modified_message;
    int result;
  };

  struct populate_target_context {
    void *self;
    void *target_id;
    int result;
  };

  struct populate_special_mob_context {
    void *self;
    void *target_id;
  };

  struct update_special_mob_context {
    void *self;
    unsigned int result;
  };

  struct assert_report_context {
    int line;
    const char *file;
    const char *msg;
    __int16 flags;
    bool handled;
    bool result;
  };

  struct get_quest_definition_context {
    void *self;
    unsigned int quest_id;
    void *result;
  };

  enum class packet_layer : std::uint8_t {
    cmsg = 0,
    stream = 1,
  };

  struct packet_context {
    void *session = nullptr;
    cmsg *cmsg_msg = nullptr;
    cmsg_stream_buffer *stream_msg = nullptr;
    ext_client::packet_direction direction = ext_client::packet_direction::server_to_client;
    packet_layer layer = packet_layer::cmsg;
    std::uint16_t opcode = 0;
    bool blocked = false;
    bool modified = false;
    int result = 0;
    const char *capture_point = nullptr;
  };

  struct d3d_device_created_context {
    IDirect3DDevice9 *device;
  };

  struct d3d_end_scene_context {
    IDirect3DDevice9 *device;
  };

  struct menu_draw_context {
    bool wants_capture_mouse = false;
    bool wants_capture_keyboard = false;
    bool menu_visible = false;
  };

  using cb_void = std::function<void()>;
  using cb_menu = std::function<void(render::menu::menu_builder &)>;
  using cb_menu_overlay = std::function<void(menu_draw_context &)>;
  using cb_version_check_create = std::function<void(version_check_create_context &)>;
  using cb_version_check_update = std::function<void(version_check_update_context &)>;
  using cb_version_check_msg_recv = std::function<void(version_check_msg_recv_context &)>;
  using cb_set_child_process = std::function<void(set_child_process_context &)>;
  using cb_load_intro_camera = std::function<void(load_intro_camera_context &)>;
  using cb_char_select_enter = std::function<void(char_select_enter_context &)>;
  using cb_char_select_msg_recv = std::function<void(char_select_msg_recv_context &)>;
  using cb_char_select_slot_change = std::function<void(char_select_slot_change_context &)>;
  using cb_char_select_update = std::function<void(char_select_update_context &)>;
  using cb_show_notice = std::function<void(show_notice_context &)>;
  using cb_populate_target = std::function<void(populate_target_context &)>;
  using cb_populate_special_mob = std::function<void(populate_special_mob_context &)>;
  using cb_update_special_mob = std::function<void(update_special_mob_context &)>;
  using cb_assert_report = std::function<void(assert_report_context &)>;
  using cb_get_quest_definition = std::function<void(get_quest_definition_context &)>;
  using cb_packet = std::function<void(packet_context &)>;
  using cb_d3d_device_created = std::function<void(d3d_device_created_context &)>;
  using cb_d3d_end_scene = std::function<void(d3d_end_scene_context &)>;

} // namespace ext_client::core::event

#define EVENT_ON_TICK                                                                                                  \
  static_cast<int>(::ext_client::core::event::event_type::on_tick), ::ext_client::core::event::cb_void
#define EVENT_ON_SHUTDOWN                                                                                              \
  static_cast<int>(::ext_client::core::event::event_type::on_shutdown), ::ext_client::core::event::cb_void
#define EVENT_ON_MENU                                                                                                  \
  static_cast<int>(::ext_client::core::event::event_type::on_menu), ::ext_client::core::event::cb_menu,                \
      ::ext_client::render::menu::menu_builder &
#define EVENT_ON_MENU_OVERLAY                                                                                          \
  static_cast<int>(::ext_client::core::event::event_type::on_menu_overlay),                                            \
      ::ext_client::core::event::cb_menu_overlay, ::ext_client::core::event::menu_draw_context &
#define EVENT_ON_MENU_TOGGLE                                                                                           \
  static_cast<int>(::ext_client::core::event::event_type::on_menu_toggle), ::ext_client::core::event::cb_void
#define EVENT_ON_VERSION_CHECK_CREATE                                                                                  \
  static_cast<int>(::ext_client::core::event::event_type::on_version_check_create),                                    \
      ::ext_client::core::event::cb_version_check_create, ::ext_client::core::event::version_check_create_context &
#define EVENT_ON_VERSION_CHECK_UPDATE                                                                                  \
  static_cast<int>(::ext_client::core::event::event_type::on_version_check_update),                                    \
      ::ext_client::core::event::cb_version_check_update, ::ext_client::core::event::version_check_update_context &
#define EVENT_ON_VERSION_CHECK_MSG_RECV                                                                                \
  static_cast<int>(::ext_client::core::event::event_type::on_version_check_msg_recv),                                  \
      ::ext_client::core::event::cb_version_check_msg_recv,                                                            \
      ::ext_client::core::event::version_check_msg_recv_context &
#define EVENT_ON_SET_CHILD_PROCESS                                                                                     \
  static_cast<int>(::ext_client::core::event::event_type::on_set_child_process),                                       \
      ::ext_client::core::event::cb_set_child_process, ::ext_client::core::event::set_child_process_context &
#define EVENT_ON_LOAD_INTRO_CAMERA                                                                                     \
  static_cast<int>(::ext_client::core::event::event_type::on_load_intro_camera),                                       \
      ::ext_client::core::event::cb_load_intro_camera, ::ext_client::core::event::load_intro_camera_context &
#define EVENT_ON_CHAR_SELECT_ENTER                                                                                     \
  static_cast<int>(::ext_client::core::event::event_type::on_char_select_enter),                                       \
      ::ext_client::core::event::cb_char_select_enter, ::ext_client::core::event::char_select_enter_context &
#define EVENT_ON_CHAR_SELECT_MSG_RECV                                                                                  \
  static_cast<int>(::ext_client::core::event::event_type::on_char_select_msg_recv),                                    \
      ::ext_client::core::event::cb_char_select_msg_recv, ::ext_client::core::event::char_select_msg_recv_context &
#define EVENT_ON_CHAR_SELECT_SLOT_CHANGE                                                                               \
  static_cast<int>(::ext_client::core::event::event_type::on_char_select_slot_change),                                 \
      ::ext_client::core::event::cb_char_select_slot_change,                                                           \
      ::ext_client::core::event::char_select_slot_change_context &
#define EVENT_ON_CHAR_SELECT_UPDATE                                                                                    \
  static_cast<int>(::ext_client::core::event::event_type::on_char_select_update),                                      \
      ::ext_client::core::event::cb_char_select_update, ::ext_client::core::event::char_select_update_context &
#define EVENT_ON_SHOW_NOTICE                                                                                           \
  static_cast<int>(::ext_client::core::event::event_type::on_show_notice), ::ext_client::core::event::cb_show_notice,  \
      ::ext_client::core::event::show_notice_context &
#define EVENT_ON_POPULATE_TARGET                                                                                       \
  static_cast<int>(::ext_client::core::event::event_type::on_populate_target),                                         \
      ::ext_client::core::event::cb_populate_target, ::ext_client::core::event::populate_target_context &
#define EVENT_ON_POPULATE_SPECIAL_MOB                                                                                  \
  static_cast<int>(::ext_client::core::event::event_type::on_populate_special_mob),                                    \
      ::ext_client::core::event::cb_populate_special_mob, ::ext_client::core::event::populate_special_mob_context &
#define EVENT_ON_UPDATE_SPECIAL_MOB                                                                                    \
  static_cast<int>(::ext_client::core::event::event_type::on_update_special_mob),                                      \
      ::ext_client::core::event::cb_update_special_mob, ::ext_client::core::event::update_special_mob_context &
#define EVENT_ON_ASSERT_REPORT                                                                                         \
  static_cast<int>(::ext_client::core::event::event_type::on_assert_report),                                           \
      ::ext_client::core::event::cb_assert_report, ::ext_client::core::event::assert_report_context &
#define EVENT_ON_GET_QUEST_DEFINITION                                                                                  \
  static_cast<int>(::ext_client::core::event::event_type::on_get_quest_definition),                                    \
      ::ext_client::core::event::cb_get_quest_definition, ::ext_client::core::event::get_quest_definition_context &
#define EVENT_ON_PACKET                                                                                                \
  static_cast<int>(::ext_client::core::event::event_type::on_packet), ::ext_client::core::event::cb_packet,            \
      ::ext_client::core::event::packet_context &
#define EVENT_ON_CONFIG_SYNC                                                                                           \
  static_cast<int>(::ext_client::core::event::event_type::on_config_sync), ::ext_client::core::event::cb_void
#define EVENT_ON_D3D_DEVICE_CREATED                                                                                    \
  static_cast<int>(::ext_client::core::event::event_type::on_d3d_device_created),                                      \
      ::ext_client::core::event::cb_d3d_device_created, ::ext_client::core::event::d3d_device_created_context &
#define EVENT_ON_D3D_PRE_RESET                                                                                         \
  static_cast<int>(::ext_client::core::event::event_type::on_d3d_pre_reset), ::ext_client::core::event::cb_void
#define EVENT_ON_D3D_POST_RESET                                                                                        \
  static_cast<int>(::ext_client::core::event::event_type::on_d3d_post_reset), ::ext_client::core::event::cb_void
#define EVENT_ON_D3D_END_SCENE                                                                                         \
  static_cast<int>(::ext_client::core::event::event_type::on_d3d_end_scene),                                           \
      ::ext_client::core::event::cb_d3d_end_scene, ::ext_client::core::event::d3d_end_scene_context &
#define EVENT_ON_BACKGROUND_TICK                                                                                       \
  static_cast<int>(::ext_client::core::event::event_type::on_background_tick), ::ext_client::core::event::cb_void
