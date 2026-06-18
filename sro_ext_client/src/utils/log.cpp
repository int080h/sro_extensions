#include "utils/log.hpp"

#include <windows.h>
#include <cstdarg>
#include <cstdio>

namespace ext_client::utils {

  auto log_init() -> void {
    AllocConsole();
    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
    SetConsoleTitleA("sro_ext_client");

    // Clear old log file on start
    if (FILE* f = nullptr; fopen_s(&f, "ext_client.log", "w") == 0 && f) {
      std::fclose(f);
    }

    log_msg("console ready");
  }

  auto log_shutdown() -> void {
    FreeConsole();
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
    FILE* f = nullptr;
    if (fopen_s(&f, "ext_client.log", "a") == 0 && f) {
      std::vfprintf(f, fmt, args);
      std::fprintf(f, "\n");
      std::fclose(f);
    }
    va_end(args);
  }

} // namespace ext_client::utils
