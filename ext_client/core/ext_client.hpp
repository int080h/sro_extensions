#pragma once

#include <windows.h>
#include <atomic>

namespace ext_client {

  /**
 * @class core
 * @brief Thread-safe singleton managing the client extension's lifecycle, state, and hooks.
 */
  class core {
  public:
    /**
     * @brief Retrieves the global core singleton instance.
     */
    static auto get() -> core&;

    /**
     * @brief Executes the main client thread logic.
     * @param module The HMODULE of the client DLL.
     */
    auto run(HMODULE module) -> void;

    /**
     * @brief Periodically called tick to process frame/hook updates.
     */
    auto tick() -> void;

    /**
     * @brief Signals the core client loop to initiate shutdown.
     */
    auto request_unload() noexcept -> void;

    /**
     * @brief Checks if the client extension is currently running.
     */
    [[nodiscard]] auto is_running() const noexcept -> bool;

    /**
     * @brief Checks if the client has received an unload signal.
     */
    [[nodiscard]] auto should_unload() const noexcept -> bool;

    /**
     * @brief Retrieves the HMODULE of the client DLL.
     */
    [[nodiscard]] auto get_module() const noexcept -> HMODULE;

    /**
     * @brief Cleans up and uninstalls all hooks and managers.
     */
    auto cleanup() -> void;

    /**
     * @brief Sets the tracked main thread ID of the client.
     */
    auto set_main_thread_id(DWORD thread_id) noexcept -> void;

    /**
     * @brief Retrieves the tracked main thread ID of the client.
     */
    [[nodiscard]] auto get_main_thread_id() const noexcept -> DWORD;

  private:
    core() = default;
    ~core() = default;

    // Prevent copy/move operations
    core(const core&) = delete;
    core& operator=(const core&) = delete;
    core(core&&) = delete;
    core& operator=(core&&) = delete;

    HMODULE m_module{nullptr};
    std::atomic<bool> m_is_running{false};
    std::atomic<bool> m_should_unload{false};
    DWORD m_main_thread_id{0};
  };

} // namespace ext_client
