#include "utils/process.hpp"

#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace ext_client::utils::process {

  auto current_exe_name() -> std::string {
    char path[MAX_PATH]{};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    const std::string exe = path;
    const auto slash = exe.find_last_of("/\\");
    const std::string name = slash == std::string::npos ? exe : exe.substr(slash + 1);
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower;
  }

  auto exe_directory(char* dst, std::size_t dst_count) -> bool {
    if (!dst || dst_count == 0) {
      return false;
    }
    dst[0] = '\0';
    if (GetModuleFileNameA(nullptr, dst, static_cast<DWORD>(dst_count)) == 0) {
      return false;
    }
    char* slash = std::strrchr(dst, '\\');
    if (!slash) {
      slash = std::strrchr(dst, '/');
    }
    if (slash) {
      slash[1] = '\0';
    }
    return true;
  }

  auto is_current_process(const char* exe_name) -> bool {
    return current_exe_name() == exe_name;
  }

  // ---------------------------------------------------------------------------
  // Game process manager (merged from process_manager.cpp)
  // ---------------------------------------------------------------------------

  auto instance() -> void* {
    if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::process_manager::address), sizeof(void*))) {
      return nullptr;
    }
    return ext_client::offsets::global_at<void*>(ext_client::offsets::process_manager::address);
  }

  auto active_child() -> void* {
    auto* mgr = instance();
    if (!mgr || !ext_client::msvc9::is_readable_ptr(
                  reinterpret_cast<const std::uint8_t*>(mgr) + ext_client::offsets::process_manager::fields::active_child, sizeof(void*))) {
      return nullptr;
    }
    return ext_client::offsets::field_at<void*>(mgr, ext_client::offsets::process_manager::fields::active_child);
  }

  auto active_child_vftable() -> std::uint32_t {
    const void* child = active_child();
    std::uint32_t vft = 0;
    if (!child || !ext_client::msvc9::try_read_u32(child, &vft)) {
      return 0;
    }
    return vft;
  }

  auto cached_active_child() -> void*& {
    static void* ptr = nullptr;
    return ptr;
  }

  auto is_child_readable(void* child) -> bool {
    if (!child || !ext_client::msvc9::is_readable_ptr(child, sizeof(void*) * 4)) {
      return false;
    }
    std::uint32_t vft = 0;
    if (!ext_client::msvc9::try_read_u32(child, &vft)) {
      return false;
    }
    return ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(vft)), sizeof(void*) * 4);
  }

  auto resolved_active_child() -> void* {
    if (auto* live = active_child()) {
      cached_active_child() = live;
      return live;
    }
    if (is_child_readable(cached_active_child())) {
      return cached_active_child();
    }
    cached_active_child() = nullptr;
    return nullptr;
  }

  auto note_set_child_process(void* child, int activate) -> void {
    if (activate != 0 && child != nullptr && is_child_readable(child)) {
      cached_active_child() = child;
    }
  }

  auto set_child_process(int process_type, bool activate) -> int {
    auto* mgr = instance();
    if (!mgr) {
      return 0;
    }
    using set_child_process_fn = int(__thiscall*)(void* process_mgr, int process_type, int activate);
    const auto fn = ext_client::offsets::as_fn<set_child_process_fn>(ext_client::offsets::process_manager::functions::set_child_process);
    return fn(mgr, process_type, activate ? 1 : 0);
  }

  auto quit_process() -> void {
    const auto fn = ext_client::offsets::as_fn<void(__cdecl*)()>(ext_client::offsets::process_manager::functions::quit_process);
    fn();
  }

} // namespace ext_client::utils::process
