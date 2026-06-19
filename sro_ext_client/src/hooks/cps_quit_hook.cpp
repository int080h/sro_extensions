#include "hooks/cps_quit_hook.hpp"

#include "ext_client.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"

#include <Windows.h>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::cps_quit {
  namespace {

    make_hook<convention_type::stdcall_t, void, UINT> g_exit_process;
    make_hook<convention_type::thiscall_t, void, void*> g_cps_quit_on_update;
    make_hook<convention_type::stdcall_t, void, int> g_post_quit_message;
    hook_group g_hooks;

    auto __stdcall exit_process_detour(UINT exit_code) -> void {
      log_msg("[exit_hooks] ExitProcess intercepted (code=%u), forcing immediate termination", exit_code);
      TerminateProcess(GetCurrentProcess(), exit_code);
    }

    auto __fastcall cps_quit_on_update_detour(void* self) -> void {
      (void)self;
      log_msg("[exit_hooks] CPSQuit::OnUpdate intercepted, initiating cleanup and termination");
      ext_client::core::get().request_unload();
      ext_client::core::get().shutdown(ext_client::shutdown_mode::terminate_after_cleanup);
      TerminateProcess(GetCurrentProcess(), 0);
    }

    auto is_main_thread() -> bool {
      const DWORD current_thread_id = GetCurrentThreadId();
      const DWORD main_thread_id = ext_client::core::get().get_main_thread_id();
      if (main_thread_id != 0) {
        return current_thread_id == main_thread_id;
      }

      const HWND hwnd = FindWindowA(nullptr, "SRO_CLIENT");
      if (hwnd) {
        return current_thread_id == GetWindowThreadProcessId(hwnd, nullptr);
      }
      return false;
    }

    auto __stdcall post_quit_message_detour(int exit_code) -> void {
      if (!is_main_thread()) {
        g_post_quit_message.call_original(exit_code);
        return;
      }

      log_msg(
        "[exit_hooks] PostQuitMessage intercepted on main thread (code=%d), initiating cleanup and immediate termination",
        exit_code);
      ext_client::core::get().request_unload();
      ext_client::core::get().shutdown(ext_client::shutdown_mode::terminate_after_cleanup);
      TerminateProcess(GetCurrentProcess(), static_cast<UINT>(exit_code));
    }

  } // namespace

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    const auto exit_process_addr = reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
    if (!exit_process_addr ||
        !g_hooks.install(g_exit_process, exit_process_addr, &exit_process_detour, "exit_hooks", "ExitProcess")) {
      log_msg("[exit_hooks] warning: ExitProcess detour failed to install");
      g_hooks.uninstall();
      return false;
    }

    const auto post_quit_message_addr =
      reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("user32.dll"), "PostQuitMessage"));
    if (!post_quit_message_addr ||
        !g_hooks.install(g_post_quit_message, post_quit_message_addr, &post_quit_message_detour, "exit_hooks", "PostQuitMessage")) {
      log_msg("[exit_hooks] warning: PostQuitMessage detour failed to install");
      g_hooks.uninstall();
      return false;
    }

    const auto cps_quit_on_update_addr = ext_client::offsets::cps_quit::functions::on_update;
    if (!g_hooks.install(
          g_cps_quit_on_update, cps_quit_on_update_addr, &cps_quit_on_update_detour, "exit_hooks", "CPSQuit::OnUpdate")) {
      log_msg("[exit_hooks] warning: CPSQuit::OnUpdate hook failed to install");
      g_hooks.uninstall();
      return false;
    }

    log_msg("[exit_hooks] hooks installed");
    return true;
  }

  auto uninstall() -> void {
    g_hooks.uninstall();
  }

} // namespace ext_client::hooks::cps_quit
