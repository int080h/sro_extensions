#pragma once
#include "core/MathTypes.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace sro {

struct NpcPlacement {
    int serviceId = 0;
    int regionId = 0;
    Vector3 localPos{};
};

struct TeleportPlacement {
    int serviceId = 0;
    int regionId = 0;
    Vector3 localPos{};
    float radius = 0.f;
    std::string codeName;
    std::string zoneStrId;
};

class ClientPlacementCatalog {
public:
    bool Load(const std::wstring& clientPath);
    bool Loaded() const { return m_loaded; }

    void CollectNpcsForRegion(int regionId, std::vector<NpcPlacement>& out) const;
    void CollectTeleportsForRegion(int regionId, std::vector<TeleportPlacement>& out) const;

    size_t NpcCount() const { return m_npcs.size(); }
    size_t TeleportCount() const { return m_teleports.size(); }

private:
    static std::wstring TextDataRoot(const std::wstring& clientPath);
    static std::vector<std::string> SplitColumns(const std::string& line);
    static bool ParseFloat(const std::string& s, float& out);
    static bool ParseInt(const std::string& s, int& out);

    bool LoadNpcPos(const std::wstring& path);
    bool LoadTeleportData(const std::wstring& path);

    bool m_loaded = false;
    std::vector<NpcPlacement> m_npcs;
    std::vector<TeleportPlacement> m_teleports;
    std::unordered_map<int, std::vector<size_t>> m_npcsByRegion;
    std::unordered_map<int, std::vector<size_t>> m_teleportsByRegion;
};

} // namespace sro
