#include "core/core_hooks.hpp"

#include "core/core_main.hpp"
#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"
#include "sdk/cmsg_stream_buffer.hpp"
#include "sdk/cmsg.hpp"
#include "sdk/cprocess.hpp"
#include "sdk/cps_character_select.hpp"
#include "sdk/cps_version_check.hpp"
#include "sdk/cgfx_video3d.hpp"
#include "sdk/ccontroler.hpp"

#include <Windows.h>
#include <d3d9.h>
#include <string>
#include <vector>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;
using namespace ext_client::core::event;

namespace ext_client::core::hooks::core_hooks {
  namespace {

    hook_group g_hooks;

    // =========================================================================
    // D3D Helpers
    // =========================================================================
    auto apply_presentation_params(D3DPRESENT_PARAMETERS* params) -> void {
      if (!params) {
        return;
      }
      const auto& cfg = ext_client::core::config::data().graphic;
      if (cfg.d3d_triple_buffering && params->Windowed) {
        log_msg("[core_hooks] Forcing triple buffering (BackBufferCount = 2).");
        params->BackBufferCount = 2;
      }
      if (cfg.d3d_discard_depth_stencil) {
        log_msg("[core_hooks] Forcing D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL.");
        params->Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
      }
    }

    auto apply_behavior_flags(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE device_type, DWORD& behavior_flags) -> void {
      const auto& cfg = ext_client::core::config::data().graphic;
      if (!cfg.d3d_force_hardware_vp) {
        return;
      }

      D3DCAPS9 caps{};
      if (FAILED(d3d->GetDeviceCaps(adapter, device_type, &caps))) {
        log_msg("[core_hooks] Failed to query device capabilities.");
        return;
      }

      if (!(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
        log_msg("[core_hooks] GPU does not support Hardware T&L. Software VP will be used.");
        return;
      }

      log_msg("[core_hooks] GPU supports Hardware T&L. Forcing Hardware VP.");
      behavior_flags &= ~D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      behavior_flags &= ~D3DCREATE_MIXED_VERTEXPROCESSING;
      behavior_flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;

      if (cfg.d3d_force_pure_device) {
        if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) {
          log_msg("[core_hooks] GPU supports Pure Device. Forcing Pure Device.");
          behavior_flags |= D3DCREATE_PUREDEVICE;
        } else {
          log_msg("[core_hooks] Pure Device requested but not supported by GPU.");
        }
      }
    }

    // =========================================================================
    // Detours & Hooks Definitions
    // =========================================================================

    // 1. Assert (BSLib) Hook
    make_hook<convention_type::cdecl_t, bool, int, const char*, const char*, __int16> g_assert_report;
    auto __cdecl assert_report_detour(int line, const char* file, const char* msg, __int16 flags) -> bool {
      assert_report_context ctx{ line, file, msg, flags, false, false };
      event_manager::get().dispatch(event_type::on_assert_report, &ctx);
      if (ctx.handled) {
        return ctx.result;
      }
      return g_assert_report.call_original(line, file, msg, flags);
    }

    // 2. Notice Display Hook
    make_hook<convention_type::thiscall_t, int, void*, void*, const void*> g_show_notice;
    auto __fastcall show_notice_detour(void* self, void* edx, const void* msg_obj) -> int {
      if (!msg_obj) {
        return g_show_notice.call_original(self, edx, msg_obj);
      }
      auto ref = ext_client::msvc9::wstring_ref::from(msg_obj);
      show_notice_context ctx{ self, msg_obj, std::wstring(ref.data(), ref.length()), false, false, L"", 0 };
      event_manager::get().dispatch(event_type::on_show_notice, &ctx);
      if (ctx.is_blocked) {
        return ctx.result;
      }
      if (ctx.is_modified) {
        ext_client::msvc9::wstring custom_msg(ctx.modified_message.c_str());
        return g_show_notice.call_original(self, edx, custom_msg.raw());
      }
      return g_show_notice.call_original(self, edx, msg_obj);
    }

