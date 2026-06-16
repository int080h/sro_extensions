#pragma once
#include <map>
#include <optional>
#include <string>
#include <utility>

namespace sro::map {

// Canonical on-disk form: "{ry}x{rx}.ddj" (no spaces around x).
inline std::wstring FilenameForRegion(int rx, int ry) {
    return std::to_wstring(ry) + L"x" + std::to_wstring(rx) + L".ddj";
}

inline bool PathHasSpacedCoords(const std::wstring& path) {
    return path.find(L" x ") != std::wstring::npos;
}

} // namespace sro::map

// Path lookup for minimap DDJ tiles indexed as (rx, ry).
class MinimapTileIndex {
public:
    void Insert(int rx, int ry, std::wstring path) {
        m_paths[{rx, ry}] = std::move(path);
    }

    std::optional<std::wstring> PathForRegion(int rx, int ry) const {
        auto it = m_paths.find({rx, ry});
        if (it == m_paths.end()) return std::nullopt;
        return it->second;
    }

    size_t Size() const { return m_paths.size(); }
    void Clear() { m_paths.clear(); }

private:
    std::map<std::pair<int, int>, std::wstring> m_paths;
};
