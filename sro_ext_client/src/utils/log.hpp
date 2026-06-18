#pragma once

namespace ext_client::utils {

  /**
 * @brief Allocates a console and redirects stdout, stderr, and stdin to it.
 * Also clears the old ext_client.log file.
 */
  auto log_init() -> void;

  /**
 * @brief Prints a formatted message to the console and appends it to ext_client.log.
 * @param fmt Format string.
 * @param ... Format arguments.
 */
  auto log_msg(const char* fmt, ...) -> void;

  auto log_shutdown() -> void;

} // namespace ext_client::utils

#define LOG_IF(condition, ...)            \
  do {                                    \
    if (condition) {                      \
      ext_client::utils::log_msg(__VA_ARGS__); \
    }                                     \
  } while (0)
