#include "pch.hpp"
#include "core/hooks/engine_hooks.hpp"
#include "core/event_bus.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/msvc9_stl.hpp"
#include "sdk/process/cps_character_select.hpp"
#include "sdk/process/cps_version_check.hpp"
#include "sdk/game/ccontroler.hpp"
#include "core/config.hpp"
#include "core/plugin_manager.hpp"
#include <string>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;
using namespace ext_client::core::event;

namespace ext_client::core::hooks::client_hooks {
  namespace {

    hook_group g_hooks;

    // =========================================================================
    // Detour Helpers
    // =========================================================================

    template <typename Context> auto trigger_context(Context &ctx) -> void;

    template <> auto trigger_context<populate_target_context>(populate_target_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_POPULATE_TARGET, ctx);
    }

    template <> auto trigger_context<populate_special_mob_context>(populate_special_mob_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_POPULATE_SPECIAL_MOB, ctx);
    }

    template <> auto trigger_context<update_special_mob_context>(update_special_mob_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_UPDATE_SPECIAL_MOB, ctx);
    }

    template <> auto trigger_context<char_select_enter_context>(char_select_enter_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_CHAR_SELECT_ENTER, ctx);
    }

    template <> auto trigger_context<char_select_slot_change_context>(char_select_slot_change_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_CHAR_SELECT_SLOT_CHANGE, ctx);
    }

    template <> auto trigger_context<char_select_msg_recv_context>(char_select_msg_recv_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_CHAR_SELECT_MSG_RECV, ctx);
    }

    template <> auto trigger_context<char_select_update_context>(char_select_update_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_CHAR_SELECT_UPDATE, ctx);
    }

    template <> auto trigger_context<version_check_create_context>(version_check_create_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_VERSION_CHECK_CREATE, ctx);
    }

    template <> auto trigger_context<version_check_update_context>(version_check_update_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_VERSION_CHECK_UPDATE, ctx);
    }

    template <> auto trigger_context<version_check_msg_recv_context>(version_check_msg_recv_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_VERSION_CHECK_MSG_RECV, ctx);
    }

    template <> auto trigger_context<get_quest_definition_context>(get_quest_definition_context &ctx) -> void {
      TRIGGER_EVENT(EVENT_ON_GET_QUEST_DEFINITION, ctx);
    }

    template <typename Context, typename Self, typename Hook, typename... Args>
    auto call_original_then_dispatch(Hook &hook, Self *self, void *edx, Args &&...args)
        -> decltype(hook.call_original(self, edx, args...)) {
      auto result = hook.call_original(self, edx, args...);
      Context ctx{self, args..., result};
      trigger_context(ctx);
      return ctx.result;
    }

    template <typename Context, typename Self, typename Hook, typename... Args>
    auto call_original_then_dispatch_void(Hook &hook, Self *self, void *edx, Args &&...args) -> void {
      hook.call_original(self, edx, args...);
      Context ctx{self, args...};
      trigger_context(ctx);
    }

    // =========================================================================
    // Detours & Hooks Definitions
    // =========================================================================

    // 0. OutputDebugString Hook to capture Direct3D and internal driver/game logs
    make_hook<convention_type::stdcall_t, void, LPCSTR> g_output_debug_string;
    auto WINAPI output_debug_string_detour(LPCSTR lpOutputString) -> void {
      ext_client::utils::hook_call_scope active_call;
      static thread_local bool s_guard = false;
      if (!s_guard && lpOutputString && lpOutputString[0]) {
        s_guard = true;
        char buf[512];
        strncpy_s(buf, lpOutputString, _TRUNCATE);
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n')) {
          buf[--len] = '\0';
        }
        if (len > 0) {
          log_msg("[debug_string] %s", buf);
        }
        s_guard = false;
      }
      g_output_debug_string.call_original(lpOutputString);
    }

    make_hook<convention_type::stdcall_t, void, LPCWSTR> g_output_debug_string_w;
    auto WINAPI output_debug_string_w_detour(LPCWSTR lpOutputString) -> void {
      ext_client::utils::hook_call_scope active_call;
      static thread_local bool s_guard = false;
      if (!s_guard && lpOutputString && lpOutputString[0]) {
        s_guard = true;
        char buf[512]{};
        WideCharToMultiByte(CP_UTF8, 0, lpOutputString, -1, buf, sizeof(buf) - 1, nullptr, nullptr);
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n')) {
          buf[--len] = '\0';
        }
        if (len > 0) {
          log_msg("[debug_string] %s", buf);
        }
        s_guard = false;
      }
      g_output_debug_string_w.call_original(lpOutputString);
    }

    // 0.5. TopLevelExceptionFilter Hook to intercept engine crash handler
    make_hook<convention_type::stdcall_t, LONG, PEXCEPTION_POINTERS> g_top_level_filter;
    auto WINAPI top_level_filter_detour(PEXCEPTION_POINTERS ep) -> LONG {
      ext_client::utils::hook_call_scope active_call;
      if (ep && ep->ExceptionRecord) {
        log_msg("[crash_handler] TopLevelExceptionFilter invoked! Code=0x%08X Address=%p TID=%u",
                ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress, GetCurrentThreadId());
        if (ep->ContextRecord) {
          log_msg("[crash_handler] Crash EIP=%08X EBP=%08X ESP=%08X EAX=%08X EBX=%08X ECX=%08X EDX=%08X",
                  ep->ContextRecord->Eip, ep->ContextRecord->Ebp, ep->ContextRecord->Esp,
                  ep->ContextRecord->Eax, ep->ContextRecord->Ebx, ep->ContextRecord->Ecx, ep->ContextRecord->Edx);
        }
        ext_client::utils::log_flush();
      }
      return g_top_level_filter.call_original(ep);
    }

    // 1. Assert (BSLib) Hook
    make_hook<convention_type::cdecl_t, bool, int, const char *, const char *, __int16> g_assert_report;
    auto __cdecl assert_report_detour(int line, const char *file, const char *msg, __int16 flags) -> bool {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] assert_report (file=%s line=%d msg=%s flags=0x%04X)",
              file ? file : "null", line, msg ? msg : "null", flags);
      assert_report_context ctx{line, file, msg, flags, false, false};
      TRIGGER_EVENT(EVENT_ON_ASSERT_REPORT, ctx);
      if (ctx.handled) {
        return ctx.result;
      }
      return g_assert_report.call_original(line, file, msg, flags);
    }

    // 2. Notice Display Hook
    make_hook<convention_type::thiscall_t, int, void *, void *, const void *> g_show_notice;
    auto __fastcall show_notice_detour(void *self, void *edx, const void *msg_obj) -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] show_notice (self=%p msg_obj=%p)", self, msg_obj);
      if (!msg_obj) {
        return g_show_notice.call_original(self, edx, msg_obj);
      }
      auto ref = ext_client::msvc9::wstring_ref::from(msg_obj);
      show_notice_context ctx{self, msg_obj, std::wstring(ref.data(), ref.length()), false, false, L"", 0};
      TRIGGER_EVENT(EVENT_ON_SHOW_NOTICE, ctx);
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
    make_hook<convention_type::thiscall_t, int, void *, void *, void *> g_populate_target;
    make_hook<convention_type::thiscall_t, void, void *, void *, void *> g_populate_special_mob;
    make_hook<convention_type::thiscall_t, unsigned int, void *, void *> g_update_special_mob;

    auto __fastcall populate_target_detour(void *self, void *edx, void *target_id) -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] populate_target (self=%p target_id=%p)", self, target_id);
      return call_original_then_dispatch<populate_target_context>(g_populate_target, self, edx, target_id);
    }

    auto __fastcall populate_special_mob_detour(void *self, void *edx, void *target_id) -> void {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] populate_special_mob (self=%p target_id=%p)", self, target_id);
      call_original_then_dispatch_void<populate_special_mob_context>(g_populate_special_mob, self, edx, target_id);
    }

    auto __fastcall update_special_mob_detour(void *self, void *edx) -> unsigned int {
      ext_client::utils::hook_call_scope active_call;
      static bool s_logged_update_mob = false;
      if (!s_logged_update_mob) {
        s_logged_update_mob = true;
        log_msg("[client_hooks] update_special_mob first call (self=%p)", self);
      }
      return call_original_then_dispatch<update_special_mob_context>(g_update_special_mob, self, edx);
    }

    // 4. Character Select Screen Hooks
    make_hook<convention_type::thiscall_t, int, cps_character_select *, void *, cprocess_msg *> g_char_select_on_enter;
    make_hook<convention_type::thiscall_t, int, cps_character_select *, void *, int, int> g_char_select_on_slot_change;
    make_hook<convention_type::thiscall_t, int, cps_character_select *, void *, cmsg_stream_buffer *>
        g_char_select_on_msg_recv;
    make_hook<convention_type::thiscall_t, int, cps_character_select *, void *> g_char_select_on_update;

    auto __fastcall char_select_on_enter_detour(cps_character_select *self, void *edx, cprocess_msg *msg) -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] char_select_on_enter (self=%p msg=%p)", self, msg);
      return call_original_then_dispatch<char_select_enter_context>(g_char_select_on_enter, self, edx, msg);
    }

    auto __fastcall char_select_on_slot_change_detour(cps_character_select *self, void *edx, int a2, int a3) -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] char_select_on_slot_change (self=%p slot=%d a3=%d)", self, a2, a3);
      return call_original_then_dispatch<char_select_slot_change_context>(g_char_select_on_slot_change, self, edx, a2,
                                                                          a3);
    }

    auto __fastcall char_select_on_msg_recv_detour(cps_character_select *self, void *edx, cmsg_stream_buffer *packet)
        -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] char_select_on_msg_recv (self=%p packet=%p)", self, packet);
      return call_original_then_dispatch<char_select_msg_recv_context>(g_char_select_on_msg_recv, self, edx, packet);
    }

    auto __fastcall char_select_on_update_detour(cps_character_select *self, void *edx) -> int {
      ext_client::utils::hook_call_scope active_call;
      static bool s_logged_cs_update = false;
      if (!s_logged_cs_update) {
        s_logged_cs_update = true;
        log_msg("[client_hooks] char_select_on_update first call (self=%p)", self);
      }
      return call_original_then_dispatch<char_select_update_context>(g_char_select_on_update, self, edx);
    }

    // 5. Loading & Version Check Screen Hooks
    make_hook<convention_type::thiscall_t, char, cps_version_check *, void *, int> g_version_check_on_create;
    make_hook<convention_type::thiscall_t, void, cps_version_check *, void *> g_version_check_on_update;
    make_hook<convention_type::thiscall_t, int, cps_version_check *, void *, cmsg_stream_buffer *>
        g_version_check_on_msg_recv;
    make_hook<convention_type::thiscall_t, int, void *, void *, int, int> g_set_child_process;
    make_hook<convention_type::cdecl_t, int, int, int, char> g_load_intro_camera;

    auto __fastcall version_check_on_create_detour(cps_version_check *self, void *edx, int process_type) -> char {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] version_check_on_create (self=%p process_type=%d)", self, process_type);
      return call_original_then_dispatch<version_check_create_context>(g_version_check_on_create, self, edx,
                                                                       process_type);
    }

    auto __fastcall version_check_on_update_detour(cps_version_check *self, void *edx) -> void {
      ext_client::utils::hook_call_scope active_call;
      static bool s_logged_vc_update = false;
      if (!s_logged_vc_update) {
        s_logged_vc_update = true;
        log_msg("[client_hooks] version_check_on_update first call (self=%p)", self);
      }
      call_original_then_dispatch_void<version_check_update_context>(g_version_check_on_update, self, edx);
    }

    auto __fastcall version_check_on_msg_recv_detour(cps_version_check *self, void *edx, cmsg_stream_buffer *packet)
        -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] version_check_on_msg_recv (self=%p packet=%p)", self, packet);
      return call_original_then_dispatch<version_check_msg_recv_context>(g_version_check_on_msg_recv, self, edx,
                                                                         packet);
    }

    auto __fastcall set_child_process_detour(void *mgr, void *edx, int process_type, int activate) -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] set_child_process (mgr=%p process_type=%d activate=%d)", mgr, process_type, activate);
      const int result = g_set_child_process.call_original(mgr, edx, process_type, activate);

      set_child_process_context ctx{process_type, activate};
      TRIGGER_EVENT(EVENT_ON_SET_CHILD_PROCESS, ctx);

      void *child = ccontroler::active_child();
      if (child == nullptr && result != 0) {
        child = reinterpret_cast<void *>(result);
      }
      log_msg("[client_hooks] set_child_process completed (child=%p)", child);
      return result;
    }

    auto __cdecl load_intro_camera_detour(int a1, int a2, char a3) -> int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] load_intro_camera (a1=%d a2=%d a3=%d)", a1, a2, static_cast<int>(a3));
      int result = g_load_intro_camera.call_original(a1, a2, a3);
      load_intro_camera_context ctx{a1, a2, a3, result};
      TRIGGER_EVENT(EVENT_ON_LOAD_INTRO_CAMERA, ctx);
      return ctx.result;
    }

    // 6. Quest Hook
    make_hook<convention_type::thiscall_t, void *, void *, void *, unsigned int> g_get_quest_definition;
    auto __fastcall get_quest_definition_detour(void *self, void *edx, unsigned int quest_id) -> void * {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] get_quest_definition (self=%p quest_id=%u)", self, quest_id);
      return call_original_then_dispatch<get_quest_definition_context>(g_get_quest_definition, self, edx, quest_id);
    }

    // 7. Promo & Guide Hooks
    make_hook<convention_type::thiscall_t, unsigned int, void *, void *, char> g_show_facebook_guide;
    make_hook<convention_type::thiscall_t, unsigned int, void *, void *, char> g_show_magic_lamp_guide;
    make_hook<convention_type::thiscall_t, unsigned int, void *, void *, char> g_show_daily_login_guide;
    make_hook<convention_type::thiscall_t, unsigned int, void *, void *, char> g_show_web_item_alarm_guide;
    make_hook<convention_type::thiscall_t, unsigned int, void *, void *, char> g_show_macro_guide;

    auto __fastcall show_facebook_guide_detour(void *self, void *edx, char show) -> unsigned int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] show_facebook_guide (self=%p show=%d)", self, static_cast<int>(show));
      if (ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
        if (ext_client::core::config::data().interface_hide.hide_facebook && show != 0) {
          return g_show_facebook_guide.call_original(self, edx, 0);
        }
      }
      return g_show_facebook_guide.call_original(self, edx, show);
    }

    auto __fastcall show_magic_lamp_guide_detour(void *self, void *edx, char show) -> unsigned int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] show_magic_lamp_guide (self=%p show=%d)", self, static_cast<int>(show));
      if (ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
        if (ext_client::core::config::data().interface_hide.hide_magic_lamp && show != 0) {
          return g_show_magic_lamp_guide.call_original(self, edx, 0);
        }
      }
      return g_show_magic_lamp_guide.call_original(self, edx, show);
    }

    auto __fastcall show_daily_login_guide_detour(void *self, void *edx, char show) -> unsigned int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] show_daily_login_guide (self=%p show=%d)", self, static_cast<int>(show));
      if (ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
        if (ext_client::core::config::data().interface_hide.hide_daily_login && show != 0) {
          return g_show_daily_login_guide.call_original(self, edx, 0);
        }
      }
      return g_show_daily_login_guide.call_original(self, edx, show);
    }

    auto __fastcall show_web_item_alarm_guide_detour(void *self, void *edx, char show) -> unsigned int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] show_web_item_alarm_guide (self=%p show=%d)", self, static_cast<int>(show));
      if (ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
        if (ext_client::core::config::data().interface_hide.hide_web_item_alarm && show != 0) {
          return g_show_web_item_alarm_guide.call_original(self, edx, 0);
        }
      }
      return g_show_web_item_alarm_guide.call_original(self, edx, show);
    }

    auto __fastcall show_macro_guide_detour(void *self, void *edx, char show) -> unsigned int {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[client_hooks] show_macro_guide (self=%p show=%d)", self, static_cast<int>(show));
      if (ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
        if (ext_client::core::config::data().interface_hide.hide_macro_guide && show != 0) {
          return g_show_macro_guide.call_original(self, edx, 0);
        }
      }
      return g_show_macro_guide.call_original(self, edx, show);
    }

  } // namespace
  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    // 0. OutputDebugString hook
    const auto output_debug_string_fn =
        reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "OutputDebugStringA"));
    if (output_debug_string_fn) {
      if (g_hooks.install(g_output_debug_string, output_debug_string_fn, &output_debug_string_detour, "core_hooks",
                          "OutputDebugStringA")) {
        log_msg("[client_hooks] hooked OutputDebugStringA @ 0x%08X", output_debug_string_fn);
      }
    }
    const auto output_debug_string_w_fn =
        reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "OutputDebugStringW"));
    if (output_debug_string_w_fn) {
      if (g_hooks.install(g_output_debug_string_w, output_debug_string_w_fn, &output_debug_string_w_detour,
                          "core_hooks", "OutputDebugStringW")) {
        log_msg("[client_hooks] hooked OutputDebugStringW @ 0x%08X", output_debug_string_w_fn);
      }
    }

    if (g_hooks.install(g_top_level_filter, 0x0092E550, &top_level_filter_detour, "core_hooks", "TopLevelExceptionFilter")) {
      log_msg("[client_hooks] hooked TopLevelExceptionFilter @ 0x0092E550");
    }

    // 1. Assert & Notices
    if (!g_hooks.install_all(g_assert_report, 0x00D898A0, &assert_report_detour, "core_hooks", "assert_report",
                             g_show_notice, 0x0085D7B0, &show_notice_detour, "core_hooks", "show_notice")) {
      return false;
    }

    // 2. Targets
    if (!g_hooks.install_all(g_populate_target, 0x007E6B60, &populate_target_detour, "core_hooks", "populate_target",
                             g_populate_special_mob, 0x007E8EB0, &populate_special_mob_detour, "core_hooks",
                             "populate_special_mob", g_update_special_mob, 0x007E8C30, &update_special_mob_detour,
                             "core_hooks", "update_special_mob")) {
      return false;
    }

    // 3. Character Select
    if (!g_hooks.install_all(g_char_select_on_enter, 0x0095EB60, &char_select_on_enter_detour, "core_hooks",
                             "char_select_on_enter", g_char_select_on_slot_change, 0x00963270,
                             &char_select_on_slot_change_detour, "core_hooks", "char_select_on_slot_change",
                             g_char_select_on_msg_recv, 0x00962800, &char_select_on_msg_recv_detour, "core_hooks",
                             "char_select_on_msg_recv", g_char_select_on_update, 0x00960610,
                             &char_select_on_update_detour, "core_hooks", "char_select_on_update")) {
      return false;
    }

    // 4. Version Check
    if (!g_hooks.install_all(g_version_check_on_create, 0x00978DE0, &version_check_on_create_detour, "core_hooks",
                             "version_check_on_create", g_version_check_on_update, 0x00978CA0,
                             &version_check_on_update_detour, "core_hooks", "version_check_on_update",
                             g_version_check_on_msg_recv, 0x00978D20, &version_check_on_msg_recv_detour, "core_hooks",
                             "version_check_on_msg_recv", g_set_child_process, 0x00D729E0, &set_child_process_detour,
                             "core_hooks", "set_child_process", g_load_intro_camera, 0x0093EDE0,
                             &load_intro_camera_detour, "core_hooks", "load_intro_camera")) {
      return false;
    }

    // 5. Quests
    if (!g_hooks.install_all(g_get_quest_definition, 0x00A75820, &get_quest_definition_detour, "core_hooks",
                             "get_quest_definition")) {
      return false;
    }

    // 6. Promo & Guides
    if (!g_hooks.install_all(
            g_show_facebook_guide, 0x00884720, &show_facebook_guide_detour, "core_hooks", "show_facebook_guide",
            g_show_magic_lamp_guide, 0x008844C0, &show_magic_lamp_guide_detour, "core_hooks", "show_magic_lamp_guide",
            g_show_daily_login_guide, 0x008844F0, &show_daily_login_guide_detour, "core_hooks", "show_daily_login_guide",
            g_show_web_item_alarm_guide, 0x00883500, &show_web_item_alarm_guide_detour, "core_hooks", "show_web_item_alarm_guide",
            g_show_macro_guide, 0x00884170, &show_macro_guide_detour, "core_hooks", "show_macro_guide")) {
      return false;
    }

    return true;
  }
  auto uninstall() -> bool {
    return g_hooks.uninstall();
  }
} // namespace ext_client::core::hooks::client_hooks
