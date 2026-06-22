#include "utils/process.hpp"
#include "core/core_main.hpp"
#include "utils/log.hpp"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <atomic>

namespace ext_client::utils::process {

  namespace {
    constexpr DWORD k_force_terminate_delay_ms = 50;

    std::atomic<bool> g_armed{false};
    std::atomic<DWORD> g_armed_at_ms{0};

    auto now_ms() -> DWORD {
      return GetTickCount();
    }
  } // namespace

  auto current_exe_name() -> std::string {
    static const std::string cached_exe_name = []() {
      char path[MAX_PATH]{};
      GetModuleFileNameA(nullptr, path, MAX_PATH);
      const std::string exe = path;
      const auto slash = exe.find_last_of("/\\");
      const std::string name = slash == std::string::npos ? exe : exe.substr(slash + 1);
      std::string lower = name;
      std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return lower;
    }();
    return cached_exe_name;
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

  auto resolve_main_window() noexcept -> HWND {
    static HWND cached_hwnd = nullptr;
    if (cached_hwnd && IsWindow(cached_hwnd)) {
      return cached_hwnd;
    }
    cached_hwnd = FindWindowA(nullptr, "SRO_CLIENT");
    return cached_hwnd;
  }

  namespace shutdown_guard {

    auto arm(const char* reason) -> void {
      bool expected = false;
      if (!g_armed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
      }

      g_armed_at_ms.store(now_ms(), std::memory_order_release);
      ext_client::utils::log_msg("[shutdown_guard] armed (%s)", reason ? reason : "unknown");
    }

    auto disarm() -> void {
      if (!g_armed.exchange(false, std::memory_order_acq_rel)) {
        return;
      }

      g_armed_at_ms.store(0, std::memory_order_release);
      ext_client::utils::log_msg("[shutdown_guard] disarmed");
    }

    auto poll() -> void {
      if (!g_armed.load(std::memory_order_acquire)) {
        return;
      }

      const DWORD armed_at = g_armed_at_ms.load(std::memory_order_acquire);
      if (armed_at == 0 || now_ms() - armed_at < k_force_terminate_delay_ms) {
        return;
      }

      ext_client::utils::log_msg("[shutdown_guard] forcing process termination after stalled shutdown");
      ext_client::core::app_main::get().shutdown(ext_client::shutdown_mode::force_terminate);
      TerminateProcess(GetCurrentProcess(), 0);
    }

    auto is_armed() -> bool {
      return g_armed.load(std::memory_order_acquire);
    }

  } // namespace shutdown_guard

} // namespace ext_client::utils::process
