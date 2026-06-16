#include "core/ext_client.hpp"

#include "config/client_config.hpp"
#include "hooks/d3d_hook.hpp"
#include "hooks/hook_manager.hpp"
#include "sdk/net_manager.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/process.hpp"
#include "utils/shutdown_guard.hpp"
#include "utils/window_style.hpp"
#include "utils/offsets.hpp"

#include <chrono>
#include <thread>

namespace ext_client {

  namespace {
    constexpr auto k_startup_delay = std::chrono::milliseconds(200);
    constexpr auto k_hook_poll_interval = std::chrono::milliseconds(200);
    constexpr int k_unload_key = VK_F7;

    ext_client::utils::make_hook<ext_client::utils::convention_type::stdcall_t, void, UINT> g_exit_process;
    ext_client::utils::make_hook<ext_client::utils::convention_type::thiscall_t, void, void*> g_cps_quit_on_update;
    ext_client::utils::make_hook<ext_client::utils::convention_type::stdcall_t, void, int> g_post_quit_message;

    auto __stdcall exit_process_detour(UINT exit_code) -> void {
      ext_client::utils::log_msg("[ext_client] ExitProcess intercepted (code=%u), forcing immediate termination", exit_code);
      TerminateProcess(GetCurrentProcess(), exit_code);
    }

    auto __fastcall cps_quit_on_update_detour(void* self) -> void {
      ext_client::utils::log_msg("[ext_client] CPSQuit::OnUpdate intercepted, forcing immediate termination");
      ExitProcess(0);
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

      ext_client::utils::log_msg("[ext_client] PostQuitMessage intercepted on main thread (code=%d), initiating cleanup and immediate termination", exit_code);
      ext_client::core::get().cleanup();
      TerminateProcess(GetCurrentProcess(), static_cast<UINT>(exit_code));
    }
  } // namespace

  auto core::get() -> core& {
    static core instance;
    return instance;
  }

  auto core::run(HMODULE module) -> void {
    if (!ext_client::utils::process::is_current_process("sro_client.exe")) {
      return;
    }

    m_module = module;
    m_is_running.store(true, std::memory_order_release);

    std::this_thread::sleep_for(k_startup_delay);

    ext_client::utils::log_init();
    ext_client::utils::log_msg("[ext_client] client core thread started");

    if (!ext_client::utils::hook_lib_init()) {
      ext_client::utils::log_msg("[ext_client] critical error: hook library initialization failed");
      m_is_running.store(false, std::memory_order_release);
      return;
    }
    ext_client::utils::log_msg("[ext_client] hook library initialized successfully");

    ext_client::config::load();
    ext_client::config::sync_to_runtime();

    if (!ext_client::hooks::manager::install_all()) {
      ext_client::utils::log_msg("[ext_client] warning: some hooks failed to install");
    } else {
      ext_client::utils::log_msg("[ext_client] all core hooks installed successfully");
    }

    // Install ExitProcess detour to prevent exit hangs
    const auto exit_process_addr = reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
    if (exit_process_addr) {
      if (g_exit_process.apply(exit_process_addr, &exit_process_detour)) {
        ext_client::utils::log_msg("[ext_client] ExitProcess detour installed successfully");
      } else {
        ext_client::utils::log_msg("[ext_client] warning: ExitProcess detour failed to install");
      }
    }

    // Install PostQuitMessage detour to prevent exit hangs
    const auto post_quit_message_addr = reinterpret_cast<std::uintptr_t>(GetProcAddress(GetModuleHandleA("user32.dll"), "PostQuitMessage"));
    if (post_quit_message_addr) {
      if (g_post_quit_message.apply(post_quit_message_addr, &post_quit_message_detour)) {
        ext_client::utils::log_msg("[ext_client] PostQuitMessage detour installed successfully");
      } else {
        ext_client::utils::log_msg("[ext_client] warning: PostQuitMessage detour failed to install");
      }
    }

    // Install CPSQuit::OnUpdate hook to prevent quit process hang
    const auto cps_quit_on_update_addr = ext_client::offsets::cps_quit::functions::on_update;
    if (g_cps_quit_on_update.apply(cps_quit_on_update_addr, &cps_quit_on_update_detour)) {
      ext_client::utils::log_msg("[ext_client] CPSQuit::OnUpdate hook installed successfully");
    } else {
      ext_client::utils::log_msg("[ext_client] warning: CPSQuit::OnUpdate hook failed to install");
    }

    // Main execution loop: poll for unload hotkey or program requested unload
    while (!m_should_unload.load(std::memory_order_acquire)) {
      if (GetAsyncKeyState(k_unload_key) & 0x8000) {
        ext_client::utils::log_msg("[ext_client] unload requested via F7 hotkey");
        m_should_unload.store(true, std::memory_order_release);
        break;
      }

      if (!ext_client::hooks::d3d::is_installed()) {
        ext_client::hooks::d3d::install();
      }

      ext_client::utils::window_style::ensure_main_window_minimize_button();

      ext_client::utils::shutdown_guard::poll();

      std::this_thread::sleep_for(k_hook_poll_interval);
    }

    ext_client::utils::log_msg("[ext_client] initiating shutdown sequence");
    cleanup();
    ext_client::utils::log_msg("[ext_client] shutdown complete");

    m_is_running.store(false, std::memory_order_release);

    // Free the library and terminate the thread
    FreeLibraryAndExitThread(m_module, 0);
  }

  auto core::tick() -> void {
    ext_client::hooks::manager::tick();
  }

  auto core::request_unload() noexcept -> void {
    m_should_unload.store(true, std::memory_order_release);
  }

  auto core::is_running() const noexcept -> bool {
    return m_is_running.load(std::memory_order_acquire);
  }

  auto core::should_unload() const noexcept -> bool {
    return m_should_unload.load(std::memory_order_acquire);
  }

  auto core::get_module() const noexcept -> HMODULE {
    return m_module;
  }

  auto core::set_main_thread_id(DWORD thread_id) noexcept -> void {
    m_main_thread_id = thread_id;
  }

  auto core::get_main_thread_id() const noexcept -> DWORD {
    return m_main_thread_id;
  }

  auto core::cleanup() -> void {
    ext_client::config::save();
    g_exit_process.disable();
    g_post_quit_message.disable();
    ext_client::hooks::manager::uninstall_all();
    ext_client::net_manager::shutdown();
    ext_client::utils::hook_lib_shutdown();
  }

} // namespace ext_client
