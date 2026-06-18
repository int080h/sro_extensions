#include "nav/NavMeshSession.h"
#include "parsers/NavMeshParser.h"
#include "parsers/AiNavDataParser.h"
#include "parsers/MapProjectParser.h"
#include "io/PathHelpers.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <filesystem>

namespace sro::nav {

void NavMeshSession::SetClientPath(const std::wstring& clientPath) {
    if (m_clientPath != clientPath) {
        Clear();
        m_clientPath = clientPath;
    }
}

void NavMeshSession::Scan() {
    m_catalog.Scan(m_clientPath);
}

void NavMeshSession::Clear() {
    m_catalog.Clear();
    m_mapInfo.reset();
    m_terrain.clear();
    m_aiNav.clear();
    m_dirtyTerrain.clear();
    m_dirtyAiNav.clear();
    m_mapInfoDirty = false;
}

formats::NavMesh* NavMeshSession::GetTerrainNavMesh(int rx, int ry) {
    auto it = m_terrain.find({rx, ry});
    return it != m_terrain.end() ? it->second.get() : nullptr;
}

formats::AiNavData* NavMeshSession::GetAiNavData(uint16_t regionId) {
    auto it = m_aiNav.find(regionId);
    return it != m_aiNav.end() ? it->second.get() : nullptr;
}

bool NavMeshSession::LoadTerrainNavMesh(int rx, int ry) {
    if (m_clientPath.empty()) return false;
    auto path = m_catalog.HasTerrain(rx, ry) ? m_catalog.TerrainPath(rx, ry) : NavMeshPath(m_clientPath, RegionCoord(rx, ry));
    if (!FileExists(path)) return false;
    auto nav = std::make_unique<formats::NavMesh>();
    if (!NavMeshParser::Read(path, *nav).ok) return false;
    m_terrain[{rx, ry}] = std::move(nav);
    return true;
}

bool NavMeshSession::LoadAiNavData(uint16_t regionId) {
    if (m_clientPath.empty()) return false;
    auto path = m_catalog.HasAiNav(regionId) ? m_catalog.AiNavPath(regionId) : AiNavDataPathCaseInsensitive(m_clientPath, regionId);
    if (!FileExists(path)) return false;
    auto data = std::make_unique<formats::AiNavData>();
    if (!AiNavDataParser::Read(path, *data).ok) return false;
    m_aiNav[regionId] = std::move(data);
    return true;
}

bool NavMeshSession::LoadMapInfo() {
    if (m_clientPath.empty()) return false;
    auto path = NavMeshMapInfoPath(m_clientPath);
    if (!FileExists(path)) return false;
    m_mapInfo = std::make_unique<formats::MapProject>();
    return MapProjectParser::Read(path, *m_mapInfo).ok;
}

bool NavMeshSession::SaveTerrainNavMesh(int rx, int ry) {
    auto* nav = GetTerrainNavMesh(rx, ry);
    if (!nav) return false;
    auto path = NavMeshPath(m_clientPath, RegionCoord(rx, ry));
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    if (!NavMeshParser::Write(path, *nav).ok) return false;
    m_dirtyTerrain.erase({rx, ry});
    return true;
}

bool NavMeshSession::SaveAiNavData(uint16_t regionId) {
    auto* data = GetAiNavData(regionId);
    if (!data) return false;
    auto path = AiNavDataPath(m_clientPath, regionId);
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    if (!AiNavDataParser::Write(path, *data).ok) return false;
    m_dirtyAiNav.erase(regionId);
    return true;
}

bool NavMeshSession::SaveMapInfo() {
    if (!m_mapInfo) return false;
    auto path = NavMeshMapInfoPath(m_clientPath);
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    if (!MapProjectParser::Write(path, *m_mapInfo).ok) return false;
    m_mapInfoDirty = false;
    return true;
}

bool NavMeshSession::CreateTerrainNavMesh(int rx, int ry) {
    auto nav = std::make_unique<formats::NavMesh>();
    nav->Signature = "JMXVNVM 1000";
    nav->TileMap.resize(96 * 96);
    for (auto& tile : nav->TileMap) {
        tile.Flags = 0xFFFF;
        tile.CellID = 0;
        tile.TextureID = 0;
    }
    nav->HeightMap.resize(97 * 97, 0.f);
    nav->PlaneTypeMap.resize(6 * 6, 0);
    nav->PlaneHeightMap.resize(6 * 6, 0.f);
    m_terrain[{rx, ry}] = std::move(nav);
    MarkTerrainDirty(rx, ry);
    return true;
}

bool NavMeshSession::DeleteTerrainNavMesh(int rx, int ry) {
    m_terrain.erase({rx, ry});
    m_dirtyTerrain.erase({rx, ry});
    auto path = NavMeshPath(m_clientPath, RegionCoord(rx, ry));
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return true;
}

bool NavMeshSession::CreateAiNavData(uint16_t regionId) {
    auto data = std::make_unique<formats::AiNavData>();
    data->Version = 1;
    data->RefDungeon.RegionID = regionId;
    data->RefDungeon.BlockCount = 0;
    data->RefDungeon.Int0 = 0;
    data->SimpleDungeon.RegionID = regionId;
    data->SimpleDungeon.BlockCount = 0;
    data->SimpleDungeon.Int1 = 0;
    m_aiNav[regionId] = std::move(data);
    MarkAiNavDirty(regionId);
    return true;
}

bool NavMeshSession::DeleteAiNavData(uint16_t regionId) {
    m_aiNav.erase(regionId);
    m_dirtyAiNav.erase(regionId);
    auto path = AiNavDataPath(m_clientPath, regionId);
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return true;
}

void NavMeshSession::MarkTerrainDirty(int rx, int ry) { m_dirtyTerrain.insert({rx, ry}); }
void NavMeshSession::MarkAiNavDirty(uint16_t regionId) { m_dirtyAiNav.insert(regionId); }
void NavMeshSession::MarkMapInfoDirty() { m_mapInfoDirty = true; }

bool NavMeshSession::HasDirtyFiles() const {
    return !m_dirtyTerrain.empty() || !m_dirtyAiNav.empty() || m_mapInfoDirty;
}

bool NavMeshSession::SaveAllDirty() {
    bool ok = true;
    for (const auto& [rx, ry] : m_dirtyTerrain) ok = SaveTerrainNavMesh(rx, ry) && ok;
    for (uint16_t id : m_dirtyAiNav) ok = SaveAiNavData(id) && ok;
    if (m_mapInfoDirty) ok = SaveMapInfo() && ok;
    return ok;
}

bool NavMeshSession::IsTerrainDirty(int rx, int ry) const { return m_dirtyTerrain.count({rx, ry}) != 0; }
bool NavMeshSession::IsAiNavDirty(uint16_t regionId) const { return m_dirtyAiNav.count(regionId) != 0; }

} // namespace sro::nav
