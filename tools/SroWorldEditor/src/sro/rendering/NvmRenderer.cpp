#include "NvmRenderer.h"
#include "sro/nav/ObjectNavSpatial.h"
#include "sro/nav/NavEdgeSemantics.h"
#include "sro/nav/TileCoord.h"
#include "Core/Log.h"
#include <algorithm>
#include <cstring>
#include <cmath>

// ============================================================================
// NvmRenderer -- visualizes the game's real navmesh data exactly as authored.
//   * Terrain: 96x96 tile grid (walkable/blocked/OOB) + CellQuad outlines (the
//     pathfinding nodes) + Global/Internal edges (the nav graph). All from .nvm.
//   * Per-object: each placement's BMS NavMeshOffset (RTNavMeshCellTri + outline/
//     inline edges), transformed by placement position + yaw. This is the real
//     object collision/nav geometry the game uses.
//   * LinkEdges: the inter-object AI link graph within a RTNavMeshTerrain.
// No app-invented AABBs, no projection. Reads straight from disk.
// ============================================================================

NvmRenderer::~NvmRenderer() { ClearBuffers(); }

void NvmRenderer::ClearBuffers() {
  for (auto &batch : m_batches) {
    if (batch.VbWalkable) batch.VbWalkable->Release();
    if (batch.VbBlocked) batch.VbBlocked->Release();
    if (batch.VbInternalEdges) batch.VbInternalEdges->Release();
    if (batch.VbGlobalEdges) batch.VbGlobalEdges->Release();
    if (batch.VbCells) batch.VbCells->Release();
  }
  m_batches.clear();
}

void NvmRenderer::LoadNavmesh(const sro::formats::NavMesh &nav, int rx, int ry) {
  std::vector<Vertex> gridLineVerts;
  std::vector<Vertex> blockedVerts;

  const DWORD gridColor = D3DCOLOR_ARGB(90, 220, 220, 220);
  const DWORD blockedColor = D3DCOLOR_ARGB(160, 230, 40, 40);
  const DWORD oobColor = D3DCOLOR_ARGB(120, 50, 50, 50);

  for (int tz = 0; tz < 96; ++tz) {
    for (int tx = 0; tx < 96; ++tx) {
      const sro::formats::NavTile &tile = nav.TileMap[tz * 96 + tx];
      bool isOob = (tile.Flags == 0xFFFF);
      bool isBlocked = !isOob && (tile.Flags & 0x0001);

      float x0 = tx * 20.0f, z0 = tz * 20.0f;
      float x1 = x0 + 20.0f, z1 = z0 + 20.0f;
      float h00 = GetHeight(x0, z0, nav.HeightMap) + 0.05f;
      float h10 = GetHeight(x1, z0, nav.HeightMap) + 0.05f;
      float h01 = GetHeight(x0, z1, nav.HeightMap) + 0.05f;
      float h11 = GetHeight(x1, z1, nav.HeightMap) + 0.05f;

      if (isOob || isBlocked) {
        DWORD color = isOob ? oobColor : blockedColor;
        blockedVerts.push_back({Vector3(x0, h00, z0), color}); blockedVerts.push_back({Vector3(x0, h01, z1), color}); blockedVerts.push_back({Vector3(x1, h10, z0), color});
        blockedVerts.push_back({Vector3(x1, h10, z0), color}); blockedVerts.push_back({Vector3(x0, h01, z1), color}); blockedVerts.push_back({Vector3(x1, h11, z1), color});
      } else {
        gridLineVerts.push_back({Vector3(x0, h00, z0), gridColor}); gridLineVerts.push_back({Vector3(x1, h10, z0), gridColor});
        gridLineVerts.push_back({Vector3(x1, h10, z0), gridColor}); gridLineVerts.push_back({Vector3(x1, h11, z1), gridColor});
        gridLineVerts.push_back({Vector3(x1, h11, z1), gridColor}); gridLineVerts.push_back({Vector3(x0, h01, z1), gridColor});
        gridLineVerts.push_back({Vector3(x0, h01, z1), gridColor}); gridLineVerts.push_back({Vector3(x0, h00, z0), gridColor});
      }
    }
  }

  std::vector<Vertex> internalEdgeVerts;
  std::vector<Vertex> globalEdgeVerts;
  auto appendEdge = [&](float minX, float minY, float maxX, float maxY, DWORD color, std::vector<Vertex>& out) {
    float dx = maxX - minX, dy = maxY - minY;
    float len = sqrtf(dx * dx + dy * dy);
    if (len <= 0.0f) return;
    int numSteps = static_cast<int>(ceilf(len / 10.0f));
    float prevX = minX, prevY = minY;
    float prevH = GetHeight(prevX, prevY, nav.HeightMap) + 0.2f;
    for (int s = 1; s <= numSteps; ++s) {
      float t = static_cast<float>(s) / numSteps;
      float nextX = minX + dx * t, nextY = minY + dy * t;
      float nextH = GetHeight(nextX, nextY, nav.HeightMap) + 0.2f;
      out.push_back({Vector3(prevX, prevH, prevY), color});
      out.push_back({Vector3(nextX, nextH, nextY), color});
      prevX = nextX; prevY = nextY; prevH = nextH;
    }
  };
  for (const auto &e : nav.GlobalEdges) {
    const auto pass = sro::nav::ClassifyTerrainEdge(e.Flags, e.D0, e.D1, e.C0, e.C1);
    appendEdge(e.MinX, e.MinY, e.MaxX, e.MaxY, sro::nav::ColorForTerrainEdge(pass, true), globalEdgeVerts);
  }
  for (const auto &e : nav.InternalEdges) {
    const auto pass = sro::nav::ClassifyTerrainEdge(e.Flags, e.D0, e.D1, e.C0, e.C1);
    appendEdge(e.MinX, e.MinY, e.MaxX, e.MaxY, sro::nav::ColorForTerrainEdge(pass, false), internalEdgeVerts);
  }

  // CellQuad outlines -- the actual pathfinding nodes (merged rects from tiles).
  std::vector<Vertex> cellVerts;
  DWORD cellColor = D3DCOLOR_ARGB(150, 255, 255, 255);
  for (const auto &cell : nav.Cells) {
    float h00 = GetHeight(cell.MinX, cell.MinY, nav.HeightMap) + 0.12f;
    float h10 = GetHeight(cell.MaxX, cell.MinY, nav.HeightMap) + 0.12f;
    float h11 = GetHeight(cell.MaxX, cell.MaxY, nav.HeightMap) + 0.12f;
    float h01 = GetHeight(cell.MinX, cell.MaxY, nav.HeightMap) + 0.12f;
    Vertex quad[8] = {
      {Vector3(cell.MinX, h00, cell.MinY), cellColor}, {Vector3(cell.MaxX, h10, cell.MinY), cellColor},
      {Vector3(cell.MaxX, h10, cell.MinY), cellColor}, {Vector3(cell.MaxX, h11, cell.MaxY), cellColor},
      {Vector3(cell.MaxX, h11, cell.MaxY), cellColor}, {Vector3(cell.MinX, h01, cell.MaxY), cellColor},
      {Vector3(cell.MinX, h01, cell.MaxY), cellColor}, {Vector3(cell.MinX, h00, cell.MinY), cellColor},
    };
    cellVerts.insert(cellVerts.end(), quad, quad + 8);
  }

  auto makeVb = [&](const std::vector<Vertex> &verts) -> LPDIRECT3DVERTEXBUFFER9 {
    if (verts.empty()) return nullptr;
    LPDIRECT3DVERTEXBUFFER9 vb = nullptr;
    m_device->CreateVertexBuffer(static_cast<UINT>(verts.size() * sizeof(Vertex)),
                                 D3DUSAGE_WRITEONLY, Vertex::FVF, D3DPOOL_MANAGED, &vb, nullptr);
    if (vb) {
      void *p = nullptr; vb->Lock(0, 0, &p, 0);
      std::memcpy(p, verts.data(), verts.size() * sizeof(Vertex));
      vb->Unlock();
    }
    return vb;
  };

  NvmBatch batch;
  batch.Rx = rx; batch.Ry = ry;
  batch.VbWalkable = makeVb(gridLineVerts); batch.WalkableCount = (int)gridLineVerts.size();
  batch.VbBlocked  = makeVb(blockedVerts);  batch.BlockedCount  = (int)blockedVerts.size();
  batch.VbInternalEdges = makeVb(internalEdgeVerts); batch.InternalEdgeCount = (int)internalEdgeVerts.size();
  batch.VbGlobalEdges = makeVb(globalEdgeVerts); batch.GlobalEdgeCount = (int)globalEdgeVerts.size();
  batch.VbCells    = makeVb(cellVerts);     batch.CellCount     = (int)cellVerts.size();
  m_batches.push_back(batch);

  LogMsg("[Navmesh] Loaded region (" + std::to_string(rx) + "," + std::to_string(ry) +
         ") cells=" + std::to_string(nav.Cells.size()) + " gEdges=" + std::to_string(nav.GlobalEdges.size()) +
         " iEdges=" + std::to_string(nav.InternalEdges.size()) + " objects=" + std::to_string(nav.Objects.size()));
}

