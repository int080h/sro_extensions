#include "Tile2DIndex.h"
#include "io/PathHelpers.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <fstream>
#include <cctype>

namespace sro {

bool Tile2DIndex::Load(const std::wstring& clientPath) {
    m_mapping.clear();
    std::wstring ifoPath = Tile2DIfoPath(clientPath);
    std::ifstream fs(ifoPath);
    if (!fs.is_open()) {
        Logger::Instance().Warning("tile2d.ifo not found: " + ToNarrow(ifoPath));
        return false;
    }

    std::string line;
    while (std::getline(fs, line)) {
        if (line.empty() || line.rfind("JMXV", 0) == 0 || !std::isdigit(static_cast<unsigned char>(line[0]))) continue;

        size_t q1 = line.find('\"');
        if (q1 == std::string::npos) continue;
        size_t q2 = line.find('\"', q1 + 1);
        if (q2 == std::string::npos) continue;
        size_t q3 = line.find('\"', q2 + 1);
        if (q3 == std::string::npos) continue;
        size_t q4 = line.find('\"', q3 + 1);
        if (q4 == std::string::npos) continue;

        std::string idStr = line.substr(0, q1);
        idStr = idStr.substr(0, idStr.find(' '));
        uint16_t id = static_cast<uint16_t>(std::stoi(idStr));
        std::string ddjName = line.substr(q3 + 1, q4 - q3 - 1);
        m_mapping[id] = std::wstring(ddjName.begin(), ddjName.end());
    }

    Logger::Instance().Info("Loaded tile2d.ifo: " + std::to_string(m_mapping.size()) + " tiles");
    return !m_mapping.empty();
}

std::wstring Tile2DIndex::ResolveDdjPath(uint16_t tileId) const {
    auto it = m_mapping.find(tileId);
    return it != m_mapping.end() ? it->second : std::wstring();
}

} // namespace sro
