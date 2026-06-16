#pragma once
#include <string>
#include <algorithm>
#include <cctype>

namespace sro {

inline std::string NormalizeBsrPathLower(std::string path) {
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) {
        if (c == '\\') return '/';
        return static_cast<char>(std::tolower(c));
    });
    return path;
}

// Event / seasonal props (winter trees, candy canes, festival banners, etc.)
inline bool IsEventOrSeasonalDecor(const std::string& bsrPath) {
    if (bsrPath.empty()) return false;
    const std::string norm = NormalizeBsrPathLower(bsrPath);

    static const char* kPatterns[] = {
        "/etc/",
        "_event",
        "/event/",
        "winter",
        "xmas",
        "christmas",
        "snowman",
        "snow_",
        "_snow",
        "festival",
        "holiday",
        "candy",
        "/deco/",
        "decoration",
        "newyear",
        "new_year",
    };

    for (const char* pat : kPatterns) {
        if (norm.find(pat) != std::string::npos) return true;
    }
    return false;
}

} // namespace sro
