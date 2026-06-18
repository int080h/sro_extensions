#include "StringTableCatalog.h"

#include "core/FileSystem.h"

#include "core/Logger.h"

#include "core/TextDecode.h"

#include <filesystem>

#include <vector>

#include <algorithm>

#include <cctype>

#include <cwctype>



namespace sro {

namespace {



bool IsPlaceholder(const std::string& s) {

    return s.empty() || s == "xxx" || s == "XXX" || s == "0" || s == "-1";

}



bool IsDirectStringKey(const std::string& key) {

    return key.rfind("SN_", 0) == 0

        || key.rfind("STORE_", 0) == 0

        || key.rfind("NPC_", 0) == 0;

}



void ToLowerAscii(std::string& s) {

    std::transform(s.begin(), s.end(), s.begin(),

        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

}



bool WideEqualsIgnoreCase(const std::wstring& a, const std::wstring& b) {

    if (a.size() != b.size()) return false;

    for (size_t i = 0; i < a.size(); ++i) {

        if (std::towlower(a[i]) != std::towlower(b[i])) return false;

    }

    return true;

}



bool WideStartsWithIgnoreCase(const std::wstring& haystack, const std::wstring& prefix) {

    if (prefix.size() > haystack.size()) return false;

    for (size_t i = 0; i < prefix.size(); ++i) {

        if (std::towlower(haystack[i]) != std::towlower(prefix[i])) return false;

    }

    return true;

}



} // namespace



std::wstring StringTableCatalog::TextDataRoot(const std::wstring& clientPath) {

    const std::wstring paths[] = {

        clientPath + L"/Media/server_dep/silkroad/textdata",

        clientPath + L"/Media/textdata",

    };

    for (const auto& p : paths) {

        if (FileExists(p)) return p;

    }

    return paths[0];

}



std::string StringTableCatalog::PickEnglishLabel(const std::vector<std::string>& cols) {

    auto tryCol = [&](size_t idx) -> std::string {

        if (idx >= cols.size()) return {};

        std::string label = TextDecode::StripTranslationQuotes(cols[idx]);

        if (IsPlaceholder(label)) return {};

        if (!TextDecode::IsMostlyLatinText(label)) return {};

        return label;

    };



    if (std::string label = tryCol(9); !label.empty()) return label;

    for (size_t i = 8; i <= 12 && i < cols.size(); ++i) {

        if (i == 9) continue;

        if (std::string label = tryCol(i); !label.empty()) return label;

    }

    return {};

}



void StringTableCatalog::StoreString(const std::string& key, const std::string& label) {

    if (key.empty() || label.empty()) return;

    m_strings[key] = label;

}



bool StringTableCatalog::ParseSplitFile(const std::wstring& path) {

    std::vector<std::string> lines;

    if (!TextDecode::ReadTextDataLines(path, lines)) return false;



    for (const std::string& line : lines) {

        if (line.empty() || line[0] == '/' || line[0] == ';') continue;

        auto cols = TextDecode::SplitTabColumns(line);

        if (cols.size() < 4) continue;



        const std::string label = PickEnglishLabel(cols);

        if (label.empty()) continue;



        const std::string& key = cols[2];

        if (IsDirectStringKey(key)) {

            StoreString(key, label);

        }



        if (cols.size() > 3) {

            const std::string alias = TextDecode::StripTranslationQuotes(cols[3]);

            if (!IsPlaceholder(alias) && alias != key && alias != label) {

                StoreString(alias, label);

            }

        }

    }

    return true;

}



bool StringTableCatalog::LoadIndexFile(const std::wstring& indexPath) {

    if (!FileExists(indexPath)) return false;

    std::vector<std::string> lines;

    if (!TextDecode::ReadTextDataLines(indexPath, lines)) return false;



    std::filesystem::path dir = std::filesystem::path(indexPath).parent_path();

    int loaded = 0;

    for (std::string line : lines) {

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        if (line.empty() || line.starts_with("//") || line.starts_with(";")) continue;

        if (line.size() < 5 || (line.back() != 't' && line.back() != 'T')) continue;

        if (ParseSplitFile((dir / line).wstring())) ++loaded;

    }

    return loaded > 0;

}



bool StringTableCatalog::IsStringIndexFile(const std::wstring& filename) const {

    if (filename.size() < 5 || filename.substr(filename.size() - 4) != L".txt") return false;

    const std::wstring nameWithoutExt = filename.substr(0, filename.size() - 4);

    for (const auto& prefix : m_prefixes) {

        if (WideEqualsIgnoreCase(nameWithoutExt, prefix)) return true;

    }

    return false;

}



bool StringTableCatalog::IsStringSplitFile(const std::wstring& filename) const {

    if (filename.size() < 5 || filename.substr(filename.size() - 4) != L".txt") return false;

    const std::wstring nameWithoutExt = filename.substr(0, filename.size() - 4);

    for (const auto& prefix : m_prefixes) {

        const std::wstring splitPrefix = prefix + L"_";

        if (nameWithoutExt.size() > splitPrefix.size()

            && WideStartsWithIgnoreCase(nameWithoutExt, splitPrefix)) {

            return true;

        }

    }

    return false;

}



void StringTableCatalog::LogLoadStats() const {

    size_t snItem = 0;

    size_t snMob = 0;

    size_t snZone = 0;

    for (const auto& [key, _] : m_strings) {

        if (key.rfind("SN_ITEM_", 0) == 0) ++snItem;

        else if (key.rfind("SN_MOB_", 0) == 0) ++snMob;

        else if (key.rfind("SN_", 0) == 0) {

            std::string upper = key;

            ToLowerAscii(upper);

            if (upper.find("zone") != std::string::npos || upper.find("region") != std::string::npos

                || upper.find("dungeon") != std::string::npos || upper.find("temple") != std::string::npos

                || upper.find("fort") != std::string::npos) {

                ++snZone;

            }

        }

    }

    Logger::Instance().Info("StringTableCatalog loaded " + std::to_string(m_strings.size())

        + " strings (SN_ITEM=" + std::to_string(snItem)

        + ", SN_MOB=" + std::to_string(snMob)

        + ", zone-related SN=" + std::to_string(snZone) + ")");

}



bool StringTableCatalog::Load(const std::wstring& clientPath) {

    BeginIncrementalLoad(clientPath);

    std::wstring path;

    while (LoadNextIndexFile(&path)) {}

    m_loaded = !m_strings.empty();

    if (m_loaded) LogLoadStats();

    return m_loaded;

}



void StringTableCatalog::BeginIncrementalLoad(const std::wstring& clientPath) {

    m_clientPath = clientPath;

    m_strings.clear();

    m_pendingIndexFiles.clear();

    m_indexLoadPos = 0;

    m_loaded = false;



    m_prefixes = {

        L"textdata_object",

        L"textdata_equip&skill",

        L"textuisystem",

        L"textquest_speech&name",

        L"textquest_queststring",

        L"textzonename",

        L"textquest_otherstring",

        L"texthelp",

        L"textevent",

    };



    const std::wstring root = TextDataRoot(clientPath);

    if (!FileExists(root)) return;



    std::error_code ec;

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {

        if (!entry.is_regular_file()) continue;

        const std::wstring filename = entry.path().filename().wstring();

        if (IsStringIndexFile(filename) || IsStringSplitFile(filename)) {

            m_pendingIndexFiles.push_back(entry.path().wstring());

        }

    }



    std::sort(m_pendingIndexFiles.begin(), m_pendingIndexFiles.end());

}



bool StringTableCatalog::LoadNextIndexFile(std::wstring* outIndexPath) {

    if (m_indexLoadPos >= m_pendingIndexFiles.size()) {

        if (!m_loaded && !m_strings.empty()) {

            m_loaded = true;

            LogLoadStats();

        }

        return false;

    }

    const std::wstring& path = m_pendingIndexFiles[m_indexLoadPos++];

    const std::wstring filename = std::filesystem::path(path).filename().wstring();

    if (IsStringSplitFile(filename)) {

        ParseSplitFile(path);

    } else {

        LoadIndexFile(path);

    }

    if (outIndexPath) *outIndexPath = path;

    return true;

}



std::string StringTableCatalog::Resolve(const std::string& key) const {

    if (const auto* v = Find(key)) return *v;

    return key;

}



const std::string* StringTableCatalog::Find(const std::string& key) const {

    auto it = m_strings.find(key);

    return it != m_strings.end() ? &it->second : nullptr;

}



} // namespace sro

