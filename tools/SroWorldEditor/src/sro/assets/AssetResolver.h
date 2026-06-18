#pragma once
#include "index/ObjectIndex.h"
#include "index/TextDataCatalog.h"
#include "index/ClientPlacementCatalog.h"
#include "index/StringTableCatalog.h"
#include "index/Tile2DIndex.h"
#include "assets/AssetPathResolver.h"
#include "core/ClientLoadProgress.h"
#include <string>
#include <cstdint>

namespace sro {

class AssetResolver {
public:
    void ResetForLoad(const std::wstring& clientPath);
    bool StepLoad(ClientLoadProgress& progress);
    void Initialize(const std::wstring& clientPath);

    int TotalLoadSteps() const { return m_totalSteps; }
    int CompletedLoadSteps() const { return m_completedSteps; }

    std::string ResolveObjectBsr(uint32_t objId) const {
        return m_objectIndex.ResolveBsrPath(objId);
    }

    std::wstring ResolveTileDdj(uint16_t tileId) const {
        return m_tileIndex.ResolveDdjPath(tileId);
    }

    std::wstring ResolveAssetPath(const std::string& relPath,
                                  const std::string& baseRelPath = {}) const {
        if (!baseRelPath.empty())
            return m_pathResolver.ResolveRelativeToBase(baseRelPath, relPath);
        return m_pathResolver.ResolveRelativePath(relPath);
    }

    const ObjectIndex& Objects() const { return m_objectIndex; }
    const Tile2DIndex& Tiles() const { return m_tileIndex; }
    TextDataCatalog& TextData() { return m_textData; }
    const TextDataCatalog& TextData() const { return m_textData; }
    const ClientPlacementCatalog& Placements() const { return m_placements; }
    const StringTableCatalog& Strings() const { return m_stringTable; }
    const AssetPathResolver& Paths() const { return m_pathResolver; }

private:
    enum class LoadPhase {
        ObjectIndex,
        TileIndex,
        StringTable,
        TextData,
        Placements,
        Done
    };

    void RecountTotalSteps();

    std::wstring m_clientPath;
    LoadPhase m_phase = LoadPhase::Done;
    int m_totalSteps = 1;
    int m_completedSteps = 0;
    ObjectIndex m_objectIndex;
    Tile2DIndex m_tileIndex;
    TextDataCatalog m_textData;
    ClientPlacementCatalog m_placements;
    StringTableCatalog m_stringTable;
    AssetPathResolver m_pathResolver;
};

} // namespace sro
