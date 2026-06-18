#include "ext_client.hpp"

#include "config/client_config.hpp"
#include "hooks/d3d_hook.hpp"
#include "hooks/hook_manager.hpp"
#include "sdk/net_manager.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/process.hpp"
#include "utils/shutdown_guard.hpp"
#include "utils/window_style.hpp"

#include <Windows.h>

#include <chrono>
#include <thread>

using ext_client::utils::log_msg;

namespace ext_client {

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

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

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

    while (!m_should_unload.load(std::memory_order_acquire)) {
      if (GetAsyncKeyState(VK_F7) & 0x8000) {
        ext_client::utils::log_msg("[ext_client] unload requested via F7 hotkey");
        m_should_unload.store(true, std::memory_order_release);
        break;
      }

      if (!ext_client::hooks::d3d::is_installed()) {
        ext_client::hooks::manager::try_install_lazy();
      }

      ext_client::utils::window_style::ensure_main_window_minimize_button();
      ext_client::utils::shutdown_guard::poll();

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    ext_client::utils::log_msg("[ext_client] initiating shutdown sequence");
    shutdown(shutdown_mode::graceful_unload);
    ext_client::utils::log_msg("[ext_client] shutdown complete");

    m_is_running.store(false, std::memory_order_release);

    FreeLibraryAndExitThread(m_module, 0);
  }

  auto core::tick() -> void {
    ext_client::hooks::manager::tick();
  }

  auto core::request_unload() noexcept -> void {
    m_should_unload.store(true, std::memory_order_release);
  }

  auto core::run_shutdown_body() -> void {
    ext_client::utils::log_msg("[ext_client] running shutdown sequence");
    ext_client::config::save();
    ext_client::hooks::manager::uninstall_all();
    ext_client::net_manager::shutdown();
    ext_client::utils::hook_lib_shutdown();
    ext_client::utils::log_shutdown();
  }

  auto core::shutdown(shutdown_mode mode) -> void {
    (void)mode;
    ext_client::utils::shutdown_guard::disarm();
    std::call_once(m_shutdown_once, [this] { run_shutdown_body(); });
  }

  auto core::cleanup() -> void {
    shutdown(shutdown_mode::graceful_unload);
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

} // namespace ext_client
