#pragma once

#include <string>
#include <string_view>

namespace ext_client::utils::string {

  // Convert wide string to UTF-8 std::string using modern C++ types
  auto to_utf8(std::wstring_view src) -> std::string;

  // Convert UTF-8 string to wide string using modern C++ types
  auto to_wide(std::string_view src) -> std::wstring;

  // Extract basename from path using std::string_view
  auto basename(std::string_view path) -> std::string_view;

  // Trim whitespace from string using std::string_view
  auto trim(std::string_view src) -> std::string_view;

  // Normalize path char for comparison
  auto normalize_path_char(char c) -> char;

  // Compare two paths case-insensitively using std::string_view
  auto paths_match(std::string_view left, std::string_view right) -> bool;

  // Search substring case-insensitively
  auto contains_case_insensitive(std::string_view src, std::string_view target) -> bool;
  auto contains_case_insensitive(std::wstring_view src, std::wstring_view target) -> bool;

} // namespace ext_client::utils::string