void NvmRenderer::UnloadNavmesh(int rx, int ry) {
  auto it = std::remove_if(m_batches.begin(), m_batches.end(), [&](const NvmBatch &b) {
    if (b.Rx == rx && b.Ry == ry) {
      if (b.VbWalkable) b.VbWalkable->Release();
      if (b.VbBlocked) b.VbBlocked->Release();
      if (b.VbInternalEdges) b.VbInternalEdges->Release();
      if (b.VbGlobalEdges) b.VbGlobalEdges->Release();
      if (b.VbCells) b.VbCells->Release();
      return true;
    }
    return false;
  });
  if (it != m_batches.end()) m_batches.erase(it, m_batches.end());
}

void NvmRenderer::RebuildNavmeshBuffers(const sro::formats::NavMesh *nav, int rx, int ry) {
  if (!nav) return;
  UnloadNavmesh(rx, ry);
  LoadNavmesh(*nav, rx, ry);
}

void NvmRenderer::Draw(const Matrix4 &view, const Matrix4 &proj,
                       int centerRx, int centerRy, int radius,
                       bool showWalkable, bool showBlocked, bool showInternalEdges, bool showGlobalEdges,
                       bool showCells, const CameraFrustum &frustum) {
  if (m_batches.empty()) return;

  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX *>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX *>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetFVF(Vertex::FVF);
  m_device->SetTexture(0, nullptr);

  float bias = -0.00001f, slopeBias = -0.5f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&bias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD *)&slopeBias);

  for (const auto &b : m_batches) {
    if (std::abs(b.Rx - centerRx) > radius || std::abs(b.Ry - centerRy) > radius) continue;
    float xOff = (b.Ry - centerRy) * 1920.0f, zOff = (b.Rx - centerRx) * 1920.0f;
    if (!frustum.IsBoxVisible(Vector3(xOff, -1000.0f, zOff), Vector3(xOff + 1920.0f, 3000.0f, zOff + 1920.0f))) continue;

    Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
    m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX *>(&world));

    if (showWalkable && b.VbWalkable && b.WalkableCount > 0) {
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      m_device->SetStreamSource(0, b.VbWalkable, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_LINELIST, 0, b.WalkableCount / 2);
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    }
    if (showBlocked && b.VbBlocked && b.BlockedCount > 0) {
      m_device->SetStreamSource(0, b.VbBlocked, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, b.BlockedCount / 3);
    }
    if (showCells && b.VbCells && b.CellCount > 0) {
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      m_device->SetStreamSource(0, b.VbCells, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_LINELIST, 0, b.CellCount / 2);
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    }
    if (showInternalEdges && b.VbInternalEdges && b.InternalEdgeCount > 0) {
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      float eb = -0.00003f; m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&eb);
      m_device->SetStreamSource(0, b.VbInternalEdges, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_LINELIST, 0, b.InternalEdgeCount / 2);
      m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&bias);
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    }
    if (showGlobalEdges && b.VbGlobalEdges && b.GlobalEdgeCount > 0) {
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      float eb = -0.00003f; m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&eb);
      m_device->SetStreamSource(0, b.VbGlobalEdges, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_LINELIST, 0, b.GlobalEdgeCount / 2);
      m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&bias);
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    }
  }

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  float z = 0.0f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&z);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD *)&z);
}

