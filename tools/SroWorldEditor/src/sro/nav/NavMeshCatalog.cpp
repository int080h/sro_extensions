#include "nav/NavMeshCatalog.h"
#include "io/PathHelpers.h"
#include "core/FileSystem.h"
#include <algorithm>
#include <cwchar>
#include <filesystem>

namespace sro::nav {

namespace {

std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
    return s;
}

} // namespace

NavMeshFileKind NavMeshCatalog::ClassifyFileName(const std::wstring& name, int& rx, int& ry, uint16_t& regionId) {
    rx = ry = -1;
    regionId = 0;
    const std::wstring lower = ToLower(name);

    if (lower == L"mapinfo.mfo") return NavMeshFileKind::MapInfo;
    if (lower == L"object.ifo") return NavMeshFileKind::ObjectIfo;
    if (lower == L"objectstring.ifo") return NavMeshFileKind::ObjectStringIfo;
    if (lower == L"objext.ifo") return NavMeshFileKind::ObjExtIfo;
    if (lower == L"tile2d.ifo") return NavMeshFileKind::Tile2DIfo;

    if (lower.size() >= 8 && lower.rfind(L"nv_") == 0 && lower.substr(lower.size() - 4) == L".nvm") {
        unsigned tr = 0, ty = 0;
        if (swscanf_s(lower.c_str() + 3, L"%2x%2x.nvm", &tr, &ty) == 2) {
            rx = static_cast<int>(tr);
            ry = static_cast<int>(ty);
            return NavMeshFileKind::TerrainNvm;
        }
    }

    if (lower.size() > 12 && lower.rfind(L"ainavdata_") == 0 && lower.substr(lower.size() - 4) == L".dat") {
        unsigned id = 0;
        if (swscanf_s(lower.c_str() + 10, L"%u.dat", &id) == 1) {
            regionId = static_cast<uint16_t>(id);
            return NavMeshFileKind::AiNavData;
        }
    }

    return NavMeshFileKind::Other;
}

void NavMeshCatalog::Clear() {
    m_clientPath.clear();
    m_files.clear();
    m_terrain.clear();
    m_aiNav.clear();
    m_terrainCoords.clear();
    m_aiNavIds.clear();
    m_terrainPaths.clear();
    m_aiNavPaths.clear();
}

void NavMeshCatalog::Scan(const std::wstring& clientPath) {
    Clear();
    m_clientPath = clientPath;
    const std::wstring folder = NavMeshFolder(clientPath);
    if (!FileExists(folder)) return;

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (!entry.is_regular_file(ec)) continue;

        NavMeshFileEntry fe;
        fe.path = entry.path().wstring();
        fe.fileName = entry.path().filename().wstring();
        fe.kind = ClassifyFileName(fe.fileName, fe.rx, fe.ry, fe.regionId);
        fe.fileSize = static_cast<uint64_t>(entry.file_size(ec));
        m_files.push_back(fe);

        switch (fe.kind) {
        case NavMeshFileKind::TerrainNvm:
            m_terrain.push_back(fe);
            m_terrainCoords.insert({fe.rx, fe.ry});
            m_terrainPaths[{fe.rx, fe.ry}] = fe.path;
            break;
        case NavMeshFileKind::AiNavData:
            m_aiNav.push_back(fe);
            m_aiNavIds.insert(fe.regionId);
            m_aiNavPaths[fe.regionId] = fe.path;
            break;
        default:
            break;
        }
    }

    auto byName = [](const NavMeshFileEntry& a, const NavMeshFileEntry& b) { return a.fileName < b.fileName; };
    std::sort(m_files.begin(), m_files.end(), byName);
    std::sort(m_terrain.begin(), m_terrain.end(), [](const NavMeshFileEntry& a, const NavMeshFileEntry& b) {
        if (a.ry != b.ry) return a.ry < b.ry;
        return a.rx < b.rx;
    });
    std::sort(m_aiNav.begin(), m_aiNav.end(), [](const NavMeshFileEntry& a, const NavMeshFileEntry& b) {
        return a.regionId < b.regionId;
    });
}

bool NavMeshCatalog::HasTerrain(int rx, int ry) const {
    return m_terrainCoords.count({rx, ry}) != 0;
}

bool NavMeshCatalog::HasAiNav(uint16_t regionId) const {
    return m_aiNavIds.count(regionId) != 0;
}

std::wstring NavMeshCatalog::TerrainPath(int rx, int ry) const {
    auto it = m_terrainPaths.find({rx, ry});
    if (it != m_terrainPaths.end()) return it->second;
    return NavMeshPath(m_clientPath, RegionCoord(rx, ry));
}

std::wstring NavMeshCatalog::AiNavPath(uint16_t regionId) const {
    auto it = m_aiNavPaths.find(regionId);
    if (it != m_aiNavPaths.end()) return it->second;
    return AiNavDataPathCaseInsensitive(m_clientPath, regionId);
}

} // namespace sro::nav
