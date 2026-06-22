#pragma once

#include <windows.h>
#include <cstddef>
#include <string>

namespace ext_client::utils::process {

  /**
   * @brief Retrieves the lowercase filename of the current process executable.
   * @return The lowercase process executable name.
   */
  auto current_exe_name() -> std::string;

  /**
   * @brief Retrieves the directory of the currently running executable (including trailing backslash).
   * @param dst Destination buffer.
   * @param dst_count Size of destination buffer in characters.
   * @return True on success, false on failure.
   */
  auto exe_directory(char* dst, std::size_t dst_count) -> bool;

  /**
   * @brief Checks if the current process matches the specified executable name (case-insensitive check).
   * @param exe_name Executable name to compare (should be lowercase).
   * @return True if matches, false otherwise.
   */
  auto is_current_process(const char* exe_name) -> bool;

  /**
   * @brief Resolves the main window handle for SRO_CLIENT.
   * Caches the result to avoid repeated slow FindWindowA calls.
   */
  [[nodiscard]] auto resolve_main_window() noexcept -> HWND;

  namespace shutdown_guard {

    /**
     * @brief Arms the shutdown watchdog guard.
     */
    auto arm(const char* reason) -> void;

    /**
     * @brief Disarms the shutdown watchdog guard.
     */
    auto disarm() -> void;

    /**
     * @brief Polls the watchdog and forces process termination if shutdown has stalled.
     */
    auto poll() -> void;

    /**
     * @brief Checks if the shutdown watchdog guard is armed.
     */
    auto is_armed() -> bool;

  } // namespace shutdown_guard

} // namespace ext_client::utils::process
