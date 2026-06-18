#pragma once
#include <string>

namespace sro {

class AssetPathResolver {
public:
    void SetClientPath(const std::wstring& clientPath) { m_clientPath = clientPath; }
    const std::wstring& ClientPath() const { return m_clientPath; }

    std::wstring ResolveRelativePath(const std::string& relPath) const;
    std::wstring ResolveRelativeToBase(const std::string& baseRelPath, const std::string& assetRelPath) const;
    bool Exists(const std::string& relPath) const;
    std::string NormalizeSlashes(std::string path) const;

private:
    std::wstring m_clientPath;
};

} // namespace sro
