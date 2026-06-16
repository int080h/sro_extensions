#pragma once
#include <d3d9.h>
#include "Core/Math.h"

// ============================================================================
// Sky gradient renderer — draws the background sky quad
// ============================================================================

inline void DrawSkyGradient(LPDIRECT3DDEVICE9 device, float width, float height, DWORD colTop, DWORD colBottom) {
    if (!device) return;
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    struct SkyVertex {
        float x, y, z, rhw;
        DWORD color;
    };

    SkyVertex vertices[4] = {
        { -0.5f,         -0.5f,          1.0f, 1.0f, colTop },
        { width - 0.5f,  -0.5f,          1.0f, 1.0f, colTop },
        { -0.5f,         height - 0.5f, 1.0f, 1.0f, colBottom },
        { width - 0.5f,  height - 0.5f, 1.0f, 1.0f, colBottom }
    };

    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    device->SetTexture(0, nullptr);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(SkyVertex));

    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}
