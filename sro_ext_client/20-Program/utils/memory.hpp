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
   * @brief Verifies that a pointer satisfies natural alignment (default 4 bytes for 32-bit x86).
   */
  inline auto is_aligned_ptr(const void* ptr, std::size_t align = 4) noexcept -> bool {
    return (reinterpret_cast<std::uintptr_t>(ptr) & (align - 1)) == 0;
  }

  /**
   * @brief Combines user-mode address range (0x10000..0x7FFE0000) and alignment check in a single fast inline branch.
   * Default alignment is 4 bytes (standard for 32-bit x86 object pointers).
   */
  inline auto is_valid_ptr(const void* ptr, std::size_t align = 4) noexcept -> bool {
    if (!ptr) {
      return false;
    }
    const auto addr = reinterpret_cast<std::uintptr_t>(ptr);
    return addr >= 0x10000 && addr <= 0x7FFE0000 && (addr & (align - 1)) == 0;
  }

  /**
   * @brief Queries virtual memory to verify that the pointer points to a committed, readable page.
   * Note: VirtualQuery is a kernel syscall; avoid calling inside per-frame hot loops.
   */
  auto is_readable_ptr(const void* ptr) noexcept -> bool;

  /**
   * @brief Queries virtual memory to verify that the pointer points to an executable (code) page.
   */
  auto is_code_ptr(const void* ptr) noexcept -> bool;

  /**
   * @brief Safely copies memory using SEH (__try / __except) with zero kernel syscalls.
   */
  auto safe_read_bytes(const void* src, void* dst, std::size_t size) noexcept -> bool;

  /**
   * @brief Type-safe zero-overhead memory read guard. Returns false on access violation.
   */
  template<typename T>
  inline auto safe_read(const void* src, T& out_val) noexcept -> bool {
    if (!is_valid_ptr(src, alignof(T))) {
      return false;
    }
    return safe_read_bytes(src, &out_val, sizeof(T));
  }
} // namespace ext_client::utils::memory