    // 3. Target HP Overlay Hooks
    make_hook<convention_type::thiscall_t, int, void*, void*, void*> g_populate_target;
    make_hook<convention_type::thiscall_t, void, void*, void*, void*> g_populate_special_mob;
    make_hook<convention_type::thiscall_t, unsigned int, void*, void*> g_update_special_mob;

    auto __fastcall populate_target_detour(void* self, void* edx, void* target_id) -> int {
      int result = g_populate_target.call_original(self, edx, target_id);
      populate_target_context ctx{ self, target_id, result };
      event_manager::get().dispatch(event_type::on_populate_target, &ctx);
      return ctx.result;
    }

    auto __fastcall populate_special_mob_detour(void* self, void* edx, void* target_id) -> void {
      g_populate_special_mob.call_original(self, edx, target_id);
      populate_special_mob_context ctx{ self, target_id };
      event_manager::get().dispatch(event_type::on_populate_special_mob, &ctx);
    }

    auto __fastcall update_special_mob_detour(void* self, void* edx) -> unsigned int {
      unsigned int result = g_update_special_mob.call_original(self, edx);
      update_special_mob_context ctx{ self, result };
      event_manager::get().dispatch(event_type::on_update_special_mob, &ctx);
      return ctx.result;
    }

    // 4. Character Select Screen Hooks
    make_hook<convention_type::thiscall_t, int, cps_character_select*, void*, cprocess_msg*> g_char_select_on_enter;
    make_hook<convention_type::thiscall_t, int, cps_character_select*, void*, int, int> g_char_select_on_slot_change;
    make_hook<convention_type::thiscall_t, int, cps_character_select*, void*, cmsg_stream_buffer*> g_char_select_on_msg_recv;
    make_hook<convention_type::thiscall_t, int, cps_character_select*, void*> g_char_select_on_update;

    auto __fastcall char_select_on_enter_detour(cps_character_select* self, void* edx, cprocess_msg* msg) -> int {
      int result = g_char_select_on_enter.call_original(self, edx, msg);
      char_select_enter_context ctx{ self, msg, result };
      event_manager::get().dispatch(event_type::on_char_select_enter, &ctx);
      return ctx.result;
    }

    auto __fastcall char_select_on_slot_change_detour(cps_character_select* self, void* edx, int a2, int a3) -> int {
      int result = g_char_select_on_slot_change.call_original(self, edx, a2, a3);
      char_select_slot_change_context ctx{ self, a2, a3, result };
      event_manager::get().dispatch(event_type::on_char_select_slot_change, &ctx);
      return ctx.result;
    }

    auto __fastcall char_select_on_msg_recv_detour(cps_character_select* self, void* edx, cmsg_stream_buffer* packet) -> int {
      int result = g_char_select_on_msg_recv.call_original(self, edx, packet);
      char_select_msg_recv_context ctx{ self, packet, result };
      event_manager::get().dispatch(event_type::on_char_select_msg_recv, &ctx);
      return ctx.result;
    }

    auto __fastcall char_select_on_update_detour(cps_character_select* self, void* edx) -> int {
      int result = g_char_select_on_update.call_original(self, edx);
      char_select_update_context ctx{ self, result };
      event_manager::get().dispatch(event_type::on_char_select_update, &ctx);
      return ctx.result;
    }

    // 5. Loading & Version Check Screen Hooks
    make_hook<convention_type::thiscall_t, char, cps_version_check*, void*, int> g_version_check_on_create;
    make_hook<convention_type::thiscall_t, void, cps_version_check*, void*> g_version_check_on_update;
    make_hook<convention_type::thiscall_t, int, cps_version_check*, void*, cmsg_stream_buffer*> g_version_check_on_msg_recv;
    make_hook<convention_type::thiscall_t, int, void*, void*, int, int> g_set_child_process;
    make_hook<convention_type::cdecl_t, int, int, int, char> g_load_intro_camera;

    auto __fastcall version_check_on_create_detour(cps_version_check* self, void* edx, int process_type) -> char {
      char result = g_version_check_on_create.call_original(self, edx, process_type);
      version_check_create_context ctx{ self, process_type, result };
      event_manager::get().dispatch(event_type::on_version_check_create, &ctx);
      return ctx.result;
    }

