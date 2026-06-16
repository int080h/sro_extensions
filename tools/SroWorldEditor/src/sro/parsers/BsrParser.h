#pragma once
#include <string>
#include <vector>

struct BsrMeshGroup {
    std::string BoneName;
    std::vector<uint32_t> MeshIndices;
};

class BsrResource {
public:
    std::string Signature;
    std::vector<std::string> MeshPaths;
    std::vector<std::string> MaterialPaths;
    std::string SkeletonPath;
    std::vector<BsrMeshGroup> MeshGroups;

    bool Read(const std::wstring& path);
};