float NvmRenderer::GetHeight(float localX, float localZ, const std::vector<float> &hm) const {
  if (hm.empty()) return 0.0f;
  float fx = localX / 20.0f, fz = localZ / 20.0f;
  int x0 = std::clamp(static_cast<int>(floorf(fx)), 0, 96);
  int z0 = std::clamp(static_cast<int>(floorf(fz)), 0, 96);
  int x1 = std::clamp(x0 + 1, 0, 96);
  int z1 = std::clamp(z0 + 1, 0, 96);
  float tx = std::clamp(fx - floorf(fx), 0.0f, 1.0f);
  float tz = std::clamp(fz - floorf(fz), 0.0f, 1.0f);
  auto at = [&](int x, int z) { return (z * 97 + x < (int)hm.size()) ? hm[z * 97 + x] : 0.0f; };
  float h0 = at(x0, z0) * (1.0f - tx) + at(x1, z0) * tx;
  float h1 = at(x0, z1) * (1.0f - tx) + at(x1, z1) * tx;
  return h0 * (1.0f - tz) + h1 * tz;
}

static void DrawWireframeBox(LPDIRECT3DDEVICE9 device, const Vector3& c, float s, DWORD color) {
  float h = s * 0.5f;
  NvmRenderer::Vertex v[24] = {
    {Vector3(c.x - h, c.y + h, c.z - h), color}, {Vector3(c.x + h, c.y + h, c.z - h), color},
    {Vector3(c.x + h, c.y + h, c.z - h), color}, {Vector3(c.x + h, c.y + h, c.z + h), color},
    {Vector3(c.x + h, c.y + h, c.z + h), color}, {Vector3(c.x - h, c.y + h, c.z + h), color},
    {Vector3(c.x - h, c.y + h, c.z + h), color}, {Vector3(c.x - h, c.y + h, c.z - h), color},
    {Vector3(c.x - h, c.y - h, c.z - h), color}, {Vector3(c.x + h, c.y - h, c.z - h), color},
    {Vector3(c.x + h, c.y - h, c.z - h), color}, {Vector3(c.x + h, c.y - h, c.z + h), color},
    {Vector3(c.x + h, c.y - h, c.z + h), color}, {Vector3(c.x - h, c.y - h, c.z + h), color},
    {Vector3(c.x - h, c.y - h, c.z + h), color}, {Vector3(c.x - h, c.y - h, c.z - h), color},
    {Vector3(c.x - h, c.y - h, c.z - h), color}, {Vector3(c.x - h, c.y + h, c.z - h), color},
    {Vector3(c.x + h, c.y - h, c.z - h), color}, {Vector3(c.x + h, c.y + h, c.z - h), color},
    {Vector3(c.x + h, c.y - h, c.z + h), color}, {Vector3(c.x + h, c.y + h, c.z + h), color},
    {Vector3(c.x - h, c.y - h, c.z + h), color}, {Vector3(c.x - h, c.y + h, c.z + h), color},
  };
  device->DrawPrimitiveUP(D3DPT_LINELIST, 12, v, sizeof(NvmRenderer::Vertex));
}

