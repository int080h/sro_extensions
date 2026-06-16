#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct BmsVertex {
    float X, Y, Z;     // Position
    float NX, NY, NZ;  // Normal
    float U, V;        // UV coordinates
};

struct BmsFace {
    uint16_t A, B, C;  // Indices
};

class BmsMesh {
public:
    std::string Signature;
    std::vector<BmsVertex> Vertices;
    std::vector<BmsFace> Faces;
    std::string Material;

    bool Read(const std::wstring& path);
    std::vector<float> GetFlattenedVertices() const;
    std::vector<uint32_t> GetFlattenedIndices() const;
};
