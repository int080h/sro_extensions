#include "utils/log.hpp"

#include <windows.h>
#include <cstdarg>
#include <cstdio>

namespace ext_client::utils {

  namespace {
    FILE* g_log_file = nullptr;
    CRITICAL_SECTION g_log_cs;
    bool g_cs_initialized = false;
  }

  auto log_init() -> void {
    if (!g_cs_initialized) {
      InitializeCriticalSection(&g_log_cs);
      g_cs_initialized = true;
    }

    AllocConsole();
    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
    SetConsoleTitleA("sro_ext_client");

    // Open persistent log file in write mode to clear previous logs
    EnterCriticalSection(&g_log_cs);
    fopen_s(&g_log_file, "ext_client.log", "w");
    LeaveCriticalSection(&g_log_cs);

    log_msg("console ready");
  }

  auto log_shutdown() -> void {
    EnterCriticalSection(&g_log_cs);
    if (g_log_file) {
      std::fclose(g_log_file);
      g_log_file = nullptr;
    }
    LeaveCriticalSection(&g_log_cs);

    FreeConsole();

    if (g_cs_initialized) {
      DeleteCriticalSection(&g_log_cs);
      g_cs_initialized = false;
    }
  }

  auto log_msg(const char* fmt, ...) -> void {
    va_list args;

    // Log to allocated console
    va_start(args, fmt);
    std::vprintf(fmt, args);
    std::printf("\n");
    std::fflush(stdout);
    va_end(args);

    // Log to file
    va_start(args, fmt);
    if (g_cs_initialized) {
      EnterCriticalSection(&g_log_cs);
    }
    if (g_log_file) {
      std::vfprintf(g_log_file, fmt, args);
      std::fprintf(g_log_file, "\n");
      std::fflush(g_log_file);
    }
    if (g_cs_initialized) {
      LeaveCriticalSection(&g_log_cs);
    }
    va_end(args);
  }

} // namespace ext_client::utils