void NvmRenderer::DrawCellsAndSelection(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                         const sro::formats::NavMesh& nav, int rx, int ry,
                                         int selectedCellIdx, int selectedEdgeIdx, bool selectedEdgeIsGlobal) {
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetFVF(Vertex::FVF);

  float bias = -0.00002f, slopeBias = -0.8f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slopeBias);

  float xOff = (ry - centerRy) * 1920.0f, zOff = (rx - centerRx) * 1920.0f;
  Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  DWORD cellColor = D3DCOLOR_ARGB(120, 255, 255, 255);
  for (int i = 0; i < (int)nav.Cells.size(); ++i) {
    const auto& c = nav.Cells[i];
    float h00 = GetHeight(c.MinX, c.MinY, nav.HeightMap) + 0.1f;
    float h10 = GetHeight(c.MaxX, c.MinY, nav.HeightMap) + 0.1f;
    float h11 = GetHeight(c.MaxX, c.MaxY, nav.HeightMap) + 0.1f;
    float h01 = GetHeight(c.MinX, c.MaxY, nav.HeightMap) + 0.1f;
    Vertex lv[8] = {
      {Vector3(c.MinX, h00, c.MinY), cellColor}, {Vector3(c.MaxX, h10, c.MinY), cellColor},
      {Vector3(c.MaxX, h10, c.MinY), cellColor}, {Vector3(c.MaxX, h11, c.MaxY), cellColor},
      {Vector3(c.MaxX, h11, c.MaxY), cellColor}, {Vector3(c.MinX, h01, c.MaxY), cellColor},
      {Vector3(c.MinX, h01, c.MaxY), cellColor}, {Vector3(c.MinX, h00, c.MinY), cellColor},
    };
    m_device->DrawPrimitiveUP(D3DPT_LINELIST, 4, lv, sizeof(Vertex));
    if (i == selectedCellIdx) {
      DWORD fc = D3DCOLOR_ARGB(70, 255, 255, 0);
      Vertex fv[6] = {
        {Vector3(c.MinX, h00, c.MinY), fc}, {Vector3(c.MinX, h01, c.MaxY), fc}, {Vector3(c.MaxX, h10, c.MinY), fc},
        {Vector3(c.MaxX, h10, c.MinY), fc}, {Vector3(c.MinX, h01, c.MaxY), fc}, {Vector3(c.MaxX, h11, c.MaxY), fc},
      };
      m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, fv, sizeof(Vertex));
      DWORD hc = D3DCOLOR_ARGB(255, 230, 20, 20);
      DrawWireframeBox(m_device, Vector3(c.MinX, h00, c.MinY), 1.5f, hc);
      DrawWireframeBox(m_device, Vector3(c.MaxX, h10, c.MinY), 1.5f, hc);
      DrawWireframeBox(m_device, Vector3(c.MaxX, h11, c.MaxY), 1.5f, hc);
      DrawWireframeBox(m_device, Vector3(c.MinX, h01, c.MaxY), 1.5f, hc);
    }
  }

  if (selectedEdgeIdx >= 0) {
    float eMinX = 0, eMinY = 0, eMaxX = 0, eMaxY = 0;
    if (selectedEdgeIsGlobal) {
      if (selectedEdgeIdx < (int)nav.GlobalEdges.size()) { const auto& e = nav.GlobalEdges[selectedEdgeIdx]; eMinX = e.MinX; eMinY = e.MinY; eMaxX = e.MaxX; eMaxY = e.MaxY; }
    } else {
      if (selectedEdgeIdx < (int)nav.InternalEdges.size()) { const auto& e = nav.InternalEdges[selectedEdgeIdx]; eMinX = e.MinX; eMinY = e.MinY; eMaxX = e.MaxX; eMaxY = e.MaxY; }
    }
    if (eMinX != eMaxX || eMinY != eMaxY) {
      float h0 = GetHeight(eMinX, eMinY, nav.HeightMap) + 0.15f;
      float h1 = GetHeight(eMaxX, eMaxY, nav.HeightMap) + 0.15f;
      DWORD lc = D3DCOLOR_ARGB(255, 255, 0, 0);
      Vertex ev[2] = {{Vector3(eMinX, h0, eMinY), lc}, {Vector3(eMaxX, h1, eMaxY), lc}};
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      m_device->DrawPrimitiveUP(D3DPT_LINELIST, 1, ev, sizeof(Vertex));
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
      DWORD hc = D3DCOLOR_ARGB(255, 255, 140, 0);
      DrawWireframeBox(m_device, Vector3(eMinX, h0, eMinY), 1.5f, hc);
      DrawWireframeBox(m_device, Vector3(eMaxX, h1, eMaxY), 1.5f, hc);
    }
  }

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  float z = 0.0f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&z);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&z);
}

void NvmRenderer::DrawNavmeshBlockers(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                       const sro::formats::NavMesh& nav, int rx, int ry) {
  if (nav.TileMap.size() != 96 * 96) return;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetFVF(Vertex::FVF);
  float bias = 0.05f, slopeBias = -0.5f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slopeBias);

  float xOff = (ry - centerRy) * 1920.0f, zOff = (rx - centerRx) * 1920.0f;
  Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  const DWORD blockedColor = D3DCOLOR_ARGB(160, 230, 40, 40);
  std::vector<Vertex> bv;
  bv.reserve(96 * 96 * 6);
  for (int tz = 0; tz < 96; ++tz) {
    for (int tx = 0; tx < 96; ++tx) {
      const auto &tile = nav.TileMap[tz * 96 + tx];
      if (!((tile.Flags == 0xFFFF) || (tile.Flags & 0x0001))) continue;
      float x0 = tx * 20.0f, z0 = tz * 20.0f, x1 = x0 + 20.0f, z1 = z0 + 20.0f;
      float h00 = GetHeight(x0, z0, nav.HeightMap) + bias, h10 = GetHeight(x1, z0, nav.HeightMap) + bias;
      float h01 = GetHeight(x0, z1, nav.HeightMap) + bias, h11 = GetHeight(x1, z1, nav.HeightMap) + bias;
      bv.push_back({Vector3(x0, h00, z0), blockedColor}); bv.push_back({Vector3(x0, h01, z1), blockedColor}); bv.push_back({Vector3(x1, h10, z0), blockedColor});
      bv.push_back({Vector3(x1, h10, z0), blockedColor}); bv.push_back({Vector3(x0, h01, z1), blockedColor}); bv.push_back({Vector3(x1, h11, z1), blockedColor});
    }
  }
  if (!bv.empty()) m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)bv.size() / 3, bv.data(), sizeof(Vertex));

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  float z = 0.0f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&z);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&z);
}

// Transform an object-local NavVertex into world space (placement position + yaw + region offset).
static inline Vector3 TransformNavVertex(const Vector3& v, const PlacementVM& p, int centerRx, int centerRy) {
  return sro::nav::TransformPlacementNavVertex(v, p, centerRx, centerRy);
}

