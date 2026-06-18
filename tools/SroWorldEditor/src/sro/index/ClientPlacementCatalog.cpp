#include "ClientPlacementCatalog.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>

namespace sro {
namespace {

std::string ReadAsciiFile(const std::wstring& path) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return {};
    return std::string((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
}

} // namespace

std::wstring ClientPlacementCatalog::TextDataRoot(const std::wstring& clientPath) {
    const std::wstring paths[] = {
        clientPath + L"/Media/server_dep/silkroad/textdata",
        clientPath + L"/Media/textdata",
    };
    for (const auto& p : paths) {
        if (FileExists(p)) return p;
    }
    return paths[0];
}

std::vector<std::string> ClientPlacementCatalog::SplitColumns(const std::string& line) {
    std::vector<std::string> cols;
    std::string cur;
    for (char c : line) {
        if (c == '\t') {
            cols.push_back(cur);
            cur.clear();
        } else if (c != '\r') {
            cur.push_back(c);
        }
    }
    cols.push_back(cur);
    return cols;
}

bool ClientPlacementCatalog::ParseFloat(const std::string& s, float& out) {
    if (s.empty()) return false;
    try {
        out = std::stof(s);
        return true;
    } catch (...) {
        return false;
    }
}

bool ClientPlacementCatalog::ParseInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    try {
        out = std::stoi(s);
        return true;
    } catch (...) {
        return false;
    }
}

bool ClientPlacementCatalog::LoadNpcPos(const std::wstring& path) {
    const std::string text = ReadAsciiFile(path);
    if (text.empty()) return false;

    std::istringstream stream(text);
    std::string line;
    int parsed = 0;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '/' || line[0] == ';') continue;
        auto cols = SplitColumns(line);
        if (cols.size() < 5) continue;

        NpcPlacement pl;
        if (!ParseInt(cols[0], pl.serviceId)) continue;
        if (!ParseInt(cols[1], pl.regionId)) continue;
        if (!ParseFloat(cols[2], pl.localPos.x)) continue;
        if (!ParseFloat(cols[3], pl.localPos.y)) continue;
        if (!ParseFloat(cols[4], pl.localPos.z)) continue;

        m_npcsByRegion[pl.regionId].push_back(m_npcs.size());
        m_npcs.push_back(pl);
        ++parsed;
    }
    Logger::Instance().Info("ClientPlacementCatalog: npcpos " + std::to_string(parsed) + " rows");
    return parsed > 0;
}

bool ClientPlacementCatalog::LoadTeleportData(const std::wstring& path) {
    const std::string text = ReadAsciiFile(path);
    if (text.empty()) return false;

    std::istringstream stream(text);
    std::string line;
    int parsed = 0;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '/' || line[0] == ';') continue;
        auto cols = SplitColumns(line);
        if (cols.size() < 10) continue;

        TeleportPlacement tp;
        if (!ParseInt(cols[3], tp.serviceId)) continue;
        tp.codeName = cols.size() > 2 ? cols[2] : "";
        tp.zoneStrId = cols.size() > 4 ? cols[4] : "";
        if (!ParseInt(cols[5], tp.regionId)) continue;
        if (!ParseFloat(cols[6], tp.localPos.x)) continue;
        if (!ParseFloat(cols[7], tp.localPos.y)) continue;
        if (!ParseFloat(cols[8], tp.localPos.z)) continue;
        ParseFloat(cols[9], tp.radius);

        m_teleportsByRegion[tp.regionId].push_back(m_teleports.size());
        m_teleports.push_back(tp);
        ++parsed;
    }
    Logger::Instance().Info("ClientPlacementCatalog: teleportdata " + std::to_string(parsed) + " rows");
    return parsed > 0;
}

bool ClientPlacementCatalog::Load(const std::wstring& clientPath) {
    m_loaded = false;
    m_npcs.clear();
    m_teleports.clear();
    m_npcsByRegion.clear();
    m_teleportsByRegion.clear();

    const std::wstring root = TextDataRoot(clientPath);
    const bool npcOk = LoadNpcPos(root + L"/npcpos.txt");
    const bool tpOk = LoadTeleportData(root + L"/teleportdata.txt");
    m_loaded = npcOk || tpOk;
    if (m_loaded) {
        Logger::Instance().Info("ClientPlacementCatalog loaded from " + ToNarrow(root));
    } else {
        Logger::Instance().Warning("ClientPlacementCatalog: no npcpos/teleportdata found under " + ToNarrow(root));
    }
    return m_loaded;
}

void ClientPlacementCatalog::CollectNpcsForRegion(int regionId, std::vector<NpcPlacement>& out) const {
    auto it = m_npcsByRegion.find(regionId);
    if (it == m_npcsByRegion.end()) return;
    for (size_t idx : it->second) {
        if (idx < m_npcs.size()) out.push_back(m_npcs[idx]);
    }
}

void ClientPlacementCatalog::CollectTeleportsForRegion(int regionId, std::vector<TeleportPlacement>& out) const {
    auto it = m_teleportsByRegion.find(regionId);
    if (it == m_teleportsByRegion.end()) return;
    for (size_t idx : it->second) {
        if (idx < m_teleports.size()) out.push_back(m_teleports[idx]);
    }
}

} // namespace sro
