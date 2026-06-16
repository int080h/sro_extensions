#pragma once
#include <string>
#include <vector>

class CpdResource {
public:
    std::string Signature;
    std::string Name;
    std::string CollisionPath;
    std::vector<std::string> ResourcePaths;

    bool Read(const std::wstring& path);
};
