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

inline std::wstring NavMeshFolder(const std::wstring& client) {
    return client + L"/Data/navmesh";
}

inline std::wstring MapInfoPath(const std::wstring& client) {
    return client + L"/Map/mapinfo.mfo";
}

inline std::wstring NavMeshMapInfoPath(const std::wstring& client) {
    auto navPath = NavMeshFolder(client) + L"/mapinfo.mfo";
    if (FileExists(navPath)) return navPath;
    return MapInfoPath(client);
}

inline std::wstring AiNavDataPath(const std::wstring& client, uint16_t regionId) {
    wchar_t buf[64];
    swprintf_s(buf, L"ainavdata_%u.dat", static_cast<unsigned>(regionId));
    return NavMeshFolder(client) + L"/" + buf;
}

inline std::wstring AiNavDataPathCaseInsensitive(const std::wstring& client, uint16_t regionId) {
    wchar_t buf[64];
    swprintf_s(buf, L"ainavdata_%u.dat", static_cast<unsigned>(regionId));
    std::wstring lower = NavMeshFolder(client) + L"/" + buf;
    std::wstring upper = NavMeshFolder(client) + L"/AINavData_" + std::to_wstring(regionId) + L".DAT";
    if (FileExists(upper)) return upper;
    return lower;
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
