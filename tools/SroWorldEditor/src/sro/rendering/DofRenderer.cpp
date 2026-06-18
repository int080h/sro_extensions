#include "DofRenderer.h"
#include <vector>
#include <cmath>
#include <algorithm>

static void DrawAabb(LPDIRECT3DDEVICE9 device, const Vector3& mn, const Vector3& mx, DWORD color) {
    DofRenderer::Vertex v[24] = {
        {Vector3(mn.x, mx.y, mn.z), color}, {Vector3(mx.x, mx.y, mn.z), color},
        {Vector3(mx.x, mx.y, mn.z), color}, {Vector3(mx.x, mx.y, mx.z), color},
        {Vector3(mx.x, mx.y, mx.z), color}, {Vector3(mn.x, mx.y, mx.z), color},
        {Vector3(mn.x, mx.y, mx.z), color}, {Vector3(mn.x, mx.y, mn.z), color},
        {Vector3(mn.x, mn.y, mn.z), color}, {Vector3(mx.x, mn.y, mn.z), color},
        {Vector3(mx.x, mn.y, mn.z), color}, {Vector3(mx.x, mn.y, mx.z), color},
        {Vector3(mx.x, mn.y, mx.z), color}, {Vector3(mn.x, mn.y, mx.z), color},
        {Vector3(mn.x, mn.y, mx.z), color}, {Vector3(mn.x, mn.y, mn.z), color},
        {Vector3(mn.x, mn.y, mn.z), color}, {Vector3(mn.x, mx.y, mn.z), color},
        {Vector3(mx.x, mn.y, mn.z), color}, {Vector3(mx.x, mx.y, mn.z), color},
        {Vector3(mx.x, mn.y, mx.z), color}, {Vector3(mx.x, mx.y, mx.z), color},
        {Vector3(mn.x, mn.y, mx.z), color}, {Vector3(mn.x, mx.y, mx.z), color},
    };
    device->DrawPrimitiveUP(D3DPT_LINELIST, 12, v, sizeof(DofRenderer::Vertex));
}

void DofRenderer::Draw(const Matrix4& view, const Matrix4& proj, const sro::formats::DofDungeon& d,
                        bool showBlocks, bool showVoxels, bool showLinks, bool showObjects,
                        int selectedBlockIdx) {
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

    const DWORD blockCol = D3DCOLOR_ARGB(180, 120, 200, 255);
    const DWORD selCol   = D3DCOLOR_ARGB(255, 255, 255, 0);
    const DWORD linkCol  = D3DCOLOR_ARGB(220, 255, 0, 255);
    const DWORD voxelCol = D3DCOLOR_ARGB(120, 200, 200, 200);
    const DWORD objCol   = D3DCOLOR_ARGB(200, 230, 230, 230);
    const DWORD colObjCol = D3DCOLOR_ARGB(220, 230, 60, 60);
    const DWORD waterCol = D3DCOLOR_ARGB(220, 60, 120, 230);

    // Blocks: authored collision box if valid, else a 200u representative cube.
    if (showBlocks) {
        for (int i = 0; i < (int)d.Blocks.size(); ++i) {
            const auto& b = d.Blocks[i];
            Vector3 mn(0, 0, 0), mx(0, 0, 0);
            bool haveBox = false;
            // CollisionBox0 is a raw 24-float array; interpret first 6 as min/max AABB.
            if (d.Blocks[i].CollisionBox0[3] > d.Blocks[i].CollisionBox0[0] ||
                d.Blocks[i].CollisionBox0[4] > d.Blocks[i].CollisionBox0[1] ||
                d.Blocks[i].CollisionBox0[5] > d.Blocks[i].CollisionBox0[2]) {
                mn = Vector3(b.CollisionBox0[0], b.CollisionBox0[1], b.CollisionBox0[2]);
                mx = Vector3(b.CollisionBox0[3], b.CollisionBox0[4], b.CollisionBox0[5]);
                haveBox = true;
            }
            if (!haveBox) { mn = Vector3(-100, -100, -100); mx = Vector3(100, 100, 100); }
            // Translate the local box to the block position.
            Vector3 wmn(b.Position.x + mn.x, b.Position.y + mn.y, b.Position.z + mn.z);
            Vector3 wmx(b.Position.x + mx.x, b.Position.y + mx.y, b.Position.z + mx.z);
            DrawAabb(m_device, wmn, wmx, (i == selectedBlockIdx) ? selCol : blockCol);
        }
    }

    // Per-block objects (Flag: 2=ColObj, 4=WaterObj).
    if (showObjects) {
        for (const auto& b : d.Blocks) {
            for (const auto& o : b.Objects) {
                Vector3 p(b.Position.x + o.Position.x, b.Position.y + o.Position.y, b.Position.z + o.Position.z);
                float s = (o.Scale.x + o.Scale.y + o.Scale.z) * 0.5f * 0.5f;
                if (s <= 0) s = 20.0f;
                DWORD c = (o.Flag & 4) ? waterCol : ((o.Flag & 2) ? colObjCol : objCol);
                DrawAabb(m_device, Vector3(p.x - s, p.y - s, p.z - s), Vector3(p.x + s, p.y + s, p.z + s), c);
            }
        }
    }

    // Links between block centers (off-mesh connectivity).
    if (showLinks && !d.Blocks.empty()) {
        std::vector<Vertex> lines;
        auto center = [&](uint32_t i) -> Vector3 {
            return (i < d.Blocks.size()) ? d.Blocks[i].Position : Vector3{};
        };
        for (const auto& link : d.Links) {
            if (link.BlockIndices.size() < 2) continue;
            Vector3 a = center(link.BlockIndices.front());
            for (size_t i = 1; i < link.BlockIndices.size(); ++i) {
                Vector3 b = center(link.BlockIndices[i]);
                lines.push_back({a, linkCol}); lines.push_back({b, linkCol});
                a = b;
            }
        }
        if (!lines.empty()) {
            m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
            m_device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)lines.size() / 2, lines.data(), sizeof(Vertex));
            m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
        }
    }

    // 3D voxel lookup grid (200u cells). Draw as small wireframe cubes.
    if (showVoxels) {
        for (const auto& vx : d.Voxels) {
            Vector3 p(vx.X() * 200.0f, vx.Y() * 200.0f, vx.Z() * 200.0f);
            float h = 100.0f;
            DrawAabb(m_device, Vector3(p.x - h, p.y - h, p.z - h), Vector3(p.x + h, p.y + h, p.z + h), voxelCol);
        }
    }

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}
