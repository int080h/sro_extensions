#pragma once
#include "core/MathTypes.h"
#include <cstdint>
#include <string>
#include <vector>

struct FrameTextureSlideData {
    Vector3 Left = Vector3(1.0f, 1.0f, 1.0f);
    std::vector<Vector4> Keys;
    float Start = 0.0f;
    float End = 1.0f;
    float Speed = 1.0f;
    uint8_t Byte0 = 0;
    uint8_t Byte1 = 0;
    bool Valid = false;

    bool HasKeys() const { return !Keys.empty(); }
    int Cols() const { return static_cast<int>(Left.x > 0.0f ? Left.x : 1.0f); }
    int Rows() const { return static_cast<int>(Left.y > 0.0f ? Left.y : 1.0f); }
};

struct EfpEmitterDef {
    int MinParticles = 1;
    int MaxParticles = 8;
    int BurstRate = 1;
    float SpawnRate = 1.0f;
    bool Valid = false;
};

struct EfpRenderDef {
    std::string Shape;
    std::string ViewMode;
    std::string MeshPath;
    std::vector<std::string> TexturePaths;
    FrameTextureSlideData TextureSlide;
    int TwoSided = 0;
    int SrcBlend = 5;  // D3DBLEND_SRCALPHA
    int DstBlend = 2;  // D3DBLEND_ONE
};

struct EfpObjectDef {
    std::string Name;
    EfpEmitterDef Emitter;
    std::string LifeCommand;
    EfpRenderDef Render;
    std::vector<EfpObjectDef> Children;
};

struct EfpRenderItem {
    std::string Shape;
    std::string MeshPath;
    std::vector<std::string> TexturePaths;
    std::string ViewMode;
    bool TextureSlide = false;
    FrameTextureSlideData SlideData;
    EfpEmitterDef Emitter;
    bool HasEmitter = false;
    int SrcBlend = 5;
    int DstBlend = 2;
};

inline bool EfpItemUsesMeshGeometry(const EfpRenderItem& item) {
    if (item.Shape == "RenderMesh") return !item.MeshPath.empty();
    if (item.ViewMode == "ViewNone" && !item.MeshPath.empty()) return true;
    return false;
}

struct EfpResource {
    std::string Signature;
    std::vector<std::string> TexturePaths;
    std::vector<std::string> MeshPaths;
    std::vector<EfpRenderItem> RenderItems;
    std::vector<EfpObjectDef> Objects;
    bool HasRenderPlate = false;
    bool HasRenderMesh = false;
    bool HasBillboard = false;
    bool HasTextureSlide = false;
    bool ParsedHeuristic = false;
};

class EfpParser {
public:
    bool Read(const std::wstring& path, EfpResource& out);
};
