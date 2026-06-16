#include "cache/ClientDataCache.h"
#include "parsers/MapMeshParser.h"
#include "parsers/MapPlacementParser.h"
#include "parsers/NavMeshParser.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <fstream>
#include <filesystem>
#include <cctype>
#include <exception>
#include <shlobj.h>

namespace sro {

namespace {

std::wstring LocalAppDataRoot() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
        return L"";
    std::wstring base(path);
    CoTaskMemFree(path);
    return base;
}

uint64_t FileTimeToU64(const std::filesystem::file_time_type& ft) {
    return static_cast<uint64_t>(ft.time_since_epoch().count());
}

bool SourceIdentity(const std::wstring& sourcePath, uint64_t& mtime, uint64_t& size) {
    std::error_code ec;
    if (!std::filesystem::exists(sourcePath, ec)) return false;
    mtime = FileTimeToU64(std::filesystem::last_write_time(sourcePath, ec));
    if (ec) return false;
    size = static_cast<uint64_t>(std::filesystem::file_size(sourcePath, ec));
    return !ec;
}

std::wstring HashWide(const std::wstring& s) {
    std::hash<std::wstring> h;
    return std::to_wstring(h(s));
}

bool ParseObjectIndexLine(const std::string& line, uint32_t& objId, std::string& bsrPath) {
    if (line.empty() || !std::isdigit(static_cast<unsigned char>(line[0]))) return false;
    size_t space1 = line.find_first_of(" \t");
    if (space1 == std::string::npos) return false;
    size_t space2 = line.find_first_of(" \t", space1 + 1);
    if (space2 == std::string::npos) return false;
    try {
        objId = static_cast<uint32_t>(std::stoul(line.substr(0, space1)));
        size_t q1 = line.find('\"', space2 + 1);
        if (q1 == std::string::npos) return false;
        size_t q2 = line.find('\"', q1 + 1);
        if (q2 == std::string::npos) return false;
        bsrPath = line.substr(q1 + 1, q2 - q1 - 1);
        while (!bsrPath.empty() && (bsrPath.back() == '\r' || bsrPath.back() == '\n' || bsrPath.back() == '\t' || bsrPath.back() == ' '))
            bsrPath.pop_back();
        return !bsrPath.empty();
    } catch (...) {
        return false;
    }
}

bool ParseObjectIndexFile(const std::filesystem::path& sourcePath, std::unordered_map<uint32_t, std::string>& out) {
    std::ifstream fs{sourcePath, std::ios::binary};
    if (!fs.is_open()) return false;
    out.clear();
    std::string line;
    while (std::getline(fs, line)) {
        uint32_t objId = 0;
        std::string bsrPath;
        if (ParseObjectIndexLine(line, objId, bsrPath))
            out[objId] = std::move(bsrPath);
    }
    return !out.empty();
}

} // namespace

ClientDataCache& ClientDataCache::Instance() {
    static ClientDataCache cache;
    return cache;
}

std::wstring ClientDataCache::CacheRootForClient(const std::wstring& clientPath) const {
    const std::wstring base = LocalAppDataRoot();
    if (base.empty()) return L"";
    std::filesystem::path root = std::filesystem::path(base) / L"SroWorldEditor" / L"cache" / HashWide(clientPath);
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    return root.wstring();
}

void ClientDataCache::SetClientPath(const std::wstring& clientPath) {
    m_clientPath = clientPath;
    m_cacheRoot = CacheRootForClient(clientPath);
}

void ClientDataCache::ClearAll() {
    const std::wstring base = LocalAppDataRoot();
    if (base.empty()) return;
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(base) / L"SroWorldEditor" / L"cache", ec);
    Logger::Instance().Info("Cleared all client data caches.");
}

void ClientDataCache::ClearCurrentClient() {
    if (m_cacheRoot.empty()) return;
    std::error_code ec;
    std::filesystem::remove_all(m_cacheRoot, ec);
    if (!m_clientPath.empty())
        m_cacheRoot = CacheRootForClient(m_clientPath);
    Logger::Instance().Info("Cleared client data cache for current client.");
}

std::wstring ClientDataCache::EntryPath(const std::wstring& sourcePath, const wchar_t* suffix) const {
    if (m_cacheRoot.empty()) return L"";
    std::filesystem::path p = std::filesystem::path(m_cacheRoot) / (HashWide(sourcePath) + suffix);
    return p.wstring();
}

std::wstring ClientDataCache::MetaPath(const std::wstring& cachePath) const {
    return cachePath + L".meta";
}

bool ClientDataCache::IsMetaValid(const std::wstring& sourcePath, const std::wstring& metaPath) const {
    if (!FileExists(metaPath)) return false;
    uint64_t srcMtime = 0, srcSize = 0;
    if (!SourceIdentity(sourcePath, srcMtime, srcSize)) return false;

    std::ifstream in{std::filesystem::path(metaPath)};
    if (!in) return false;

    std::string versionTag, storedPath;
    uint64_t storedMtime = 0, storedSize = 0;
    if (!(in >> versionTag >> storedMtime >> storedSize)) return false;
    if (!std::getline(in, storedPath)) return false;
    if (!storedPath.empty() && storedPath[0] == ' ') storedPath.erase(0, 1);

    if (versionTag != "v1") return false;
    return storedMtime == srcMtime && storedSize == srcSize
        && storedPath == ToNarrow(sourcePath);
}