    auto __fastcall version_check_on_update_detour(cps_version_check* self, void* edx) -> void {
      g_version_check_on_update.call_original(self, edx);
      version_check_update_context ctx{ self };
      event_manager::get().dispatch(event_type::on_version_check_update, &ctx);
    }

    auto __fastcall version_check_on_msg_recv_detour(cps_version_check* self, void* edx, cmsg_stream_buffer* packet) -> int {
      int result = g_version_check_on_msg_recv.call_original(self, edx, packet);
      version_check_msg_recv_context ctx{ self, packet, result };
      event_manager::get().dispatch(event_type::on_version_check_msg_recv, &ctx);
      return ctx.result;
    }

    auto __fastcall set_child_process_detour(void* mgr, void* edx, int process_type, int activate) -> int {
      const int result = g_set_child_process.call_original(mgr, edx, process_type, activate);

      set_child_process_context ctx{ process_type, activate };
      event_manager::get().dispatch(event_type::on_set_child_process, &ctx);

      void* child = ccontroler::active_child();
      if (child == nullptr && result != 0) {
        child = reinterpret_cast<void*>(result);
      }
      ccontroler::note_set_child_process(child, activate);
      return result;
    }

    auto __cdecl load_intro_camera_detour(int a1, int a2, char a3) -> int {
      int result = g_load_intro_camera.call_original(a1, a2, a3);
      load_intro_camera_context ctx{ a1, a2, a3, result };
      event_manager::get().dispatch(event_type::on_load_intro_camera, &ctx);
      return ctx.result;
    }

    // 6. Quest Hook
    make_hook<convention_type::thiscall_t, void*, void*, void*, unsigned int> g_get_quest_definition;
    auto __fastcall get_quest_definition_detour(void* self, void* edx, unsigned int quest_id) -> void* {
      void* result = g_get_quest_definition.call_original(self, edx, quest_id);
      get_quest_definition_context ctx{ self, quest_id, result };
      event_manager::get().dispatch(event_type::on_get_quest_definition, &ctx);
      return ctx.result;
    }

    // 7. Network Hooks
    make_hook<convention_type::thiscall_t, int, void*, void*, void*> g_dispatch_handler;
    make_hook<convention_type::cdecl_t, void*, cmsg_stream_buffer*> g_send_from_buffer;
    make_hook<convention_type::thiscall_t, int, void*, void*, cmsg*> g_session_send_a;
    make_hook<convention_type::thiscall_t, int, void*, void*, cmsg*> g_session_send_b;

    auto __fastcall dispatch_handler_detour(void* self, void* edx, void* msg) -> int {
      auto* cmsg_ptr = static_cast<cmsg*>(msg);
      packet_context ctx{};
      ctx.session = self;
      ctx.cmsg_msg = cmsg_ptr;
      ctx.direction = ext_client::packet_direction::server_to_client;
      ctx.layer = packet_layer::cmsg;
      ctx.opcode = cmsg_ptr ? cmsg_ptr->opcode() : 0;
      ctx.capture_point = "dispatch_handler";
      event_manager::get().dispatch(event_type::on_packet, &ctx);
      if (ctx.blocked) {
        return ctx.result;
      }
      return g_dispatch_handler.call_original(self, edx, msg);
    }

    auto __cdecl send_from_buffer_detour(cmsg_stream_buffer* msg) -> void* {
      packet_context ctx{};
      ctx.stream_msg = msg;
      ctx.direction = ext_client::packet_direction::client_to_server;
      ctx.layer = packet_layer::stream;
      ctx.opcode = msg ? msg->opcode() : 0;
      ctx.capture_point = "send_from_buffer";
      event_manager::get().dispatch(event_type::on_packet, &ctx);
      if (ctx.blocked) {
        return nullptr;
      }
      return g_send_from_buffer.call_original(msg);
    }

