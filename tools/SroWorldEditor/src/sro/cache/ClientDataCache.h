#pragma once
#include "formats/MapFormats.h"
#include "formats/NavMeshFormat.h"
#include <string>
#include <unordered_map>
#include <cstdint>

namespace sro {

class ClientDataCache {
public:
    static constexpr uint32_t kFormatVersion = 1;

    static ClientDataCache& Instance();

    void SetClientPath(const std::wstring& clientPath);
    void ClearAll();
    void ClearCurrentClient();

    bool LoadMapMesh(const std::wstring& sourcePath, formats::MapMesh& out);
    bool LoadMapPlacements(const std::wstring& sourcePath, formats::MapPlacements& out);
    bool LoadNavMesh(const std::wstring& sourcePath, formats::NavMesh& out);
    bool LoadObjectIndex(const std::wstring& sourcePath, std::unordered_map<uint32_t, std::string>& out);

private:
    ClientDataCache() = default;

    std::wstring m_clientPath;
    std::wstring m_cacheRoot;

    std::wstring CacheRootForClient(const std::wstring& clientPath) const;
    std::wstring EntryPath(const std::wstring& sourcePath, const wchar_t* suffix) const;
    std::wstring MetaPath(const std::wstring& cachePath) const;
    bool IsMetaValid(const std::wstring& sourcePath, const std::wstring& metaPath) const;
    void WriteMeta(const std::wstring& sourcePath, const std::wstring& metaPath) const;
};

} // namespace sro
