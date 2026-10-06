#include "pch.hpp"
#include "core/hooks/engine_hooks.hpp"
#include "core/app.hpp"
#include "utils/hooks.hpp"

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::core::hooks::shutdown_hooks {
  namespace {
    hook_group g_hooks;
    // 8. Quit / Shutdown Hooks
    make_hook<convention_type::stdcall_t, void, UINT> g_exit_process;
    make_hook<convention_type::thiscall_t, void, void *, void *> g_cps_quit_on_update;
    make_hook<convention_type::stdcall_t, void, int> g_post_quit_message;

    auto WINAPI exit_process_detour(UINT exit_code) -> void {
      ext_client::utils::abandon_current_hook_calls();
      log_msg("[shutdown_hooks] ExitProcess intercepted (exit_code=%u)", exit_code);
      app_main::get().request_exit(exit_code);
      // ExitProcess never returns. Cleanup runs on the worker instead of this stack.
      Sleep(INFINITE);
    }
    auto __fastcall cps_quit_on_update_detour(void *, void *) -> void {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[shutdown_hooks] CPSQuit::OnUpdate intercepted");
      app_main::get().request_exit(0);
    }
    auto WINAPI post_quit_message_detour(int exit_code) -> void {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[shutdown_hooks] PostQuitMessage intercepted (exit_code=%d)", exit_code);
      const DWORD main_thread_id = app_main::get().get_main_thread_id();
      if (main_thread_id && GetCurrentThreadId() != main_thread_id) {
        g_post_quit_message.call_original(exit_code);
        return;
      }
      app_main::get().request_exit(static_cast<UINT>(exit_code));
    }
  } // namespace
  auto install() -> bool {
    const auto exit_process =
        reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
    const auto post_quit =
        reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("user32.dll"), "PostQuitMessage"));
    return g_hooks.install_all(g_exit_process, exit_process, &exit_process_detour, "shutdown_hooks", "ExitProcess",
                               g_post_quit_message, post_quit, &post_quit_message_detour, "shutdown_hooks",
                               "PostQuitMessage", g_cps_quit_on_update, 0x0094E130, &cps_quit_on_update_detour,
                               "shutdown_hooks", "CPSQuit::OnUpdate");
  }
  auto uninstall() -> bool {
    return g_hooks.uninstall();
  }
} // namespace ext_client::core::hooks::shutdown_hooks