    auto __fastcall session_send_a_detour(void* self, void* edx, cmsg* msg) -> int {
      packet_context ctx{};
      ctx.session = self;
      ctx.cmsg_msg = msg;
      ctx.direction = ext_client::packet_direction::client_to_server;
      ctx.layer = packet_layer::cmsg;
      ctx.opcode = msg ? msg->opcode() : 0;
      ctx.capture_point = "session_send";
      event_manager::get().dispatch(event_type::on_packet, &ctx);
      if (ctx.blocked) {
        return ctx.result;
      }
      return g_session_send_a.call_original(self, edx, msg);
    }

    auto __fastcall session_send_b_detour(void* self, void* edx, cmsg* msg) -> int {
      packet_context ctx{};
      ctx.session = self;
      ctx.cmsg_msg = msg;
      ctx.direction = ext_client::packet_direction::client_to_server;
      ctx.layer = packet_layer::cmsg;
      ctx.opcode = msg ? msg->opcode() : 0;
      ctx.capture_point = "session_send";
      event_manager::get().dispatch(event_type::on_packet, &ctx);
      if (ctx.blocked) {
        return ctx.result;
      }
      return g_session_send_b.call_original(self, edx, msg);
    }

    // 8. Quit / Shutdown Hooks
    make_hook<convention_type::stdcall_t, void, UINT> g_exit_process;
    make_hook<convention_type::thiscall_t, void, void*, void*> g_cps_quit_on_update;
    make_hook<convention_type::stdcall_t, void, int> g_post_quit_message;

    auto WINAPI exit_process_detour(UINT exit_code) -> void {
      app_main::get().request_unload();
      app_main::get().shutdown(shutdown_mode::terminate_after_cleanup);
      TerminateProcess(GetCurrentProcess(), exit_code);
    }

    auto __fastcall cps_quit_on_update_detour(void* self, void* edx) -> void {
      app_main::get().request_unload();
      app_main::get().shutdown(shutdown_mode::terminate_after_cleanup);
      TerminateProcess(GetCurrentProcess(), 0);
    }

    auto WINAPI post_quit_message_detour(int exit_code) -> void {
      const DWORD current_thread_id = GetCurrentThreadId();
      const DWORD main_thread_id = app_main::get().get_main_thread_id();
      if (main_thread_id != 0 && current_thread_id != main_thread_id) {
        g_post_quit_message.call_original(exit_code);
        return;
      }

      app_main::get().request_unload();
      app_main::get().shutdown(shutdown_mode::terminate_after_cleanup);
      TerminateProcess(GetCurrentProcess(), static_cast<UINT>(exit_code));
    }

    // 9. Direct3D Detours
    auto hook_create_device(IDirect3D9* d3d) -> void;
    auto hook_device_methods(IDirect3DDevice9* device) -> void;

    make_hook<convention_type::stdcall_t, IDirect3D9*, UINT> g_direct3d_create9;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**> g_create_device;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9*> g_end_scene;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9*, D3DPRESENT_PARAMETERS*> g_reset;

    auto WINAPI direct3d_create9_detour(UINT SDKVersion) -> IDirect3D9* {
      IDirect3D9* d3d = g_direct3d_create9.call_original(SDKVersion);
      if (d3d) {
        log_msg("[core_hooks] Direct3DCreate9 intercepted");
        hook_create_device(d3d);
      }
      return d3d;
    }

