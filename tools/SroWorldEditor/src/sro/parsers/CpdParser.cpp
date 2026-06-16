#include "CpdParser.h"
#include "ParserHelpers.h"
#include "core/FileSystem.h"
#include <fstream>

namespace {

bool ReadSizedString(std::ifstream& fs, std::string& out) {
    uint32_t len = 0;
    ReadVal(fs, len);
    if (!fs || len > 4096) return false;
    if (len == 0) {
        out.clear();
        return true;
    }
    out.resize(len);
    fs.read(out.data(), static_cast<std::streamsize>(len));
    return static_cast<bool>(fs);
}

} // namespace

bool CpdResource::Read(const std::wstring& path) {
    if (!FileExists(path)) return false;

    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return false;

    char sig[13] = {0};
    fs.read(sig, 12);
    Signature = sig;
    if (Signature.rfind("JMXVCPD", 0) != 0) return false;

    uint32_t collisionOffset = 0;
    uint32_t resourceOffset = 0;
    uint32_t unk[5]{};
    ReadVal(fs, collisionOffset);
    ReadVal(fs, resourceOffset);
    for (int i = 0; i < 5; ++i) ReadVal(fs, unk[i]);
    if (!fs) return false;

    uint32_t type = 0;
    ReadVal(fs, type);
    if (!ReadSizedString(fs, Name)) return false;

    uint32_t unk5 = 0;
    uint32_t unk6 = 0;
    ReadVal(fs, unk5);
    ReadVal(fs, unk6);
    if (!fs) return false;

    if (collisionOffset > 0) {
        fs.seekg(static_cast<std::streamoff>(collisionOffset), std::ios::beg);
        if (!ReadSizedString(fs, CollisionPath)) return false;
    }

    if (resourceOffset > 0) {
        fs.seekg(static_cast<std::streamoff>(resourceOffset), std::ios::beg);
        uint32_t resourceCount = 0;
        ReadVal(fs, resourceCount);
        if (!fs || resourceCount > 256) return false;
        ResourcePaths.clear();
        ResourcePaths.reserve(resourceCount);
        for (uint32_t i = 0; i < resourceCount; ++i) {
            std::string resourcePath;
            if (!ReadSizedString(fs, resourcePath)) return false;
            if (!resourcePath.empty()) {
                ResourcePaths.push_back(std::move(resourcePath));
            }
        }
    }

    return !ResourcePaths.empty();
}
