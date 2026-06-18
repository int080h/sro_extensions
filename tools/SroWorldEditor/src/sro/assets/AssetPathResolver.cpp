#include "assets/AssetPathResolver.h"

#include "core/FileSystem.h"

#include <algorithm>



namespace sro {



std::string AssetPathResolver::NormalizeSlashes(std::string path) const {

    std::replace(path.begin(), path.end(), '\\', '/');

    while (!path.empty() && path.front() == '/') path.erase(path.begin());

    return path;

}



std::wstring AssetPathResolver::ResolveRelativePath(const std::string& relPath) const {

    if (relPath.empty() || m_clientPath.empty()) return {};

    const std::string norm = NormalizeSlashes(relPath);

    const std::wstring wRel(norm.begin(), norm.end());



    const std::wstring candidates[] = {

        m_clientPath + L"/" + wRel,

        m_clientPath + L"/Particles/" + wRel,

        m_clientPath + L"/Data/" + wRel,

        m_clientPath + L"/Data/res/" + wRel,

        m_clientPath + L"/Data/Prim/" + wRel,

        m_clientPath + L"/Data/Effect/" + wRel,

        m_clientPath + L"/Effect/" + wRel,

        m_clientPath + L"/Map/" + wRel,

    };

    for (const auto& p : candidates) {

        if (FileExists(p)) return p;

    }

    return {};

}



std::wstring AssetPathResolver::ResolveRelativeToBase(const std::string& baseRelPath,

                                                      const std::string& assetRelPath) const {

    const auto direct = ResolveRelativePath(assetRelPath);

    if (!direct.empty() && FileExists(direct)) return direct;

    if (baseRelPath.empty() || assetRelPath.empty()) return direct;



    std::string base = NormalizeSlashes(baseRelPath);

    const auto slash = base.find_last_of('/');

    if (slash == std::string::npos) return direct;



    const std::string combined = base.substr(0, slash + 1) + NormalizeSlashes(assetRelPath);

    const auto resolved = ResolveRelativePath(combined);

    if (!resolved.empty() && FileExists(resolved)) return resolved;

    return resolved.empty() ? direct : resolved;

}



bool AssetPathResolver::Exists(const std::string& relPath) const {

    const auto resolved = ResolveRelativePath(relPath);

    return !resolved.empty() && FileExists(resolved);

}



} // namespace sro

