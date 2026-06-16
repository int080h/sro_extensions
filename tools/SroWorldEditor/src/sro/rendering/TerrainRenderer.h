#pragma once
#include <d3d9.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "rendering/Texture.h"
#include "core/Math.h"
#include "rendering/Camera.h"
#include "formats/MapFormats.h"

class TerrainRenderer {
public:
    struct Vertex {
        Vector3 Pos;
        DWORD Color;
        Vector2 UV;
        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
    };

    struct TerrainBatch {
        LPDIRECT3DVERTEXBUFFER9 Vb = nullptr;
        int VertexCount = 0;
        uint16_t TextureId = 0;
    };

    struct LoadedRegion {
        int Rx;
        int Ry;
        std::vector<TerrainBatch> BaseBatches;
        std::vector<TerrainBatch> BlendBatches;
    };

private:
    LPDIRECT3DDEVICE9 m_device;
    std::wstring m_clientPath;
    std::map<uint16_t, std::wstring> m_tileTextures;
    std::map<std::pair<int, int>, sro::formats::MapMesh> m_regionMeshes;
    std::map<uint16_t, Texture*> m_loadedTextures;
    std::vector<LoadedRegion> m_loadedRegions;
    std::set<std::pair<int, int>> m_loadedRegionSet;
    std::set<std::pair<int, int>> m_failedRegionSet;
    TextureManager* m_texManager;

    void LoadTileIfo();
    void LoadSingleRegion(int rx, int ry);
    void ClearBatches();

public:
    TerrainRenderer(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath, TextureManager* texMgr);
    ~TerrainRenderer();

    const std::map<uint16_t, std::wstring>& GetTileTextures() const { return m_tileTextures; }
    sro::formats::MapMesh* GetMapMesh(int rx, int ry) {
        auto it = m_regionMeshes.find({rx, ry});
        if (it != m_regionMeshes.end()) return &it->second;
        return nullptr;
    }
    void RebuildTerrainBuffers(int rx, int ry);
    void ClearLoadedTextures() { m_loadedTextures.clear(); }

    void LoadRegion(int rx, int ry, int radius);
    void UnloadRegion(int rx, int ry);
    void Draw(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy, int radius, const CameraFrustum& frustum, int timeOfDay = 0, bool wireframe = false);
    float GetHeight(float x, float z, int centerRx, int centerRy) const;
};
