#include "utils/shutdown_guard.hpp"

#include "utils/log.hpp"

#include <Windows.h>

#include <atomic>

namespace ext_client::utils::shutdown_guard {
  namespace {

    constexpr DWORD k_force_terminate_delay_ms = 50;

    std::atomic<bool> g_armed{false};
    std::atomic<DWORD> g_armed_at_ms{0};

    auto now_ms() -> DWORD {
      return GetTickCount();
    }

  } // namespace

  auto arm(const char* reason) -> void {
    bool expected = false;
    if (!g_armed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
      return;
    }

    g_armed_at_ms.store(now_ms(), std::memory_order_release);
    ext_client::utils::log_msg("[shutdown_guard] armed (%s)", reason ? reason : "unknown");
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
    TerminateProcess(GetCurrentProcess(), 0);
  }

  auto is_armed() -> bool {
    return g_armed.load(std::memory_order_acquire);
  }

} // namespace ext_client::utils::shutdown_guard
