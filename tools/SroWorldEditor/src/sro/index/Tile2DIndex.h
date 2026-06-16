#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

namespace sro {

class Tile2DIndex {
public:
    bool Load(const std::wstring& clientPath);
    std::wstring ResolveDdjPath(uint16_t tileId) const;
    bool HasTile(uint16_t tileId) const { return m_mapping.count(tileId) > 0; }
    size_t Count() const { return m_mapping.size(); }

private:
    std::unordered_map<uint16_t, std::wstring> m_mapping;
};

} // namespace sro