    auto __stdcall create_device_detour(
      IDirect3D9* self,
      UINT Adapter,
      D3DDEVTYPE DeviceType,
      HWND hFocusWindow,
      DWORD BehaviorFlags,
      D3DPRESENT_PARAMETERS* pPresentationParameters,
      IDirect3DDevice9** ppReturnedDeviceInterface
    ) -> HRESULT {
      apply_behavior_flags(self, Adapter, DeviceType, BehaviorFlags);
      apply_presentation_params(pPresentationParameters);

      HRESULT hr = g_create_device.call_original(self, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
      if (SUCCEEDED(hr) && ppReturnedDeviceInterface && *ppReturnedDeviceInterface) {
        hook_device_methods(*ppReturnedDeviceInterface);
        
        d3d_device_created_context ctx{ *ppReturnedDeviceInterface };
        event_manager::get().dispatch(event_type::on_d3d_device_created, &ctx);
      }
      return hr;
    }

    auto __stdcall end_scene_detour(IDirect3DDevice9* device) -> HRESULT {
      const HRESULT hr = g_end_scene.call_original(device);
      d3d_end_scene_context ctx{ device };
      event_manager::get().dispatch(event_type::on_d3d_end_scene, &ctx);
      return hr;
    }

    auto __stdcall reset_detour(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) -> HRESULT {
      apply_presentation_params(params);

      event_manager::get().dispatch(event_type::on_d3d_pre_reset, nullptr);
      const HRESULT hr = g_reset.call_original(device, params);
      if (SUCCEEDED(hr)) {
        event_manager::get().dispatch(event_type::on_d3d_post_reset, nullptr);
      }
      return hr;
    }

    auto hook_create_device(IDirect3D9* d3d) -> void {
      if (g_create_device.is_applied()) {
        return;
      }
      const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void***>(d3d));
      const auto create_device_addr = ext_client::offsets::vtable_slot(vtable, 16);
      if (g_hooks.install(g_create_device, create_device_addr, &create_device_detour, "core_hooks", "CreateDevice")) {
        log_msg("[core_hooks] hooked CreateDevice @ 0x%08X", create_device_addr);
      }
    }

    auto hook_device_methods(IDirect3DDevice9* device) -> void {
      if (g_end_scene.is_applied() || g_reset.is_applied()) {
        return;
      }
      const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void***>(device));
      const auto end_scene_addr = ext_client::offsets::vtable_slot(vtable, 42);
      const auto reset_addr = ext_client::offsets::vtable_slot(vtable, 16);

