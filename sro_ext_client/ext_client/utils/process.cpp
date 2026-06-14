#include "utils/process.hpp"

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

} // namespace ext_client::utils::process
