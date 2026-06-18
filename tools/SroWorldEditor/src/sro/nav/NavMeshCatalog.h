#pragma once
#include "core/RegionCoord.h"
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace sro::nav {

enum class NavMeshFileKind {
    TerrainNvm,
    AiNavData,
    MapInfo,
    ObjectIfo,
    ObjectStringIfo,
    ObjExtIfo,
    Tile2DIfo,
    Other
};

struct NavMeshFileEntry {
    NavMeshFileKind kind = NavMeshFileKind::Other;
    std::wstring path;
    std::wstring fileName;
    int rx = -1;
    int ry = -1;
    uint16_t regionId = 0;
    uint64_t fileSize = 0;
};

class NavMeshCatalog {
public:
    void Scan(const std::wstring& clientPath);
    void Clear();

    const std::wstring& ClientPath() const { return m_clientPath; }
    const std::vector<NavMeshFileEntry>& AllFiles() const { return m_files; }
    const std::vector<NavMeshFileEntry>& TerrainFiles() const { return m_terrain; }
    const std::vector<NavMeshFileEntry>& AiNavFiles() const { return m_aiNav; }
    const std::set<std::pair<int, int>>& TerrainRegionCoords() const { return m_terrainCoords; }
    const std::set<uint16_t>& AiNavRegionIds() const { return m_aiNavIds; }

    bool HasTerrain(int rx, int ry) const;
    bool HasAiNav(uint16_t regionId) const;
    std::wstring TerrainPath(int rx, int ry) const;
    std::wstring AiNavPath(uint16_t regionId) const;

private:
    static NavMeshFileKind ClassifyFileName(const std::wstring& name, int& rx, int& ry, uint16_t& regionId);

    std::wstring m_clientPath;
    std::vector<NavMeshFileEntry> m_files;
    std::vector<NavMeshFileEntry> m_terrain;
    std::vector<NavMeshFileEntry> m_aiNav;
    std::set<std::pair<int, int>> m_terrainCoords;
    std::set<uint16_t> m_aiNavIds;
    std::map<std::pair<int, int>, std::wstring> m_terrainPaths;
    std::map<uint16_t, std::wstring> m_aiNavPaths;
};

} // namespace sro::nav