static void AppendEdgeQuad(std::vector<NvmRenderer::Vertex>& tris,
                           const Vector3& a, const Vector3& b, float halfWidth, DWORD col) {
  const float dx = b.x - a.x;
  const float dz = b.z - a.z;
  const float len = sqrtf(dx * dx + dz * dz);
  if (len < 0.001f) return;
  const float px = -dz / len * halfWidth;
  const float pz = dx / len * halfWidth;
  const float y = (a.y + b.y) * 0.5f + 0.08f;
  const Vector3 a0(a.x + px, y, a.z + pz);
  const Vector3 a1(a.x - px, y, a.z - pz);
  const Vector3 b0(b.x + px, y, b.z + pz);
  const Vector3 b1(b.x - px, y, b.z - pz);
  tris.push_back({a0, col}); tris.push_back({a1, col}); tris.push_back({b0, col});
  tris.push_back({a1, col}); tris.push_back({b1, col}); tris.push_back({b0, col});
}

void NvmRenderer::DrawObjectNavmeshes(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                       const std::vector<PlacementVM>& placements,
                                       const BmsDrawLayers& layers, int16_t selectedUid) {
  const bool anyLayer = layers.wikiCollision || layers.passabilityClass || layers.showBuilding
      || layers.showPlatform || layers.edgeDebug;
  if (!anyLayer) return;

  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetFVF(Vertex::FVF);
  float bias = 0.2f, slopeBias = -0.8f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slopeBias);
  Matrix4 world = Matrix4::Identity();
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  for (const auto& p : placements) {
    if (p.LoadedRx != centerRx || p.LoadedRy != centerRy) continue;
    const BmsNavMesh* nav = p.Collision.NavMesh;
    if (!nav || nav->Cells.empty()) continue;

    const bool isSelected = (selectedUid >= 0 && p.Object.UID == selectedUid);
    const DWORD colWikiFill = isSelected ? D3DCOLOR_ARGB(200, 200, 140, 120) : D3DCOLOR_ARGB(180, 180, 100, 120);
    const DWORD colBuilding = isSelected ? D3DCOLOR_ARGB(200, 255, 200, 80) : D3DCOLOR_ARGB(160, 230, 120, 30);
    const DWORD colPlatform = isSelected ? D3DCOLOR_ARGB(180, 255, 255, 0) : D3DCOLOR_ARGB(140, 60, 200, 60);
    const float edgeHalfWidth = isSelected ? 4.0f : 2.5f;

    auto xform = [&](uint16_t vi) {
      const Vector3 local = (vi < nav->Vertices.size()) ? nav->Vertices[vi].Position : Vector3{};
      return TransformNavVertex(local, p, centerRx, centerRy);
    };

    if (layers.wikiCollision) {
      std::vector<Vertex> wikiTris;
      wikiTris.reserve(nav->Cells.size() * 3);
      for (const auto& cell : nav->Cells) {
        Vector3 a = xform(cell.V0), b = xform(cell.V1), c = xform(cell.V2);
        wikiTris.push_back({a, colWikiFill}); wikiTris.push_back({b, colWikiFill}); wikiTris.push_back({c, colWikiFill});
      }
      if (!wikiTris.empty())
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)wikiTris.size() / 3, wikiTris.data(), sizeof(Vertex));

      std::vector<Vertex> inlineLines;
      inlineLines.reserve(nav->InlineEdges.size() * 2);
      const DWORD white = D3DCOLOR_ARGB(220, 255, 255, 255);
      for (const auto& e : nav->InlineEdges) {
        inlineLines.push_back({xform(e.SrcVertex), white});
        inlineLines.push_back({xform(e.DstVertex), white});
      }
      if (!inlineLines.empty()) {
        m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
        m_device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)inlineLines.size() / 2, inlineLines.data(), sizeof(Vertex));
        m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
      }

      std::vector<Vertex> blockedOutline;
      blockedOutline.reserve(nav->OutlineEdges.size() * 6);
      for (const auto& e : nav->OutlineEdges) {
        if (!sro::nav::EdgeFlagBlocksCrossing(e.Flag)) continue;
        AppendEdgeQuad(blockedOutline, xform(e.SrcVertex), xform(e.DstVertex), edgeHalfWidth,
                       D3DCOLOR_ARGB(255, 255, 40, 40));
      }
      if (!blockedOutline.empty())
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)blockedOutline.size() / 3, blockedOutline.data(), sizeof(Vertex));
    }

    if (layers.passabilityClass || layers.showBuilding || layers.showPlatform) {
      std::vector<Vertex> kindTris;
      kindTris.reserve(nav->Cells.size() * 3);
      for (int ci = 0; ci < static_cast<int>(nav->Cells.size()); ++ci) {
        const auto kind = sro::nav::ClassifyBmsCell(*nav, ci);
        if (kind == sro::nav::BmsCellNavKind::Platform && !(layers.passabilityClass || layers.showPlatform)) continue;
        if (kind == sro::nav::BmsCellNavKind::Building && !(layers.passabilityClass || layers.showBuilding)) continue;
        const auto& cell = nav->Cells[static_cast<size_t>(ci)];
        Vector3 a = xform(cell.V0), b = xform(cell.V1), c = xform(cell.V2);
        const DWORD col = (kind == sro::nav::BmsCellNavKind::Platform) ? colPlatform : colBuilding;
        kindTris.push_back({a, col}); kindTris.push_back({b, col}); kindTris.push_back({c, col});
      }
      if (!kindTris.empty())
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)kindTris.size() / 3, kindTris.data(), sizeof(Vertex));
    }

    if (layers.edgeDebug) {
      std::vector<Vertex> edgeTris;
      edgeTris.reserve((nav->OutlineEdges.size() + nav->InlineEdges.size()) * 6);
      for (const auto& e : nav->OutlineEdges) {
        const bool tf = sro::nav::IsBmsTerrainFacingEdge(e);
        const bool open = sro::nav::IsBmsEdgeOpenToTerrain(e);
        const DWORD col = sro::nav::ColorForBmsOutlineEdge(e.Flag, tf, open);
        AppendEdgeQuad(edgeTris, xform(e.SrcVertex), xform(e.DstVertex), edgeHalfWidth * 0.8f, col);
      }
      for (const auto& e : nav->InlineEdges) {
        if (sro::nav::EdgeFlagBlocksCrossing(e.Flag))
          AppendEdgeQuad(edgeTris, xform(e.SrcVertex), xform(e.DstVertex), edgeHalfWidth * 0.5f,
                         D3DCOLOR_ARGB(200, 255, 80, 80));
      }
      if (!edgeTris.empty())
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)edgeTris.size() / 3, edgeTris.data(), sizeof(Vertex));
    }
  }

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  float z = 0.0f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&z);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&z);
}

