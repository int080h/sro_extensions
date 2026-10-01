#pragma once

#include <cstdint>

namespace ext_client::utils::memory {

  /**
   * @brief Checks if a pointer lies within the plausible 32-bit user-mode address space.
   * This is a very fast check that filters null pointers, low stub/sentinel pages, and kernel addresses.
   */
  inline auto is_game_ptr(const void* ptr) noexcept -> bool {
    if (!ptr) {
      return false;
    }
    const auto addr = reinterpret_cast<std::uintptr_t>(ptr);
    return addr >= 0x10000 && addr <= 0x7FFE0000;
  }

  /**
   * @brief Queries virtual memory to verify that the pointer points to a committed, readable page.
   */
  auto is_readable_ptr(const void* ptr) noexcept -> bool;

  /**
   * @brief Queries virtual memory to verify that the pointer points to an executable (code) page.
   */
  auto is_code_ptr(const void* ptr) noexcept -> bool;

} // namespace ext_client::utils::memory
