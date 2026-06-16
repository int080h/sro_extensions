#pragma once
#include <d3d9.h>
#include <vector>
#include "core/Math.h"
#include "rendering/Camera.h"
#include "formats/NavMeshFormat.h"

class NvmRenderer {
public:
    struct Vertex {
        Vector3 Pos;
        DWORD Color;
        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    };

    struct NvmBatch {
        int Rx = 0;
        int Ry = 0;
        LPDIRECT3DVERTEXBUFFER9 VbWalkable = nullptr;
        int WalkableCount = 0;
        LPDIRECT3DVERTEXBUFFER9 VbBlocked = nullptr;
        int BlockedCount = 0;
        LPDIRECT3DVERTEXBUFFER9 VbEdges = nullptr;
        int EdgeCount = 0;
    };

private:
    LPDIRECT3DDEVICE9 m_device;
    std::vector<NvmBatch> m_batches;

public:
    NvmRenderer(LPDIRECT3DDEVICE9 device) : m_device(device) {}
    ~NvmRenderer();

    void ClearBuffers();
    void LoadNavmesh(const sro::formats::NavMesh& nav, int rx, int ry);
    void UnloadNavmesh(int rx, int ry);
    void Draw(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy, int radius, bool showWalkable, bool showBlocked, bool showEdges, const CameraFrustum& frustum);
    void DrawCellsAndSelection(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                               const sro::formats::NavMesh& nav, int rx, int ry,
                               int selectedCellIdx, int selectedEdgeIdx, bool selectedEdgeIsGlobal);
    float GetHeight(float localX, float localZ, const std::vector<float>& heightMap) const;
    void RebuildNavmeshBuffers(const sro::formats::NavMesh* nav, int rx, int ry);

    const std::vector<NvmBatch>& GetBatches() const { return m_batches; }
};
