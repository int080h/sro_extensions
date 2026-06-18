#pragma once
#include "nav/NavMeshCatalog.h"
#include "formats/NavMeshFormat.h"
#include "formats/AiNavDataFormat.h"
#include "formats/MapProject.h"
#include <map>
#include <memory>
#include <set>
#include <string>

namespace sro::nav {

class NavMeshSession {
public:
    void SetClientPath(const std::wstring& clientPath);
    void Scan();
    void Clear();

    const NavMeshCatalog& Catalog() const { return m_catalog; }
    const std::wstring& ClientPath() const { return m_clientPath; }

    formats::NavMesh* GetTerrainNavMesh(int rx, int ry);
    formats::AiNavData* GetAiNavData(uint16_t regionId);
    const formats::MapProject* MapInfo() const { return m_mapInfo ? &*m_mapInfo : nullptr; }

    bool LoadTerrainNavMesh(int rx, int ry);
    bool LoadAiNavData(uint16_t regionId);
    bool LoadMapInfo();

    bool SaveTerrainNavMesh(int rx, int ry);
    bool SaveAiNavData(uint16_t regionId);
    bool SaveMapInfo();

    bool CreateTerrainNavMesh(int rx, int ry);
    bool DeleteTerrainNavMesh(int rx, int ry);
    bool CreateAiNavData(uint16_t regionId);
    bool DeleteAiNavData(uint16_t regionId);

    void MarkTerrainDirty(int rx, int ry);
    void MarkAiNavDirty(uint16_t regionId);
    void MarkMapInfoDirty();
    bool HasDirtyFiles() const;

    bool SaveAllDirty();
    bool IsTerrainDirty(int rx, int ry) const;
    bool IsAiNavDirty(uint16_t regionId) const;
    bool IsMapInfoDirty() const { return m_mapInfoDirty; }

private:
    std::wstring m_clientPath;
    NavMeshCatalog m_catalog;
    std::unique_ptr<formats::MapProject> m_mapInfo;
    std::map<std::pair<int, int>, std::unique_ptr<formats::NavMesh>> m_terrain;
    std::map<uint16_t, std::unique_ptr<formats::AiNavData>> m_aiNav;
    std::set<std::pair<int, int>> m_dirtyTerrain;
    std::set<uint16_t> m_dirtyAiNav;
    bool m_mapInfoDirty = false;
};

} // namespace sro::nav
