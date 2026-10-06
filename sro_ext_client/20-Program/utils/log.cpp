#include "pch.hpp"
#include "utils/log.hpp"
#include <cstdarg>
#include <deque>

namespace ext_client::utils {
  namespace {
    std::mutex g_mutex;
    std::mutex g_io_mutex;
    std::deque<std::string> g_pending;
    FILE *g_file = nullptr;
    bool g_accepting = false;
    std::size_t g_dropped = 0;
  } // namespace
  auto log_init() -> void {
    std::lock_guard lock(g_mutex);
    AllocConsole();
    FILE *stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
    SetConsoleTitleA("sro_ext_client");
    fopen_s(&g_file, "ext_client.log", "w");
    g_accepting = true;
  }
  auto log_msg(const char *fmt, ...) -> void {
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    std::lock_guard io_lock(g_io_mutex);
    std::lock_guard lock(g_mutex);
    if (!g_accepting)
      return;

    std::puts(buffer);
    if (g_file) {
      std::fputs(buffer, g_file);
      std::fputc('\n', g_file);
      std::fflush(g_file);
    }
    std::fflush(stdout);
  }
  auto log_flush() -> void {
    std::lock_guard io_lock(g_io_mutex);
    std::fflush(stdout);
    if (g_file)
      std::fflush(g_file);
  }
  auto log_shutdown() -> void {
    {
      std::lock_guard lock(g_mutex);
      g_accepting = false;
    }
    log_flush();
    std::lock_guard lock(g_io_mutex);
    if (g_file) {
      std::fclose(g_file);
      g_file = nullptr;
    }
    FreeConsole();
  }
} // namespace ext_client::utils
