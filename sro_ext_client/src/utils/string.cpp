#include "utils/string.hpp"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cwctype>

namespace ext_client::utils::string {

  auto to_utf8(std::wstring_view src) -> std::string {
    if (src.empty()) {
      return {};
    }
    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, src.data(), static_cast<int>(src.size()), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) {
      return {};
    }
    std::string dst(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, src.data(), static_cast<int>(src.size()), dst.data(), size_needed, nullptr, nullptr);
    return dst;
  }

  auto to_wide(std::string_view src) -> std::wstring {
    if (src.empty()) {
      return {};
    }
    const int size_needed = MultiByteToWideChar(CP_UTF8, 0, src.data(), static_cast<int>(src.size()), nullptr, 0);
    if (size_needed <= 0) {
      return {};
    }
    std::wstring dst(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, src.data(), static_cast<int>(src.size()), dst.data(), size_needed);
    return dst;
  }

  auto basename(std::string_view path) -> std::string_view {
    const auto last_slash = path.find_last_of("\\/");
    if (last_slash == std::string_view::npos) {
      return path;
    }
    return path.substr(last_slash + 1);
  }

  auto trim(std::string_view src) -> std::string_view {
    const std::string_view whitespace = " \t\r\n";
    const auto start = src.find_first_not_of(whitespace);
    if (start == std::string_view::npos) {
      return {};
    }
    const auto end = src.find_last_not_of(whitespace);
    return src.substr(start, end - start + 1);
  }

  auto normalize_path_char(char c) -> char {
    if (c >= 'A' && c <= 'Z') {
      return static_cast<char>(c - 'A' + 'a');
    }
    if (c == '/') {
      return '\\';
    }
    return c;
  }

  auto paths_match(std::string_view left, std::string_view right) -> bool {
    if (left.size() != right.size()) {
      return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
      if (normalize_path_char(left[i]) != normalize_path_char(right[i])) {
        return false;
      }
    }
    return true;
  }

  auto contains_case_insensitive(std::string_view src, std::string_view target) -> bool {
    if (target.empty()) {
      return false;
    }
    auto it = std::search(
      src.begin(), src.end(),
      target.begin(), target.end(),
      [](char ch1, char ch2) {
        return std::tolower(static_cast<unsigned char>(ch1)) == std::tolower(static_cast<unsigned char>(ch2));
      }
    );
    return it != src.end();
  }

  auto contains_case_insensitive(std::wstring_view src, std::wstring_view target) -> bool {
    if (target.empty()) {
      return false;
    }
    auto it = std::search(
      src.begin(), src.end(),
      target.begin(), target.end(),
      [](wchar_t ch1, wchar_t ch2) {
        return std::towlower(ch1) == std::towlower(ch2);
      }
    );
    return it != src.end();
  }

} // namespace ext_client::utils::string
