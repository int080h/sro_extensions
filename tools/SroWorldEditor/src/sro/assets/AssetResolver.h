#pragma once
#include "index/ObjectIndex.h"
#include "index/Tile2DIndex.h"
#include <string>
#include <cstdint>

namespace sro {

class AssetResolver {
public:
    void Initialize(const std::wstring& clientPath) {
        m_clientPath = clientPath;
        m_objectIndex.Load(clientPath);
        m_tileIndex.Load(clientPath);
    }

    std::string ResolveObjectBsr(uint32_t objId) const {
        return m_objectIndex.ResolveBsrPath(objId);
    }

    std::wstring ResolveTileDdj(uint16_t tileId) const {
        return m_tileIndex.ResolveDdjPath(tileId);
    }

    const ObjectIndex& Objects() const { return m_objectIndex; }
    const Tile2DIndex& Tiles() const { return m_tileIndex; }

private:
    std::wstring m_clientPath;
    ObjectIndex m_objectIndex;
    Tile2DIndex m_tileIndex;
};

} // namespace sro
