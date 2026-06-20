#pragma once

#include <cstddef>
#include <cstdint>
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

} // namespace ext_client::utils::process