void NvmRenderer::DrawLinkEdges(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                 const sro::formats::NavMesh& nav, int rx, int ry,
                                 const std::vector<PlacementVM>& placements) {
  if (nav.Objects.empty()) return;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetFVF(Vertex::FVF);
  float bias = 0.15f, slopeBias = -0.5f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slopeBias);
  Matrix4 world = Matrix4::Identity();
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  auto findPlacement = [&](int16_t localUid) -> const PlacementVM* {
    for (const auto& p : placements) {
      if (p.LoadedRx == rx && p.LoadedRy == ry && p.Object.UID == localUid)
        return &p;
    }
    return nullptr;
  };

  auto edgeSegment = [&](const PlacementVM* p, const BmsNavMesh* bms, int edgeId,
                         Vector3& outA, Vector3& outB) -> bool {
    if (!p || !bms || edgeId < 0 || edgeId >= static_cast<int>(bms->OutlineEdges.size())) return false;
    const auto& e = bms->OutlineEdges[static_cast<size_t>(edgeId)];
    outA = TransformNavVertex(
        (e.SrcVertex < bms->Vertices.size()) ? bms->Vertices[e.SrcVertex].Position : Vector3{}, *p, centerRx, centerRy);
    outB = TransformNavVertex(
        (e.DstVertex < bms->Vertices.size()) ? bms->Vertices[e.DstVertex].Position : Vector3{}, *p, centerRx, centerRy);
    return true;
  };

  const DWORD linkCol = D3DCOLOR_ARGB(220, 255, 0, 255);
  std::vector<Vertex> lines;
  for (int i = 0; i < static_cast<int>(nav.Objects.size()); ++i) {
    const auto& o = nav.Objects[static_cast<size_t>(i)];
    const PlacementVM* srcP = findPlacement(o.LocalUID);
    const BmsNavMesh* srcBms = srcP ? srcP->Collision.NavMesh : nullptr;
    for (const auto& le : o.Edges) {
      if (le.LinkedObjID < 0 || le.LinkedObjID >= static_cast<int>(nav.Objects.size())) continue;
      const auto& linked = nav.Objects[static_cast<size_t>(le.LinkedObjID)];
      const PlacementVM* dstP = findPlacement(linked.LocalUID);
      const BmsNavMesh* dstBms = dstP ? dstP->Collision.NavMesh : nullptr;
      Vector3 a{}, b{};
      if (edgeSegment(srcP, srcBms, le.EdgeID, a, b)) {
        lines.push_back({a, linkCol});
        Vector3 la{}, lb{};
        if (edgeSegment(dstP, dstBms, le.LinkedObjEdgeID, la, lb)) {
          lines.push_back({lb, linkCol});
        } else {
          int oRx = (linked.RegionID >> 8) & 0xFF, oRy = linked.RegionID & 0xFF;
          float oX = sro::nav::RegionOffsetX(oRy, centerRy);
          float oZ = sro::nav::RegionOffsetZ(oRx, centerRx);
          lines.push_back({Vector3(linked.PosX + oX, linked.PosY, linked.PosZ + oZ), linkCol});
        }
      }
    }
  }
  if (!lines.empty()) {
    m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
    m_device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)lines.size() / 2, lines.data(), sizeof(Vertex));
    m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
  }

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  float z = 0.0f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&z);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&z);
}

static DWORD TileOverlayColor(const sro::formats::NavTile& tile, int colorMode) {
  if (tile.Flags == 0xFFFF) return D3DCOLOR_ARGB(120, 50, 50, 50);
  if (colorMode == 1) return D3DCOLOR_ARGB(140, 60, 200, 60);
  if (colorMode == 2) return D3DCOLOR_ARGB(140, (tile.Flags & 1) ? 230 : 60, 60, 60);
  if (colorMode == 3) {
    const uint32_t h = static_cast<uint32_t>(tile.CellID) * 2654435761u;
    return D3DCOLOR_ARGB(140, (h & 0xFF), ((h >> 8) & 0xFF), ((h >> 16) & 0xFF));
  }
  if (colorMode == 5) {
    const uint32_t h = tile.TextureID * 2654435761u;
    return D3DCOLOR_ARGB(140, (h & 0xFF), ((h >> 8) & 0xFF), ((h >> 16) & 0xFF));
  }
  return (tile.Flags & 0x0001) ? D3DCOLOR_ARGB(160, 230, 40, 40) : D3DCOLOR_ARGB(80, 60, 200, 60);
}

