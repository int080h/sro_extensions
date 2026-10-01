#include "pch.hpp"
#include "core/core_main.hpp"

#include "core/core_config.hpp"
#include "core/core_hooks.hpp"
#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "render/render_system.hpp"
#include "render/loading_splash_overlay.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/process.hpp"

#include <chrono>
#include <thread>

using ext_client::utils::log_msg;

namespace {

  DWORD WINAPI init_thread_proc(LPVOID param) {
    ext_client::core::app_main::get().run(static_cast<HMODULE>(param));
    return 0;
  }
} // namespace

namespace ext_client::core {

  auto app_main::get() -> app_main & {
    static app_main instance;
    return instance;
  }

  auto app_main::run(HMODULE module) -> void {
    if (!ext_client::utils::process::is_current_process("sro_client.exe")) {
      return;
    }

    m_module = module;
    m_is_running.store(true, std::memory_order_release);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ext_client::utils::log_init();
    log_msg("[core_main] client core thread started");

    if (!ext_client::utils::hook_lib_init()) {
      log_msg("[core_main] critical error: hook library initialization failed");
      m_is_running.store(false, std::memory_order_release);
      return;
    }
    log_msg("[core_main] hook library initialized successfully");

    ext_client::core::plugin::plugin_manager::get().initialize_all();

    ext_client::core::config::load();
    ext_client::core::config::publish_runtime();

    if (!ext_client::core::hooks::core_hooks::install_all()) {
      log_msg("[core_main] warning: some hooks failed to install");
    } else {
      log_msg("[core_main] all core hooks installed successfully");
    }

    while (!m_should_unload.load(std::memory_order_acquire)) {
      if (GetAsyncKeyState(VK_F7) & 0x8000) {
        log_msg("[core_main] unload requested via F7 hotkey");
        m_should_unload.store(true, std::memory_order_release);
        break;
      }

      if (!ext_client::core::hooks::core_hooks::is_render_installed()) {
        ext_client::core::hooks::core_hooks::install_lazy();
      }

      ext_client::utils::process::shutdown_guard::poll();
      ext_client::core::config::flush_pending();
      TRIGGER_EVENT(EVENT_ON_BACKGROUND_TICK);
      ext_client::utils::log_flush();

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    log_msg("[core_main] initiating shutdown sequence");
    const bool exiting = m_exit_requested.load(std::memory_order_acquire);
    shutdown(exiting ? shutdown_mode::terminate_after_cleanup : shutdown_mode::graceful_unload);

    m_is_running.store(false, std::memory_order_release);

    if (m_exit_requested.load(std::memory_order_acquire))
      TerminateProcess(GetCurrentProcess(), m_exit_code.load());
    if (m_shutdown_complete)
      FreeLibraryAndExitThread(m_module, 0);
    // A failed unpatch/drain leaves this DLL resident so no thread can return into unmapped code.
    ext_client::utils::log_flush();
  }

  auto app_main::request_unload() noexcept -> void {
    m_should_unload.store(true, std::memory_order_release);
  }

  auto app_main::request_exit(UINT exit_code) noexcept -> void {
    m_exit_code.store(exit_code);
    m_exit_requested.store(true, std::memory_order_release);
    request_unload();
  }

  auto app_main::run_shutdown_body() -> void {
    log_msg("[core_main] running shutdown sequence");
    utils::stop_hook_installation();
    event::stop_dispatch();
    if (!render::render_system::get().detach_input() || !render::loading_splash_overlay::stop()) {
      log_msg("[core_main] input or splash window still active; DLL will remain resident");
      return;
    }
    const bool unpatched = hooks::core_hooks::uninstall_all();
    if (!unpatched || !utils::wait_for_hook_calls(5000)) {
      log_msg("[core_main] shutdown could not drain hooks; DLL will remain resident");
      return;
    }
    TRIGGER_EVENT(EVENT_ON_SHUTDOWN);
    config::save();
    if (!render::render_system::get().uninstall()) {
      log_msg("[core_main] render input still attached; DLL will remain resident");
      return;
    }
    if (!utils::hook_lib_shutdown(m_module)) {
      log_msg("[core_main] shutdown could not prove code is idle; DLL will remain resident");
      return;
    }
    m_shutdown_complete = true;
    log_msg("[core_main] shutdown complete");
    utils::log_shutdown();
  }

  auto app_main::shutdown(shutdown_mode mode) -> void {
    if (mode == shutdown_mode::force_terminate) {
      // The watchdog must not wait on a stalled game thread.
      utils::log_flush();
      return;
    }
    utils::process::shutdown_guard::disarm();
    std::call_once(m_shutdown_once, [this] { run_shutdown_body(); });
  }

  auto app_main::cleanup() -> void {
    shutdown(shutdown_mode::graceful_unload);
  }

  auto app_main::is_running() const noexcept -> bool {
    return m_is_running.load(std::memory_order_acquire);
  }

  auto app_main::should_unload() const noexcept -> bool {
    return m_should_unload.load(std::memory_order_acquire);
  }

  auto app_main::get_module() const noexcept -> HMODULE {
    return m_module;
  }

  auto app_main::set_main_thread_id(DWORD thread_id) noexcept -> void {
    m_main_thread_id = thread_id;
  }

  auto app_main::get_main_thread_id() const noexcept -> DWORD {
    return m_main_thread_id;
  }

} // namespace ext_client::core

// Never wait for a thread or run application cleanup under the loader lock.
auto WINAPI DllMain(HMODULE module, DWORD reason, LPVOID) -> BOOL {
  if (reason == DLL_PROCESS_ATTACH) {
    ext_client::core::app_main::get().set_main_thread_id(GetCurrentThreadId());
    const HANDLE thread = CreateThread(nullptr, 0, init_thread_proc, module, 0, nullptr);
    if (!thread)
      return FALSE;
    CloseHandle(thread);
  }
  return TRUE;
}
