#pragma once
#include <d3d9.h>
#include "core/Math.h"
#include "rendering/Camera.h"
#include "formats/DofFormat.h"

// Renders a parsed JMXVDOF dungeon (RTNavMeshDungeon): blocks (rooms) at their
// authored positions, the 3D voxel lookup grid (200u cells), off-mesh links
// between blocks, and per-block objects (ColObj/WaterObj). All read from disk.
class DofRenderer {
public:
    struct Vertex {
        Vector3 Pos;
        DWORD Color;
        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    };

    DofRenderer(LPDIRECT3DDEVICE9 device) : m_device(device) {}

    void Draw(const Matrix4& view, const Matrix4& proj, const sro::formats::DofDungeon& d,
              bool showBlocks, bool showVoxels, bool showLinks, bool showObjects,
              int selectedBlockIdx);

private:
    LPDIRECT3DDEVICE9 m_device;
};
