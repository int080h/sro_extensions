#include "AiNavDataRenderer.h"
#include <vector>
#include <cmath>

static void DrawPointCross(LPDIRECT3DDEVICE9 device, const Vector3& p, float s, DWORD color) {
    AiNavDataRenderer::Vertex v[4] = {
        {Vector3(p.x - s, p.y, p.z), color}, {Vector3(p.x + s, p.y, p.z), color},
        {Vector3(p.x, p.y, p.z - s), color}, {Vector3(p.x, p.y, p.z + s), color},
    };
    device->DrawPrimitiveUP(D3DPT_LINELIST, 2, v, sizeof(AiNavDataRenderer::Vertex));
}

void AiNavDataRenderer::Draw(const Matrix4& view, const Matrix4& proj, const sro::formats::AiNavData& data,
                              bool showEdges, bool showLinks) {
    if (!m_device) return;

    m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
    m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_device->SetTexture(0, nullptr);
    m_device->SetFVF(Vertex::FVF);
    Matrix4 world = Matrix4::Identity();
    m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

    const DWORD edgeCol  = D3DCOLOR_ARGB(220, 0, 255, 200);
    const DWORD linkCol  = D3DCOLOR_ARGB(220, 255, 0, 255);
    const float crossSize = 8.0f;

    auto edgeCenter = [&](uint32_t blockIdx, uint32_t edgeIdx) -> Vector3 {
        if (blockIdx >= data.SimpleDungeon.Blocks.size()) return {};
        const auto& block = data.SimpleDungeon.Blocks[blockIdx];
        if (edgeIdx >= block.EdgeCount) return {};
        return Vector3(block.EdgeCenterX[edgeIdx], block.EdgeCenterY[edgeIdx], block.EdgeCenterZ[edgeIdx]);
    };

    if (showEdges) {
        std::vector<Vertex> edgeLines;
        for (const auto& block : data.SimpleDungeon.Blocks) {
            for (uint32_t j = 0; j < block.EdgeCount; ++j) {
                Vector3 p(block.EdgeCenterX[j], block.EdgeCenterY[j], block.EdgeCenterZ[j]);
                DrawPointCross(m_device, p, crossSize, edgeCol);
                if (j + 1 < block.EdgeCount) {
                    Vector3 q(block.EdgeCenterX[j + 1], block.EdgeCenterY[j + 1], block.EdgeCenterZ[j + 1]);
                    edgeLines.push_back({p, edgeCol});
                    edgeLines.push_back({q, edgeCol});
                }
            }
        }
        if (!edgeLines.empty()) {
            m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
            m_device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)edgeLines.size() / 2, edgeLines.data(), sizeof(Vertex));
            m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
        }
    }

    if (showLinks) {
        std::vector<Vertex> linkLines;
        for (size_t bi = 0; bi < data.RefDungeon.Blocks.size(); ++bi) {
            const auto& refBlock = data.RefDungeon.Blocks[bi];
            for (const auto& link : refBlock.Links) {
                uint32_t srcEdge = link.CellID;
                if (srcEdge >= refBlock.EdgeCount && refBlock.EdgeCount > 0)
                    srcEdge = refBlock.EdgeCount - 1;
                Vector3 a = edgeCenter(static_cast<uint32_t>(bi), srcEdge);
                Vector3 b = edgeCenter(link.LinkedObjID, link.LinkedObjRefEdgeIndex);
                if (a.x == 0.f && a.y == 0.f && a.z == 0.f &&
                    b.x == 0.f && b.y == 0.f && b.z == 0.f)
                    continue;
                linkLines.push_back({a, linkCol});
                linkLines.push_back({b, linkCol});
            }
        }
        if (!linkLines.empty()) {
            m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
            m_device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)linkLines.size() / 2, linkLines.data(), sizeof(Vertex));
            m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
        }
    }

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}
