#pragma once
#include "core/FileSystem.h"
#include "core/RegionCoord.h" // sro::RegionCoord
#include <string>

namespace sro {

inline std::wstring MapMeshPath(const std::wstring& client, const RegionCoord& c) {
    return client + L"/Map/" + std::to_wstring(c.rx) + L"/" + std::to_wstring(c.ry) + L".m";
}

inline std::wstring MapPlacementO2Path(const std::wstring& client, const RegionCoord& c) {
    return client + L"/Map/" + std::to_wstring(c.rx) + L"/" + std::to_wstring(c.ry) + L".o2";
}

inline std::wstring MapPlacementOPath(const std::wstring& client, const RegionCoord& c) {
    return client + L"/Map/" + std::to_wstring(c.rx) + L"/" + std::to_wstring(c.ry) + L".o";
}

inline std::wstring NavMeshPath(const std::wstring& client, const RegionCoord& c) {
    wchar_t buf[64];
    swprintf_s(buf, L"nv_%02x%02x.nvm", c.rx, c.ry);
    return client + L"/Data/navmesh/" + buf;
}

inline std::wstring MapInfoPath(const std::wstring& client) {
    return client + L"/Map/mapinfo.mfo";
}

inline std::wstring Tile2DIfoPath(const std::wstring& client) {
    return client + L"/Map/tile2d.ifo";
}

inline std::wstring ResolvePlacementPath(const std::wstring& client, const RegionCoord& c) {
    auto o2 = MapPlacementO2Path(client, c);
    if (FileExists(o2)) return o2;
    return MapPlacementOPath(client, c);
}

inline bool IsO2Path(const std::wstring& path) {
    return path.size() >= 3 && path.substr(path.size() - 3) == L".o2";
}

} // namespace sro
