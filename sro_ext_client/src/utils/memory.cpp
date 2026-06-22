#include "utils/memory.hpp"

#include <windows.h>

namespace ext_client::utils::memory {

  auto is_readable_ptr(const void* ptr) noexcept -> bool {
    if (!is_game_ptr(ptr)) {
      return false;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(ptr, &info, sizeof(info)) != sizeof(info)) {
      return false;
    }
    return info.State == MEM_COMMIT && 
           !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
           (info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
  }

  auto is_code_ptr(const void* ptr) noexcept -> bool {
    if (!is_game_ptr(ptr)) {
      return false;
    }
    constexpr DWORD protect_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(ptr, &info, sizeof(info))) {
      return false;
    }
    return info.Type != 0 && 
           !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && 
           (info.Protect & protect_flags) != 0;
  }

} // namespace ext_client::utils::memory