void NvmRenderer::DrawTileMapOverlay(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                     const sro::formats::NavMesh& nav, int rx, int ry, int colorMode,
                                     int selectedTileIdx) {
  if (nav.TileMap.size() != 96u * 96u) return;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetFVF(Vertex::FVF);
  float xOff = sro::nav::RegionOffsetX(ry, centerRy);
  float zOff = sro::nav::RegionOffsetZ(rx, centerRx);
  Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  std::vector<Vertex> tris;
  tris.reserve(96 * 96 * 6);
  for (int tz = 0; tz < 96; ++tz) {
    for (int tx = 0; tx < 96; ++tx) {
      const auto& tile = nav.TileMap[static_cast<size_t>(tz * 96 + tx)];
      float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
      sro::nav::TileBoundsLocal(tx, tz, x0, z0, x1, z1);
      float h00 = GetHeight(x0, z0, nav.HeightMap) + 0.08f;
      float h10 = GetHeight(x1, z0, nav.HeightMap) + 0.08f;
      float h01 = GetHeight(x0, z1, nav.HeightMap) + 0.08f;
      float h11 = GetHeight(x1, z1, nav.HeightMap) + 0.08f;
      DWORD col = TileOverlayColor(tile, colorMode);
      tris.push_back({Vector3(x0, h00, z0), col}); tris.push_back({Vector3(x0, h01, z1), col}); tris.push_back({Vector3(x1, h10, z0), col});
      tris.push_back({Vector3(x1, h10, z0), col}); tris.push_back({Vector3(x0, h01, z1), col}); tris.push_back({Vector3(x1, h11, z1), col});
    }
  }
  if (!tris.empty())
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)tris.size() / 3, tris.data(), sizeof(Vertex));

  if (selectedTileIdx >= 0 && selectedTileIdx < 96 * 96)
    DrawSelectedTile(view, proj, centerRx, centerRy, nav, rx, ry, selectedTileIdx);

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void NvmRenderer::DrawHeightMapSurface(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                         const sro::formats::NavMesh& nav, int rx, int ry,
                                         bool surface, bool wireframe) {
  if (nav.HeightMap.size() != 97u * 97u) return;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetFVF(Vertex::FVF);
  float xOff = sro::nav::RegionOffsetX(ry, centerRy);
  float zOff = sro::nav::RegionOffsetZ(rx, centerRx);
  Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  float minH = nav.HeightMap[0], maxH = nav.HeightMap[0];
  for (float h : nav.HeightMap) { minH = std::min(minH, h); maxH = std::max(maxH, h); }
  const float range = std::max(1.0f, maxH - minH);

  if (surface) {
    std::vector<Vertex> tris;
    for (int tz = 0; tz < 95; ++tz) {
      for (int tx = 0; tx < 95; ++tx) {
        auto hv = [&](int x, int z) { return nav.HeightMap[static_cast<size_t>(z * 97 + x)]; };
        float x0 = tx * 20.f, z0 = tz * 20.f, x1 = x0 + 20.f, z1 = z0 + 20.f;
        auto col = [&](float h) {
          float t = (h - minH) / range;
          return D3DCOLOR_ARGB(100, (BYTE)(40 + t * 80), (BYTE)(100 + t * 100), (BYTE)(60 + t * 40));
        };
        float h00 = hv(tx, tz), h10 = hv(tx + 1, tz), h01 = hv(tx, tz + 1), h11 = hv(tx + 1, tz + 1);
        tris.push_back({Vector3(x0, h00, z0), col(h00)}); tris.push_back({Vector3(x0, h01, z1), col(h01)}); tris.push_back({Vector3(x1, h10, z0), col(h10)});
        tris.push_back({Vector3(x1, h10, z0), col(h10)}); tris.push_back({Vector3(x0, h01, z1), col(h01)}); tris.push_back({Vector3(x1, h11, z1), col(h11)});
      }
    }
    if (!tris.empty())
      m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)tris.size() / 3, tris.data(), sizeof(Vertex));
  }
  if (wireframe) {
    std::vector<Vertex> lines;
    const DWORD gridCol = D3DCOLOR_ARGB(160, 200, 200, 100);
    for (int z = 0; z < 97; ++z) {
      for (int x = 0; x < 96; ++x) {
        float x0 = x * 20.f, z0 = z * 20.f, x1 = (x + 1) * 20.f;
        float h0 = nav.HeightMap[static_cast<size_t>(z * 97 + x)];
        float h1 = nav.HeightMap[static_cast<size_t>(z * 97 + x + 1)];
        lines.push_back({Vector3(x0, h0, z0), gridCol}); lines.push_back({Vector3(x1, h1, z0), gridCol});
      }
    }
    for (int x = 0; x < 97; ++x) {
      for (int z = 0; z < 96; ++z) {
        float x0 = x * 20.f, z0 = z * 20.f, z1 = (z + 1) * 20.f;
        float h0 = nav.HeightMap[static_cast<size_t>(z * 97 + x)];
        float h1 = nav.HeightMap[static_cast<size_t>((z + 1) * 97 + x)];
        lines.push_back({Vector3(x0, h0, z0), gridCol}); lines.push_back({Vector3(x0, h1, z1), gridCol});
      }
    }
    if (!lines.empty()) {
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      m_device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)lines.size() / 2, lines.data(), sizeof(Vertex));
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    }
  }
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void NvmRenderer::DrawPlaneMap(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                               const sro::formats::NavMesh& nav, int rx, int ry) {
  if (!nav.HasPlanes || nav.PlaneTypeMap.size() < 36 || nav.PlaneHeightMap.size() < 36) return;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetFVF(Vertex::FVF);
  float xOff = sro::nav::RegionOffsetX(ry, centerRy);
  float zOff = sro::nav::RegionOffsetZ(rx, centerRx);
  Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  std::vector<Vertex> tris;
  for (int pz = 0; pz < 6; ++pz) {
    for (int px = 0; px < 6; ++px) {
      const int pi = pz * 6 + px;
      const uint8_t ptype = nav.PlaneTypeMap[static_cast<size_t>(pi)];
      if (ptype == 0) continue;
      const float ph = nav.PlaneHeightMap[static_cast<size_t>(pi)];
      const float x0 = px * 320.f, z0 = pz * 320.f, x1 = x0 + 320.f, z1 = z0 + 320.f;
      DWORD col = (ptype == 2) ? D3DCOLOR_ARGB(120, 180, 240, 255) : D3DCOLOR_ARGB(120, 40, 100, 220);
      tris.push_back({Vector3(x0, ph, z0), col}); tris.push_back({Vector3(x0, ph, z1), col}); tris.push_back({Vector3(x1, ph, z0), col});
      tris.push_back({Vector3(x1, ph, z0), col}); tris.push_back({Vector3(x0, ph, z1), col}); tris.push_back({Vector3(x1, ph, z1), col});
    }
  }
  if (!tris.empty())
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)tris.size() / 3, tris.data(), sizeof(Vertex));
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void NvmRenderer::DrawSelectedTile(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                   const sro::formats::NavMesh& nav, int rx, int ry, int selectedTileIdx) {
  if (selectedTileIdx < 0 || selectedTileIdx >= 96 * 96 || nav.TileMap.size() != 96u * 96u) return;
  const int tx = selectedTileIdx % 96;
  const int tz = selectedTileIdx / 96;
  float x0 = tx * 20.0f, z0 = tz * 20.0f, x1 = x0 + 20.0f, z1 = z0 + 20.0f;
  float h00 = GetHeight(x0, z0, nav.HeightMap) + 0.15f;
  float h10 = GetHeight(x1, z0, nav.HeightMap) + 0.15f;
  float h01 = GetHeight(x0, z1, nav.HeightMap) + 0.15f;
  float h11 = GetHeight(x1, z1, nav.HeightMap) + 0.15f;

  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetFVF(Vertex::FVF);
  float xOff = (ry - centerRy) * 1920.0f, zOff = (rx - centerRx) * 1920.0f;
  Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  const DWORD col = D3DCOLOR_ARGB(255, 255, 255, 0);
  Vertex lv[8] = {
    {Vector3(x0, h00, z0), col}, {Vector3(x1, h10, z0), col},
    {Vector3(x1, h10, z0), col}, {Vector3(x1, h11, z1), col},
    {Vector3(x1, h11, z1), col}, {Vector3(x0, h01, z1), col},
    {Vector3(x0, h01, z1), col}, {Vector3(x0, h00, z0), col},
  };
  m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
  m_device->DrawPrimitiveUP(D3DPT_LINELIST, 4, lv, sizeof(Vertex));
  m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void NvmRenderer::DrawPathfindCells(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                    const sro::formats::NavMesh& nav, int rx, int ry, const std::vector<int>& cellPath) {
  if (cellPath.empty()) return;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetFVF(Vertex::FVF);
  float xOff = (ry - centerRy) * 1920.0f, zOff = (rx - centerRx) * 1920.0f;
  Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  const DWORD pathCol = D3DCOLOR_ARGB(220, 40, 255, 80);
  std::vector<Vertex> lines;
  Vector3 prev{};
  bool hasPrev = false;
  for (int ci : cellPath) {
    if (ci < 0 || ci >= (int)nav.Cells.size()) continue;
    const auto& c = nav.Cells[ci];
    float cx = (c.MinX + c.MaxX) * 0.5f;
    float cz = (c.MinY + c.MaxY) * 0.5f;
    float cy = GetHeight(cx, cz, nav.HeightMap) + 0.25f;
    Vector3 p(cx, cy, cz);
    if (hasPrev) { lines.push_back({prev, pathCol}); lines.push_back({p, pathCol}); }
    prev = p;
    hasPrev = true;
  }
  if (!lines.empty()) {
    m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
    m_device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)lines.size() / 2, lines.data(), sizeof(Vertex));
    m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
  }
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void NvmRenderer::DrawNeighborRegionFrames(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy, int radius) {
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetFVF(Vertex::FVF);
  const DWORD col = D3DCOLOR_ARGB(120, 180, 180, 255);
  const float s = 1920.0f;
  for (int drx = -radius; drx <= radius; ++drx) {
    for (int dry = -radius; dry <= radius; ++dry) {
      if (drx == 0 && dry == 0) continue;
      int rx = centerRx + drx, ry = centerRy + dry;
      float xOff = (ry - centerRy) * s, zOff = (rx - centerRx) * s;
      Matrix4 world = MatrixTranslation(xOff, 0.0f, zOff);
      m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));
      Vertex box[8] = {
        {Vector3(0, 2, 0), col}, {Vector3(s, 2, 0), col},
        {Vector3(s, 2, 0), col}, {Vector3(s, 2, s), col},
        {Vector3(s, 2, s), col}, {Vector3(0, 2, s), col},
        {Vector3(0, 2, s), col}, {Vector3(0, 2, 0), col},
      };
      m_device->DrawPrimitiveUP(D3DPT_LINELIST, 4, box, sizeof(Vertex));
    }
  }
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}
