#pragma once

#include "config/client_config.hpp"

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

  /**
   * @class core
   * @brief Thread-safe singleton managing the client extension's lifecycle, state, and hooks.
   */
  class core {
  public:
    static auto get() -> core&;

    auto run(HMODULE module) -> void;
    auto tick() -> void;
    auto request_unload() noexcept -> void;
    auto shutdown(shutdown_mode mode) -> void;
    [[nodiscard]] auto is_running() const noexcept -> bool;
    [[nodiscard]] auto should_unload() const noexcept -> bool;
    [[nodiscard]] auto get_module() const noexcept -> HMODULE;
    auto cleanup() -> void;
    auto set_main_thread_id(DWORD thread_id) noexcept -> void;
    [[nodiscard]] auto get_main_thread_id() const noexcept -> DWORD;

  private:
    core() = default;
    ~core() = default;

    core(const core&) = delete;
    core& operator=(const core&) = delete;
    core(core&&) = delete;
    core& operator=(core&&) = delete;

    auto run_shutdown_body() -> void;

    HMODULE m_module{nullptr};
    std::atomic<bool> m_is_running{false};
    std::atomic<bool> m_should_unload{false};
    DWORD m_main_thread_id{0};
    std::once_flag m_shutdown_once{};
  };

} // namespace ext_client
