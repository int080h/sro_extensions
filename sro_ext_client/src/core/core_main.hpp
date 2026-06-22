#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <windows.h>

namespace ext_client {

  enum class shutdown_mode {
    graceful_unload,
    terminate_after_cleanup,
    force_terminate,
  };

  enum class packet_direction : std::uint8_t {
    client_to_server = 0,
    server_to_client = 1,
  };

} // namespace ext_client

namespace ext_client::core {

  class app_main {
  public:
    static auto get() -> app_main&;

    auto run(HMODULE module) -> void;
    auto request_unload() noexcept -> void;
    auto shutdown(shutdown_mode mode) -> void;
    [[nodiscard]] auto is_running() const noexcept -> bool;
    [[nodiscard]] auto should_unload() const noexcept -> bool;
    [[nodiscard]] auto get_module() const noexcept -> HMODULE;
    auto cleanup() -> void;
    auto set_main_thread_id(DWORD thread_id) noexcept -> void;
    [[nodiscard]] auto get_main_thread_id() const noexcept -> DWORD;

  private:
    app_main() = default;
    ~app_main() = default;

    app_main(const app_main&) = delete;
    app_main& operator=(const app_main&) = delete;
    app_main(app_main&&) = delete;
    app_main& operator=(app_main&&) = delete;

    auto run_shutdown_body() -> void;

    HMODULE m_module{nullptr};
    std::atomic<bool> m_is_running{false};
    std::atomic<bool> m_should_unload{false};
    DWORD m_main_thread_id{0};
    std::once_flag m_shutdown_once{};
  };

} // namespace ext_client::core
