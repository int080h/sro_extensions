#include "core/core_main.hpp"

#include "core/core_config.hpp"
#include "core/core_hooks.hpp"
#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "render/render_system.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/process.hpp"

#include <chrono>
#include <thread>

using ext_client::utils::log_msg;

namespace {
  HANDLE g_init_thread = nullptr;

  DWORD WINAPI init_thread_proc(LPVOID param) {
    ext_client::core::app_main::get().run(static_cast<HMODULE>(param));
    return 0;
  }
} // namespace

namespace ext_client::core {

  auto app_main::get() -> app_main& {
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
    ext_client::core::config::sync_to_runtime();

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

      if (!ext_client::render::render_system::get().is_installed()) {
        ext_client::core::hooks::core_hooks::install_lazy();
      }

      ext_client::utils::process::shutdown_guard::poll();

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    log_msg("[core_main] initiating shutdown sequence");
    shutdown(shutdown_mode::graceful_unload);
    log_msg("[core_main] shutdown complete");

    m_is_running.store(false, std::memory_order_release);

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // thread safety padding
    FreeLibraryAndExitThread(m_module, 0);
  }

  auto app_main::request_unload() noexcept -> void {
    m_should_unload.store(true, std::memory_order_release);
  }

  auto app_main::run_shutdown_body() -> void {
    log_msg("[core_main] running shutdown sequence");
    
    // Dispatch on_shutdown to all plugins
    ext_client::core::event::event_manager::get().dispatch(ext_client::core::event::event_type::on_shutdown, nullptr);

    ext_client::core::config::save();
    ext_client::core::hooks::core_hooks::uninstall_all();
    ext_client::utils::hook_lib_shutdown();
    
    log_msg("[core_main] hook library shut down");
    ext_client::utils::log_shutdown();
  }

  auto app_main::shutdown(shutdown_mode mode) -> void {
    ext_client::utils::process::shutdown_guard::disarm();
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

// DLL Entry Point
auto WINAPI DllMain(HMODULE h_module, DWORD reason, LPVOID) -> BOOL {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(h_module);
      ext_client::core::app_main::get().set_main_thread_id(GetCurrentThreadId());
      g_init_thread = CreateThread(nullptr, 0, init_thread_proc, h_module, 0, nullptr);
      if (!g_init_thread) {
        OutputDebugStringA("[ext_client] failed to create init thread\n");
      }
      break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      ext_client::core::app_main::get().request_unload();
      if (g_init_thread) {
        WaitForSingleObject(g_init_thread, 3000);
        CloseHandle(g_init_thread);
        g_init_thread = nullptr;
      }
      if (ext_client::core::app_main::get().is_running()) {
        ext_client::core::app_main::get().shutdown(ext_client::shutdown_mode::graceful_unload);
      }
      break;
  }
  return TRUE;
}
