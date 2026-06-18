set(IMGUI_ROOT "${CMAKE_SOURCE_DIR}/third-party/imgui")
if(NOT EXISTS "${IMGUI_ROOT}/imgui.cpp"
    AND EXISTS "${ISROR_DEVKIT_ROOT}/source/third-party/imgui/imgui/imgui.cpp")
    set(IMGUI_ROOT "${ISROR_DEVKIT_ROOT}/source/third-party/imgui")
    set(IMGUI_SRC_DIR "${IMGUI_ROOT}/imgui")
    set(IMGUI_BACKEND_DIR "${IMGUI_ROOT}/imgui/examples")
else()
    set(IMGUI_SRC_DIR "${IMGUI_ROOT}")
    set(IMGUI_BACKEND_DIR "${IMGUI_ROOT}/backends")
endif()

set(EXT_CLIENT_LOCAL_SOURCES
    # core
    dllmain.cpp
    ext_client.cpp
    config/client_config.cpp

    # hooks
    hooks/hook_manager.cpp
    hooks/exit_hooks.cpp
    hooks/d3d_hook.cpp
    hooks/version_check_hook.cpp
    hooks/loading_splash_overlay.cpp
    hooks/title_hook.cpp
    hooks/title_login_layout.cpp
    hooks/character_select_hook.cpp
    hooks/packet_hook.cpp
    hooks/interface_hide_hook.cpp
    hooks/interface_manager_runtime.cpp
    hooks/welcome_msg_hook.cpp
    hooks/sight_range_hook.cpp
    hooks/target_window_hook.cpp
    hooks/intro_hook.cpp
    hooks/quest_hook.cpp
    hooks/assert_hook.cpp

    # menu
    menu/menu.cpp
    menu/menu_ui.cpp
    menu/menu_theme.cpp
    menu/menu_shell.cpp
    menu/settings_panels.cpp
    menu/settings_interface.cpp
    menu/interface_manager.cpp
    menu/ui_browser_common.cpp
    menu/settings_version_check.cpp
    menu/settings_login.cpp
    menu/settings_graphics.cpp
    menu/settings_welcome.cpp
    menu/settings_advanced.cpp
    menu/settings_app.cpp
    menu/settings_combat.cpp
    menu/widget_inspector.cpp
    menu/packet_inspector.cpp
    menu/packet_decoder.cpp

    # sdk
    sdk/net_manager.cpp
    sdk/net_msg.cpp
    sdk/cmsg_stream_buffer.cpp
    sdk/cmsg.cpp
    sdk/cclient_net.cpp
    sdk/cif_manager.cpp
    sdk/cgame_text.cpp
    sdk/cd3d_application.cpp
    sdk/cgfx_video3d.cpp
    sdk/cps_version_check.cpp
    sdk/cps_title.cpp
    sdk/cps_character_select.cpp
    sdk/cps_mission.cpp
    sdk/cicharactor.cpp
    sdk/cicplayer.cpp
    sdk/ccompoundobj.cpp
    sdk/cif_static.cpp
    sdk/cif_edit.cpp
    sdk/calarm_entry.cpp
    sdk/calarm_data.cpp
    sdk/calarm_store.cpp
    sdk/calarm_guide_mgr_wnd.cpp
    sdk/cnif_sro_ingame_start.cpp
    sdk/cif_notify.cpp
    sdk/cg_interface.cpp
    sdk/cres_id_manager.cpp
    sdk/cnif_page_manager.cpp
    sdk/cobj_render_manager.cpp
    sdk/cirm_manager.cpp
    sdk/centity_manager.cpp
    sdk/centity_manager_client.cpp
    sdk/cif_target_window.cpp
    sdk/sworld.cpp
    sdk/cic_deco_character.cpp
    sdk/cic_script_obj.cpp
    sdk/cgwnd.cpp
    sdk/sro_hierarchy.cpp

    # utils
    utils/hooks.cpp
    utils/shutdown_guard.cpp
    utils/window_style.cpp
    utils/log.cpp
    utils/msvc9_stl.cpp
    utils/ui_res_catalog.cpp
    utils/process_manager.cpp
    utils/process.cpp
    utils/string.cpp
    utils/vectorf.cpp

    features/combat_overlay/entity_manager.cpp
    features/combat_overlay/combat_target.cpp
    features/combat_overlay/target_window_hp.cpp
    features/combat_overlay/combat_packets.cpp
    features/combat_overlay/combat_overlay.cpp
    features/combat_overlay/combat_overlay_ui.cpp
)

set(EXT_CLIENT_IMGUI_SOURCES
    ${IMGUI_SRC_DIR}/imgui.cpp
    ${IMGUI_SRC_DIR}/imgui_draw.cpp
    ${IMGUI_SRC_DIR}/imgui_tables.cpp
    ${IMGUI_SRC_DIR}/imgui_widgets.cpp
    ${IMGUI_BACKEND_DIR}/imgui_impl_dx9.cpp
    ${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp
)

add_library(ext_client SHARED
    ${EXT_CLIENT_LOCAL_SOURCES}
    ${EXT_CLIENT_IMGUI_SOURCES}
)

target_include_directories(ext_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk
    ${IMGUI_SRC_DIR}
    ${IMGUI_BACKEND_DIR}
    ${DirectX_INCLUDE_DIRS}
)

target_link_libraries(ext_client PRIVATE minhook ${DirectX_LIBRARIES} gdiplus)

target_compile_definitions(ext_client PRIVATE NOMINMAX)

if(CMAKE_CONFIGURATION_TYPES)
  target_compile_definitions(ext_client PRIVATE
    $<$<CONFIG:Debug>:EXT_CLIENT_DEV_BUILD=1>
    $<$<NOT:$<CONFIG:Debug>>:EXT_CLIENT_DEV_BUILD=0>)
else()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ext_client PRIVATE EXT_CLIENT_DEV_BUILD=1)
  else()
    target_compile_definitions(ext_client PRIVATE EXT_CLIENT_DEV_BUILD=0)
  endif()
endif()

target_link_directories(ext_client PRIVATE ${DirectX_LIBRARY_PATHS})

set_target_properties(ext_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../output"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "" FILES ${EXT_CLIENT_LOCAL_SOURCES})
source_group("imgui" FILES
    ${IMGUI_SRC_DIR}/imgui.cpp
    ${IMGUI_SRC_DIR}/imgui_draw.cpp
    ${IMGUI_SRC_DIR}/imgui_tables.cpp
    ${IMGUI_SRC_DIR}/imgui_widgets.cpp
)
source_group("imgui\\d3d" FILES ${IMGUI_BACKEND_DIR}/imgui_impl_dx9.cpp)
source_group("imgui\\win" FILES ${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp)
