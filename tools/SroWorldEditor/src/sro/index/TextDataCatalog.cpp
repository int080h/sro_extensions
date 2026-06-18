#include "TextDataCatalog.h"
#include "StringTableCatalog.h"

#include "core/FileSystem.h"

#include "core/Logger.h"

#include "core/TextDecode.h"

#include <fstream>

#include <sstream>

#include <algorithm>

#include <cctype>

#include <filesystem>



namespace sro {

namespace {



bool IsPlaceholder(const std::string& s) {

    return s.empty() || s == "xxx" || s == "XXX" || s == "-1";

}



bool LooksLikeModelPath(const std::string& s) {

    if (s.size() < 5) return false;

    const auto ext = s.substr(s.size() - 4);

    return ext == ".bsr" || ext == ".cpd" || ext == ".ddj" || ext == ".BMS";

}



std::wstring TextDataRoot(const std::wstring& clientPath) {

    const std::wstring paths[] = {

        clientPath + L"/Media/server_dep/silkroad/textdata",

        clientPath + L"/Media/textdata",

    };

    for (const auto& p : paths) {

        if (FileExists(p)) return p;

    }

    return paths[0];

}



} // namespace



std::vector<std::string> TextDataCatalog::SplitColumns(const std::string& line) {

    return TextDecode::SplitTabColumns(line);

}



void TextDataCatalog::ExtractModelPaths(const std::vector<std::string>& cols, GameObjectKind kind, GameObjectRef& ref) {

    auto add = [&](int idx) {

        if (idx < 0 || idx >= static_cast<int>(cols.size())) return;

        const auto& p = cols[idx];

        if (!IsPlaceholder(p) && LooksLikeModelPath(p)) {

            if (std::find(ref.modelPaths.begin(), ref.modelPaths.end(), p) == ref.modelPaths.end())

                ref.modelPaths.push_back(p);

        }

    };



    switch (kind) {

    case GameObjectKind::Character:

        add(52);

        break;

    case GameObjectKind::Item:

        add(52);

        add(53);

        add(54);

        break;

    default:

        for (int i = 0; i < static_cast<int>(cols.size()); ++i)

            add(i);

        break;

    }

}



bool TextDataCatalog::ParseSplitFile(const std::wstring& path, GameObjectKind kind) {

    std::vector<std::string> lines;

    if (!TextDecode::ReadTextDataLines(path, lines)) return false;



    const std::string src = ToNarrow(path);

    for (const std::string& line : lines) {

        if (line.empty() || line[0] == '/' || line[0] == ';') continue;

        auto cols = SplitColumns(line);

        if (cols.size() < 3) continue;

        if (!std::isdigit(static_cast<unsigned char>(cols[0][0]))) continue;



        GameObjectRef ref;

        ref.kind = kind;

        ref.sourceFile = src;

        ref.rawColumns = cols;

        try {

            ref.serviceId = std::stoi(ref.rawColumns[1]);

        } catch (...) {

            continue;

        }

        ref.codeName = ref.rawColumns.size() > 2 ? ref.rawColumns[2] : "";

        if (ref.codeName.empty()) continue;



        if (ref.rawColumns.size() > 3 && !IsPlaceholder(ref.rawColumns[3])

            && TextDecode::IsPrintableDisplayText(ref.rawColumns[3])) {

            ref.displayName = ref.rawColumns[3];

        }

        if (ref.rawColumns.size() > 4 && !IsPlaceholder(ref.rawColumns[4])
            && ref.rawColumns[4] != ref.codeName
            && ref.rawColumns[4].rfind("SN_", 0) != 0) {
            ref.parentCodeName = ref.rawColumns[4];
        }
        for (int i = 4; i <= 10 && i < static_cast<int>(ref.rawColumns.size()); ++i) {
            if (ref.rawColumns[i].rfind("SN_", 0) == 0) {
                ref.stringKey = ref.rawColumns[i];
                break;
            }
        }



        ExtractModelPaths(ref.rawColumns, kind, ref);

        m_byCodeName[ref.codeName] = std::move(ref);

    }

    return true;

}



bool TextDataCatalog::LoadIndexFile(const std::wstring& indexPath, GameObjectKind kind) {

    if (!FileExists(indexPath)) return false;

    std::vector<std::string> lines;

    if (!TextDecode::ReadTextDataLines(indexPath, lines)) return false;



    std::filesystem::path dir = std::filesystem::path(indexPath).parent_path();

    int loaded = 0;

    for (std::string line : lines) {

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        if (line.empty() || line.starts_with("//") || line.starts_with(";")) continue;

        if (line.size() < 5 || (line.back() != 't' && line.back() != 'T')) continue;

        const auto splitPath = dir / line;

        if (ParseSplitFile(splitPath.wstring(), kind)) ++loaded;

    }

    return loaded > 0;

}



void TextDataCatalog::CollectSplitPathsFromIndex(const std::wstring& indexPath, GameObjectKind kind) {

    if (!FileExists(indexPath)) return;

    std::vector<std::string> lines;

    if (!TextDecode::ReadTextDataLines(indexPath, lines)) return;



    const std::filesystem::path dir = std::filesystem::path(indexPath).parent_path();

    for (std::string line : lines) {

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        if (line.empty() || line.starts_with("//") || line.starts_with(";")) continue;

        if (line.size() < 5 || (line.back() != 't' && line.back() != 'T')) continue;

        PendingSplit entry;

        entry.path = (dir / line).wstring();

        entry.kind = kind;

        m_pendingSplits.push_back(std::move(entry));

    }

}



void TextDataCatalog::BeginIncrementalLoad(const std::wstring& clientPath) {

    m_clientPath = clientPath;

    m_byCodeName.clear();

    m_byServiceId.clear();

    m_pendingSplits.clear();

    m_splitLoadIndex = 0;



    const std::wstring root = TextDataRoot(clientPath);

    CollectSplitPathsFromIndex(root + L"/characterdata.txt", GameObjectKind::Character);

    CollectSplitPathsFromIndex(root + L"/itemdata.txt", GameObjectKind::Item);

}



bool TextDataCatalog::LoadNextSplit(std::wstring* outSplitPath) {

    if (m_splitLoadIndex >= m_pendingSplits.size()) return false;

    const PendingSplit& entry = m_pendingSplits[m_splitLoadIndex++];

    ParseSplitFile(entry.path, entry.kind);

    if (outSplitPath) *outSplitPath = std::filesystem::path(entry.path).filename().wstring();

    return true;

}



void TextDataCatalog::FinalizeServiceIds() {

    m_byServiceId.clear();

    for (const auto& [name, ref] : m_byCodeName) {

        m_byServiceId[ref.serviceId] = name;

        (void)name;

    }

}



bool TextDataCatalog::Load(const std::wstring& clientPath) {

    m_clientPath = clientPath;

    m_byCodeName.clear();

    m_byServiceId.clear();



    const std::wstring root = TextDataRoot(clientPath);

    struct Entry { const wchar_t* index; GameObjectKind kind; };

    const Entry entries[] = {

        {L"characterdata.txt", GameObjectKind::Character},

        {L"itemdata.txt", GameObjectKind::Item},

    };



    int totalFiles = 0;

    for (const auto& e : entries) {

        if (LoadIndexFile(root + L"/" + e.index, e.kind)) ++totalFiles;

    }

    FinalizeServiceIds();



    Logger::Instance().Info("TextDataCatalog loaded " + std::to_string(m_byCodeName.size())

        + " objects from " + std::to_string(totalFiles) + " index files (" + ToNarrow(root) + ")");

    return !m_byCodeName.empty();

}



std::string TextDataCatalog::ResolvePrimaryModelPath(const GameObjectRef& ref, int maxDepth) const {

    if (const std::string own = ref.PrimaryModelPath(); !own.empty()) return own;

    if (maxDepth <= 0 || ref.parentCodeName.empty()) return {};

    const auto* parent = FindByCodeName(ref.parentCodeName);

    if (!parent) return {};

    return ResolvePrimaryModelPath(*parent, maxDepth - 1);

}



const GameObjectRef* TextDataCatalog::FindByCodeName(const std::string& codeName) const {

    auto it = m_byCodeName.find(codeName);

    return it != m_byCodeName.end() ? &it->second : nullptr;

}



const GameObjectRef* TextDataCatalog::FindByServiceId(int serviceId) const {

    auto it = m_byServiceId.find(serviceId);

    if (it == m_byServiceId.end()) return nullptr;

    return FindByCodeName(it->second);

}



void TextDataCatalog::CollectAll(std::vector<const GameObjectRef*>& out) const {

    out.clear();

    out.reserve(m_byCodeName.size());

    for (const auto& [_, ref] : m_byCodeName) out.push_back(&ref);

}



void TextDataCatalog::CollectByKind(GameObjectKind kind, std::vector<const GameObjectRef*>& out) const {

    out.clear();

    for (const auto& [_, ref] : m_byCodeName) {

        if (ref.kind == kind) out.push_back(&ref);

    }

}



void TextDataCatalog::Search(const char* query, std::vector<const GameObjectRef*>& out, int maxResults) const {

    out.clear();

    if (!query || !query[0]) {

        CollectAll(out);

        if (static_cast<int>(out.size()) > maxResults) out.resize(maxResults);

        return;

    }

    std::string q = query;

    std::transform(q.begin(), q.end(), q.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (const auto& [_, ref] : m_byCodeName) {

        auto contains = [&](const std::string& s) {

            std::string lower = s;

            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            return lower.find(q) != std::string::npos;

        };

        if (contains(ref.codeName) || contains(ref.displayName) || contains(ref.stringKey)) {

            out.push_back(&ref);

        } else {

            for (const auto& mp : ref.modelPaths) {

                if (contains(mp)) { out.push_back(&ref); break; }

            }

        }

        if (static_cast<int>(out.size()) >= maxResults) break;

    }

}



void TextDataCatalog::CollectForFilter(CatalogFilter filter, const char* searchQuery,
                                       std::vector<const GameObjectRef*>& out,
                                       const StringTableCatalog* strings) const {

    out.clear();

    out.reserve(m_byCodeName.size());



    auto matchesSearch = [&](const GameObjectRef& ref) {

        if (!searchQuery || !searchQuery[0]) return true;

        std::string q = searchQuery;

        std::transform(q.begin(), q.end(), q.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        auto contains = [&](const std::string& s) {

            if (s.empty()) return false;

            std::string lower = s;

            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            return lower.find(q) != std::string::npos;

        };

        if (contains(ref.codeName) || contains(ref.displayName) || contains(ref.stringKey)) return true;

        if (strings) {
            auto tryKey = [&](const std::string& key) -> bool {
                if (key.empty()) return false;
                if (const auto* label = strings->FindEnglish(key)) return contains(*label);
                return false;
            };
            if (tryKey(ref.stringKey)) return true;
            if (!ref.codeName.empty() && ref.codeName.rfind("SN_", 0) != 0) {
                if (tryKey("SN_" + ref.codeName)) return true;
            }
            if (tryKey(ref.displayName)) return true;
        }

        for (const auto& mp : ref.modelPaths) {

            if (contains(mp)) return true;

        }

        return contains(std::to_string(ref.serviceId));

    };



    auto passesFilter = [&](const GameObjectRef& ref) {

        switch (filter) {

        case CatalogFilter::All:

            return true;

        case CatalogFilter::Characters:

            return ref.kind == GameObjectKind::Character && !ref.IsNpc() && !ref.IsMonster();

        case CatalogFilter::Items:

            return ref.kind == GameObjectKind::Item;

        case CatalogFilter::TextObjects:

            return ref.kind == GameObjectKind::MapObjectText;

        case CatalogFilter::Npcs:

            return ref.IsNpc();

        case CatalogFilter::Monsters:

            return ref.IsMonster();

        case CatalogFilter::MapObjects:

            return false;

        }

        return false;

    };



    for (const auto& [_, ref] : m_byCodeName) {

        if (!passesFilter(ref) || !matchesSearch(ref)) continue;

        out.push_back(&ref);

    }



    std::sort(out.begin(), out.end(), [](const GameObjectRef* a, const GameObjectRef* b) {

        return a->codeName < b->codeName;

    });

}



} // namespace sro

