#include "ObjectIndex.h"
#include "cache/ClientDataCache.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <fstream>
#include <cctype>

namespace sro {

bool ObjectIndex::ParseFile(const std::wstring& path) {
    std::ifstream fs(path);
    if (!fs.is_open()) return false;

    std::string line;
    while (std::getline(fs, line)) {
        if (line.empty() || !std::isdigit(static_cast<unsigned char>(line[0]))) continue;

        size_t space1 = line.find_first_of(" \t");
        if (space1 == std::string::npos) continue;
        size_t space2 = line.find_first_of(" \t", space1 + 1);
        if (space2 == std::string::npos) continue;

        try {
            uint32_t objId = static_cast<uint32_t>(std::stoul(line.substr(0, space1)));
            size_t q1 = line.find('\"', space2 + 1);
            if (q1 == std::string::npos) continue;
            size_t q2 = line.find('\"', q1 + 1);
            if (q2 == std::string::npos) continue;
            std::string bsrPath = line.substr(q1 + 1, q2 - q1 - 1);
            while (!bsrPath.empty() && (bsrPath.back() == '\r' || bsrPath.back() == '\n' || bsrPath.back() == '\t' || bsrPath.back() == ' ')) {
                bsrPath.pop_back();
            }
            if (!bsrPath.empty()) {
                m_mapping[objId] = bsrPath;
            }
        } catch (...) {}
    }
    return true;
}

bool ObjectIndex::Load(const std::wstring& clientPath) {
    m_mapping.clear();
    const std::wstring paths[] = {
        clientPath + L"/Data/object.ifo",
        clientPath + L"/Map/object.ifo",
        clientPath + L"/Data/navmesh/object.ifo",
    };
    for (const auto& p : paths) {
        if (!FileExists(p)) continue;
        if (ClientDataCache::Instance().LoadObjectIndex(p, m_mapping) || ParseFile(p)) {
            Logger::Instance().Info("Loaded object.ifo: " + ToNarrow(p) + " (" + std::to_string(m_mapping.size()) + " entries)");
            return true;
        }
    }
    Logger::Instance().Warning("object.ifo not found in client data.");
    return false;
}

std::string ObjectIndex::ResolveBsrPath(uint32_t objId) const {
    auto it = m_mapping.find(objId);
    return it != m_mapping.end() ? it->second : std::string();
}

} // namespace sro