void ClientDataCache::WriteMeta(const std::wstring& sourcePath, const std::wstring& metaPath) const {
    uint64_t mtime = 0, size = 0;
    if (!SourceIdentity(sourcePath, mtime, size)) return;

    std::ofstream out{std::filesystem::path(metaPath)};
    if (!out) return;
    out << "v1 " << mtime << ' ' << size << ' ' << ToNarrow(sourcePath) << '\n';
}

bool ClientDataCache::LoadMapMesh(const std::wstring& sourcePath, formats::MapMesh& out) {
    try {
        const std::wstring cachePath = EntryPath(sourcePath, L".m.cache");
        const std::wstring metaPath = MetaPath(cachePath);
        if (!cachePath.empty() && IsMetaValid(sourcePath, metaPath)) {
            if (MapMeshParser::Read(cachePath, out).ok) {
                Logger::Instance().Info("Cache hit: MapMesh " + ToNarrow(sourcePath));
                return true;
            }
        }
        if (!MapMeshParser::Read(sourcePath, out).ok) return false;
        if (!cachePath.empty() && MapMeshParser::Write(cachePath, out).ok)
            WriteMeta(sourcePath, metaPath);
        return true;
    } catch (const std::exception& ex) {
        Logger::Instance().Warning(std::string("MapMesh cache error: ") + ex.what());
        return MapMeshParser::Read(sourcePath, out).ok;
    } catch (...) {
        Logger::Instance().Warning("MapMesh cache error: unknown exception");
        return MapMeshParser::Read(sourcePath, out).ok;
    }
}

bool ClientDataCache::LoadMapPlacements(const std::wstring& sourcePath, formats::MapPlacements& out) {
    try {
        const std::wstring cachePath = EntryPath(sourcePath, L".o.cache");
        const std::wstring metaPath = MetaPath(cachePath);
        if (!cachePath.empty() && IsMetaValid(sourcePath, metaPath)) {
            if (MapPlacementParser::Read(cachePath, out).ok) {
                Logger::Instance().Info("Cache hit: MapPlacements " + ToNarrow(sourcePath));
                return true;
            }
        }
        if (!MapPlacementParser::Read(sourcePath, out).ok) return false;
        if (!cachePath.empty() && MapPlacementParser::Write(cachePath, out).ok)
            WriteMeta(sourcePath, metaPath);
        return true;
    } catch (const std::exception& ex) {
        Logger::Instance().Warning(std::string("MapPlacements cache error: ") + ex.what());
        return MapPlacementParser::Read(sourcePath, out).ok;
    } catch (...) {
        Logger::Instance().Warning("MapPlacements cache error: unknown exception");
        return MapPlacementParser::Read(sourcePath, out).ok;
    }
}

bool ClientDataCache::LoadNavMesh(const std::wstring& sourcePath, formats::NavMesh& out) {
    try {
        const std::wstring cachePath = EntryPath(sourcePath, L".nvm.cache");
        const std::wstring metaPath = MetaPath(cachePath);
        if (!cachePath.empty() && IsMetaValid(sourcePath, metaPath)) {
            if (NavMeshParser::Read(cachePath, out).ok) {
                Logger::Instance().Info("Cache hit: NavMesh " + ToNarrow(sourcePath));
                return true;
            }
        }
        if (!NavMeshParser::Read(sourcePath, out).ok) return false;
        if (!cachePath.empty() && NavMeshParser::Write(cachePath, out).ok)
            WriteMeta(sourcePath, metaPath);
        return true;
    } catch (const std::exception& ex) {
        Logger::Instance().Warning(std::string("NavMesh cache error: ") + ex.what());
        return NavMeshParser::Read(sourcePath, out).ok;
    } catch (...) {
        Logger::Instance().Warning("NavMesh cache error: unknown exception");
        return NavMeshParser::Read(sourcePath, out).ok;
    }
}

bool ClientDataCache::LoadObjectIndex(const std::wstring& sourcePath, std::unordered_map<uint32_t, std::string>& out) {
    try {
        const std::filesystem::path sourceFs(sourcePath);
        const std::wstring cachePath = EntryPath(sourcePath, L".ifo.txt");
        const std::wstring metaPath = MetaPath(cachePath);

        if (!cachePath.empty() && IsMetaValid(sourcePath, metaPath)) {
            std::ifstream in{std::filesystem::path(cachePath)};
            if (in) {
                out.clear();
                std::string line;
                while (std::getline(in, line)) {
                    uint32_t objId = 0;
                    std::string bsrPath;
                    if (ParseObjectIndexLine(line, objId, bsrPath))
                        out[objId] = std::move(bsrPath);
                }
                if (!out.empty()) {
                    Logger::Instance().Info("Cache hit: object index " + ToNarrow(sourcePath));
                    return true;
                }
            }
        }

        if (!ParseObjectIndexFile(sourceFs, out)) return false;

        if (!cachePath.empty()) {
            std::ofstream cout{std::filesystem::path(cachePath)};
            if (cout) {
                for (const auto& [id, path] : out)
                    cout << id << " \"" << path << "\"\n";
                WriteMeta(sourcePath, metaPath);
            }
        }
        return true;
    } catch (const std::exception& ex) {
        Logger::Instance().Warning(std::string("Object index cache error: ") + ex.what());
        out.clear();
        return ParseObjectIndexFile(std::filesystem::path(sourcePath), out);
    } catch (...) {
        Logger::Instance().Warning("Object index cache error: unknown exception");
        out.clear();
        return ParseObjectIndexFile(std::filesystem::path(sourcePath), out);
    }
}

} // namespace sro
