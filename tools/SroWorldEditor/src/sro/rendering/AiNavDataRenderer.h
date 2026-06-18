#pragma once
#include <d3d9.h>
#include "core/Math.h"
#include "formats/AiNavDataFormat.h"

// Renders parsed ainavdata: simple-dungeon edge centers (points/lines) and ref-block links.
class AiNavDataRenderer {
public:
    struct Vertex {
        Vector3 Pos;
        DWORD Color;
        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    };

    AiNavDataRenderer() = default;

    void Initialize(LPDIRECT3DDEVICE9 device) { m_device = device; }

    void Draw(const Matrix4& view, const Matrix4& proj, const sro::formats::AiNavData& data,
              bool showEdges, bool showLinks);

private:
    LPDIRECT3DDEVICE9 m_device = nullptr;
};
