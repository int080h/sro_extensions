#pragma once

#include <string>

#include <vector>

#include <unordered_map>

#include <optional>

#include <cstdint>



namespace sro {

class StringTableCatalog;



enum class GameObjectKind {

    Character,

    Item,

    MapObjectText,

    Unknown

};



enum class CatalogFilter {

    All,

    Characters,

    Items,

    TextObjects,

    Npcs,

    Monsters,

    MapObjects

};



struct GameObjectRef {

    GameObjectKind kind = GameObjectKind::Unknown;

    int serviceId = 0;

    std::string codeName;

    std::string displayName;

    std::string stringKey;

    std::string parentCodeName;

    std::vector<std::string> modelPaths;

    std::vector<std::string> rawColumns;

    std::string sourceFile;



    bool IsNpc() const {

        return codeName.rfind("NPC_", 0) == 0;

    }

    bool IsMonster() const {

        return codeName.rfind("MOB_", 0) == 0;

    }

    bool IsClone() const {

        return codeName.size() >= 6 && codeName.rfind("_CLON") == codeName.size() - 5;

    }

    std::string PrimaryModelPath() const {

        for (const auto& p : modelPaths) {

            if (p.empty() || p == "xxx") continue;

            const auto lower = p;

            if (p.size() >= 4) {

                const auto ext = p.substr(p.size() - 4);

                if (ext == ".bsr" || ext == ".cpd" || ext == ".CPD") return p;

            }

        }

        return {};

    }

};



class TextDataCatalog {

public:

    bool Load(const std::wstring& clientPath);

    void BeginIncrementalLoad(const std::wstring& clientPath);
    bool LoadNextSplit(std::wstring* outSplitPath = nullptr);
    size_t TotalSplitCount() const { return m_pendingSplits.size(); }
    size_t LoadedSplitCount() const { return m_splitLoadIndex; }
    bool Loaded() const { return !m_clientPath.empty() && m_splitLoadIndex >= m_pendingSplits.size() && !m_pendingSplits.empty(); }

    size_t Count() const { return m_byCodeName.size(); }



    const GameObjectRef* FindByCodeName(const std::string& codeName) const;

    const GameObjectRef* FindByServiceId(int serviceId) const;



    std::string ResolvePrimaryModelPath(const GameObjectRef& ref, int maxDepth = 4) const;



    void CollectAll(std::vector<const GameObjectRef*>& out) const;

    void CollectByKind(GameObjectKind kind, std::vector<const GameObjectRef*>& out) const;

    void Search(const char* query, std::vector<const GameObjectRef*>& out, int maxResults = 500) const;

    void CollectForFilter(CatalogFilter filter, const char* searchQuery,
                          std::vector<const GameObjectRef*>& out,
                          const StringTableCatalog* strings = nullptr) const;

    void FinalizeServiceIds();

private:

    bool LoadIndexFile(const std::wstring& indexPath, GameObjectKind kind);

    bool ParseSplitFile(const std::wstring& path, GameObjectKind kind);

    static std::vector<std::string> SplitColumns(const std::string& line);

    static void ExtractModelPaths(const std::vector<std::string>& cols, GameObjectKind kind, GameObjectRef& ref);

    void CollectSplitPathsFromIndex(const std::wstring& indexPath, GameObjectKind kind);

    struct PendingSplit {
        std::wstring path;
        GameObjectKind kind = GameObjectKind::Unknown;
    };

    std::wstring m_clientPath;
    std::vector<PendingSplit> m_pendingSplits;
    size_t m_splitLoadIndex = 0;

    std::unordered_map<std::string, GameObjectRef> m_byCodeName;

    std::unordered_map<int, std::string> m_byServiceId;

};



} // namespace sro