      if (g_hooks.install(g_end_scene, end_scene_addr, &end_scene_detour, "core_hooks", "EndScene") &&
          g_hooks.install(g_reset, reset_addr, &reset_detour, "core_hooks", "Reset")) {
        log_msg("[core_hooks] hooked EndScene @ 0x%08X and Reset @ 0x%08X", end_scene_addr, reset_addr);
      }
    }

  } // namespace

  auto install_all() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace ext_client::offsets;

    // 1. Assert
    if (!g_hooks.install(g_assert_report, bslib::functions::assert_report, &assert_report_detour, "core_hooks", "assert_report")) {
      return false;
    }

    // 2. Notices
    if (!g_hooks.install(g_show_notice, cif_notify::functions::show_notice, &show_notice_detour, "core_hooks", "show_notice")) {
      return false;
    }

    // 3. Targets
    if (!g_hooks.install(g_populate_target, cif_target_window::functions::populate_target, &populate_target_detour, "core_hooks", "populate_target") ||
        !g_hooks.install(g_populate_special_mob, cif_target_window::functions::populate_special_mob, &populate_special_mob_detour, "core_hooks", "populate_special_mob") ||
        !g_hooks.install(g_update_special_mob, cif_target_window::functions::on_update_special_mob, &update_special_mob_detour, "core_hooks", "update_special_mob")) {
      g_hooks.uninstall();
      return false;
    }

    // 4. Character Select
    if (!g_hooks.install(g_char_select_on_enter, cps_character_select::functions::on_enter, &char_select_on_enter_detour, "core_hooks", "char_select_on_enter") ||
        !g_hooks.install(g_char_select_on_slot_change, cps_character_select::functions::on_slot_change, &char_select_on_slot_change_detour, "core_hooks", "char_select_on_slot_change") ||
        !g_hooks.install(g_char_select_on_msg_recv, cps_character_select::functions::on_msg_recv, &char_select_on_msg_recv_detour, "core_hooks", "char_select_on_msg_recv") ||
        !g_hooks.install(g_char_select_on_update, cps_character_select::functions::on_update, &char_select_on_update_detour, "core_hooks", "char_select_on_update")) {
      g_hooks.uninstall();
      return false;
    }

    // 5. Version Check
     if (!g_hooks.install(g_version_check_on_create, cps_version_check::functions::on_create, &version_check_on_create_detour, "core_hooks", "version_check_on_create") ||
        !g_hooks.install(g_version_check_on_update, cps_version_check::functions::on_update, &version_check_on_update_detour, "core_hooks", "version_check_on_update") ||
        !g_hooks.install(g_version_check_on_msg_recv, cps_version_check::functions::on_msg_recv, &version_check_on_msg_recv_detour, "core_hooks", "version_check_on_msg_recv") ||
        !g_hooks.install(g_set_child_process, ccontroler::functions::set_child_process, &set_child_process_detour, "core_hooks", "set_child_process") ||
        !g_hooks.install(g_load_intro_camera, cps_version_check::functions::load_intro_camera, &load_intro_camera_detour, "core_hooks", "load_intro_camera")) {
      g_hooks.uninstall();
      return false;
    }

    // 6. Quests
    if (!g_hooks.install(g_get_quest_definition, quest::functions::get_quest_definition, &get_quest_definition_detour, "core_hooks", "get_quest_definition")) {
      g_hooks.uninstall();
      return false;
    }

    // 7. Network
      if (!g_hooks.install(g_dispatch_handler, ext_client::offsets::cmsg::functions::dispatch_handler, &dispatch_handler_detour, "core_hooks", "dispatch_handler") ||
        !g_hooks.install(g_send_from_buffer, ext_client::offsets::cmsg::functions::send_from_buffer, &send_from_buffer_detour, "core_hooks", "send_from_buffer") ||
        !g_hooks.install(g_session_send_a, ext_client::offsets::cmsg::functions::session_send_a, &session_send_a_detour, "core_hooks", "session_send_a") ||
        !g_hooks.install(g_session_send_b, ext_client::offsets::cmsg::functions::session_send_b, &session_send_b_detour, "core_hooks", "session_send_b")) {
      g_hooks.uninstall();
      return false;
    }

    // 8. Quit
    const auto exit_process_addr = reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
    const auto post_quit_message_addr = reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("user32.dll"), "PostQuitMessage"));
    if (!g_hooks.install(g_exit_process, exit_process_addr, &exit_process_detour, "core_hooks", "ExitProcess") ||
        !g_hooks.install(g_post_quit_message, post_quit_message_addr, &post_quit_message_detour, "core_hooks", "PostQuitMessage") ||
        !g_hooks.install(g_cps_quit_on_update, cps_quit::functions::on_update, &cps_quit_on_update_detour, "core_hooks", "CPSQuit::OnUpdate")) {
      g_hooks.uninstall();
      return false;
    }

    return true;
  }

  auto uninstall_all() -> void {
    g_hooks.uninstall();
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

  auto install_lazy() -> void {
    HMODULE h_d3d9 = GetModuleHandleA("d3d9.dll");
    if (!h_d3d9) {
      return;
    }

    auto d3d_create9_fn = reinterpret_cast<std::uintptr_t>(GetProcAddress(h_d3d9, "Direct3DCreate9"));
    if (d3d_create9_fn && !g_direct3d_create9.is_applied()) {
      if (g_hooks.install(g_direct3d_create9, d3d_create9_fn, &direct3d_create9_detour, "core_hooks", "Direct3DCreate9")) {
        log_msg("[core_hooks] hooked Direct3DCreate9 @ 0x%08X", d3d_create9_fn);
      }
    }

    if (auto* app = cgfx_video3d::get()) {
      if (auto* device = app->device()) {
        if (!g_end_scene.is_applied() && !g_reset.is_applied()) {
          log_msg("[core_hooks] active device already exists, hooking device methods immediately");
          hook_device_methods(device);
        }
      }
    }
  }

  auto uninstall_lazy() -> void {
    // implicit in uninstall_all
  }

  auto tick() -> void {
    static DWORD last_tick = 0;
    const DWORD now = GetTickCount();
    if (now == last_tick) {
      return;
    }
    last_tick = now;

    // Dispatch tick event to subscribers
    event_manager::get().dispatch(event_type::on_tick, nullptr);
  }

} // namespace ext_client::core::hooks::core_hooks
