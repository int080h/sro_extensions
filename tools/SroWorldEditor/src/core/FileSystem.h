#pragma once
#include <string>
#include <filesystem>

inline bool FileExists(const std::wstring& path) {
    return std::filesystem::exists(path);
}

inline bool FileExistsA(const std::string& path) {
    return std::filesystem::exists(path);
}

inline std::wstring ToWide(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

inline std::string ToNarrow(const std::wstring& s) {
    return std::string(s.begin(), s.end());
}
