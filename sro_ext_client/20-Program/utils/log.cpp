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
    std::lock_guard lock(g_mutex);
    if (!g_accepting)
      return;
    if (g_pending.size() >= 4096) {
      ++g_dropped;
      return;
    }
    g_pending.emplace_back(buffer);
  }
  auto log_flush() -> void {
    std::lock_guard io_lock(g_io_mutex);
    std::deque<std::string> pending;
    std::size_t dropped;
    {
      std::lock_guard lock(g_mutex);
      pending.swap(g_pending);
      dropped = std::exchange(g_dropped, 0);
    }
    for (const auto &line : pending) {
      std::puts(line.c_str());
      if (g_file) {
        std::fputs(line.c_str(), g_file);
        std::fputc('\n', g_file);
      }
    }
    if (dropped) {
      std::printf("[log] dropped %zu messages\n", dropped);
      if (g_file)
        std::fprintf(g_file, "[log] dropped %zu messages\n", dropped);
    }

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
