#include "NvmRenderer.h"
#include "Core/Log.h"
#include <algorithm>
#include <cstring>

// ============================================================================
// NvmRenderer implementation
// ============================================================================

NvmRenderer::~NvmRenderer() { ClearBuffers(); }

void NvmRenderer::ClearBuffers() {
  for (auto &batch : m_batches) {
    if (batch.VbWalkable) batch.VbWalkable->Release();
    if (batch.VbBlocked) batch.VbBlocked->Release();
    if (batch.VbEdges) batch.VbEdges->Release();
  }
  m_batches.clear();
}

void NvmRenderer::LoadNavmesh(const sro::formats::NavMesh &nav, int rx, int ry) {
  float minH = 999999.0f;
  float maxH = -999999.0f;
  for (float h : nav.HeightMap) {
    if (h < minH) minH = h;
    if (h > maxH) maxH = h;
  }
  char buf[256];
  sprintf_s(buf, "[Navmesh] LoadNavmesh: region=(%d,%d) heightmap_size=%d minH=%.2f maxH=%.2f objects=%d cells=%d",
            rx, ry, (int)nav.HeightMap.size(), minH, maxH, (int)nav.Objects.size(), (int)nav.Cells.size());
  LogMsg(buf);

  std::vector<Vertex> walkableVerts;
  std::vector<Vertex> blockedVerts;

  for (int tz = 0; tz < 96; ++tz) {
    for (int tx = 0; tx < 96; ++tx) {
      const sro::formats::NavTile &tile = nav.TileMap[tz * 96 + tx];

      // Blocked if 0xFFFF (out of bounds) or if bit 0 is set (painted/explicitly blocked)
      bool isBlocked = (tile.Flags == 0xFFFF) || (tile.Flags & 0x0001);

      DWORD color;
      if (isBlocked) {
        color = D3DCOLOR_ARGB(120, 200, 30, 30); // Blocked
      } else {
        color = D3DCOLOR_ARGB(80, 30, 200, 30); // Walkable
      }

      float x0 = tx * 20.0f;
      float z0 = tz * 20.0f;
      float x1 = x0 + 20.0f;
      float z1 = z0 + 20.0f;

      float h00 = GetHeight(tx * 20.0f, tz * 20.0f, nav.HeightMap);
      float h10 = GetHeight((tx + 1) * 20.0f, tz * 20.0f, nav.HeightMap);
      float h01 = GetHeight(tx * 20.0f, (tz + 1) * 20.0f, nav.HeightMap);
      float h11 = GetHeight((tx + 1) * 20.0f, (tz + 1) * 20.0f, nav.HeightMap);

      float bias = 0.05f;

      auto &targetVerts = isBlocked ? blockedVerts : walkableVerts;
      targetVerts.push_back({Vector3(x0, h00 + bias, z0), color});
      targetVerts.push_back({Vector3(x0, h01 + bias, z1), color});
      targetVerts.push_back({Vector3(x1, h10 + bias, z0), color});

      targetVerts.push_back({Vector3(x1, h10 + bias, z0), color});
      targetVerts.push_back({Vector3(x0, h01 + bias, z1), color});
      targetVerts.push_back({Vector3(x1, h11 + bias, z1), color});
    }
  }

  // Load Edges (subdivided to hug the terrain contour and prevent depth clipping)
  std::vector<Vertex> edgeVerts;
  float bias = 0.2f;

  // 1. Global Edges (colored Yellow)
  for (const auto &edge : nav.GlobalEdges) {
    DWORD color = D3DCOLOR_ARGB(255, 255, 255, 0); // Yellow
    float dx = edge.MaxX - edge.MinX;
    float dy = edge.MaxY - edge.MinY;
    float len = sqrtf(dx * dx + dy * dy);
    
    if (len > 0.0f) {
      float stepSize = 10.0f;
      int numSteps = static_cast<int>(ceilf(len / stepSize));
      
      float prevX = edge.MinX;
      float prevY = edge.MinY;
      float prevH = GetHeight(prevX, prevY, nav.HeightMap);
      
      for (int s = 1; s <= numSteps; ++s) {
        float t = static_cast<float>(s) / numSteps;
        float nextX = edge.MinX + dx * t;
        float nextY = edge.MinY + dy * t;
        float nextH = GetHeight(nextX, nextY, nav.HeightMap);
        
        edgeVerts.push_back({Vector3(prevX, prevH + bias, prevY), color});
        edgeVerts.push_back({Vector3(nextX, nextH + bias, nextY), color});
        
        prevX = nextX;
        prevY = nextY;
        prevH = nextH;
      }
    }
  }

  // 2. Internal Edges (colored Cyan)
  for (const auto &edge : nav.InternalEdges) {
    DWORD color = D3DCOLOR_ARGB(255, 0, 255, 255); // Cyan
    float dx = edge.MaxX - edge.MinX;
    float dy = edge.MaxY - edge.MinY;
    float len = sqrtf(dx * dx + dy * dy);
    
    if (len > 0.0f) {
      float stepSize = 10.0f;
      int numSteps = static_cast<int>(ceilf(len / stepSize));
      
      float prevX = edge.MinX;
      float prevY = edge.MinY;
      float prevH = GetHeight(prevX, prevY, nav.HeightMap);
      
      for (int s = 1; s <= numSteps; ++s) {
        float t = static_cast<float>(s) / numSteps;
        float nextX = edge.MinX + dx * t;
        float nextY = edge.MinY + dy * t;
        float nextH = GetHeight(nextX, nextY, nav.HeightMap);
        
        edgeVerts.push_back({Vector3(prevX, prevH + bias, prevY), color});
        edgeVerts.push_back({Vector3(nextX, nextH + bias, nextY), color});
        
        prevX = nextX;
        prevY = nextY;
        prevH = nextH;
      }
    }
  }

  LPDIRECT3DVERTEXBUFFER9 vbWalkable = nullptr;
  if (!walkableVerts.empty()) {
    m_device->CreateVertexBuffer(
        static_cast<UINT>(walkableVerts.size() * sizeof(Vertex)), D3DUSAGE_WRITEONLY,
        Vertex::FVF, D3DPOOL_MANAGED, &vbWalkable, nullptr);
    if (vbWalkable) {
      void *pV = nullptr;
      vbWalkable->Lock(0, 0, &pV, 0);
      std::memcpy(pV, walkableVerts.data(), walkableVerts.size() * sizeof(Vertex));
      vbWalkable->Unlock();
    }
  }

  LPDIRECT3DVERTEXBUFFER9 vbBlocked = nullptr;
  if (!blockedVerts.empty()) {
    m_device->CreateVertexBuffer(
        static_cast<UINT>(blockedVerts.size() * sizeof(Vertex)), D3DUSAGE_WRITEONLY,
        Vertex::FVF, D3DPOOL_MANAGED, &vbBlocked, nullptr);
    if (vbBlocked) {
      void *pV = nullptr;
      vbBlocked->Lock(0, 0, &pV, 0);
      std::memcpy(pV, blockedVerts.data(), blockedVerts.size() * sizeof(Vertex));
      vbBlocked->Unlock();
    }
  }

  LPDIRECT3DVERTEXBUFFER9 vbEdges = nullptr;
  if (!edgeVerts.empty()) {
    m_device->CreateVertexBuffer(
        static_cast<UINT>(edgeVerts.size() * sizeof(Vertex)), D3DUSAGE_WRITEONLY,
        Vertex::FVF, D3DPOOL_MANAGED, &vbEdges, nullptr);
    if (vbEdges) {
      void *pV = nullptr;
      vbEdges->Lock(0, 0, &pV, 0);
      std::memcpy(pV, edgeVerts.data(), edgeVerts.size() * sizeof(Vertex));
      vbEdges->Unlock();
    }
  }

  NvmBatch batch;
  batch.Rx = rx;
  batch.Ry = ry;
  batch.VbWalkable = vbWalkable;
  batch.WalkableCount = static_cast<int>(walkableVerts.size());
  batch.VbBlocked = vbBlocked;
  batch.BlockedCount = static_cast<int>(blockedVerts.size());
  batch.VbEdges = vbEdges;
  batch.EdgeCount = static_cast<int>(edgeVerts.size());

  m_batches.push_back(batch);
}

void NvmRenderer::UnloadNavmesh(int rx, int ry) {
  auto it = std::remove_if(m_batches.begin(), m_batches.end(), [&](const NvmBatch &batch) {
    if (batch.Rx == rx && batch.Ry == ry) {
      if (batch.VbWalkable) batch.VbWalkable->Release();
      if (batch.VbBlocked) batch.VbBlocked->Release();
      if (batch.VbEdges) batch.VbEdges->Release();
      return true;
    }
    return false;
  });
  if (it != m_batches.end()) {
    m_batches.erase(it, m_batches.end());
  }
}

void NvmRenderer::Draw(const Matrix4 &view, const Matrix4 &proj,
                       int centerRx, int centerRy, int radius,
                       bool showWalkable, bool showBlocked, bool showEdges,
                       const CameraFrustum &frustum) {
  if (m_batches.empty()) return;

  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX *>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX *>(&proj));

  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

  float bias = -0.00001f;
  float slopeBias = -0.5f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slopeBias);

  m_device->SetFVF(Vertex::FVF);
  m_device->SetTexture(0, nullptr);
  for (const auto &batch : m_batches) {
    if (std::abs(batch.Rx - centerRx) > radius || std::abs(batch.Ry - centerRy) > radius) {
      continue;
    }

    float xOffset = (batch.Ry - centerRy) * 1920.0f;
    float zOffset = (batch.Rx - centerRx) * 1920.0f;
    if (!frustum.IsBoxVisible(Vector3(xOffset, -1000.0f, zOffset), Vector3(xOffset + 1920.0f, 3000.0f, zOffset + 1920.0f))) {
      continue;
    }

    Matrix4 world = MatrixTranslation(xOffset, 0.0f, zOffset);
    m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX *>(&world));

    if (showWalkable && batch.VbWalkable && batch.WalkableCount > 0) {
      m_device->SetStreamSource(0, batch.VbWalkable, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batch.WalkableCount / 3);
    }

    if (showBlocked && batch.VbBlocked && batch.BlockedCount > 0) {
      m_device->SetStreamSource(0, batch.VbBlocked, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batch.BlockedCount / 3);
    }

    if (showEdges && batch.VbEdges && batch.EdgeCount > 0) {
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      float edgeBias = -0.00003f;
      m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&edgeBias);
      m_device->SetStreamSource(0, batch.VbEdges, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_LINELIST, 0, batch.EdgeCount / 2);
      m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
      m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    }
  }

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  float zeroBias = 0.0f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&zeroBias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&zeroBias);
}

float NvmRenderer::GetHeight(float localX, float localZ,
                             const std::vector<float> &heightMap) const {
  if (heightMap.empty()) return 0.0f;

  float fx = localX / 20.0f;
  float fz = localZ / 20.0f;

  int x0 = static_cast<int>(floorf(fx));
  int z0 = static_cast<int>(floorf(fz));

  x0 = std::clamp(x0, 0, 96);
  z0 = std::clamp(z0, 0, 96);

  int x1 = std::clamp(x0 + 1, 0, 96);
  int z1 = std::clamp(z0 + 1, 0, 96);

  float tx = fx - floorf(fx);
  if (tx < 0.0f) tx = 0.0f;
  if (tx > 1.0f) tx = 1.0f;
  float tz = fz - floorf(fz);
  if (tz < 0.0f) tz = 0.0f;
  if (tz > 1.0f) tz = 1.0f;

  float h00 = 0.0f;
  float h10 = 0.0f;
  float h01 = 0.0f;
  float h11 = 0.0f;

  if (z0 * 97 + x0 < static_cast<int>(heightMap.size())) h00 = heightMap[z0 * 97 + x0];
  if (z0 * 97 + x1 < static_cast<int>(heightMap.size())) h10 = heightMap[z0 * 97 + x1];
  if (z1 * 97 + x0 < static_cast<int>(heightMap.size())) h01 = heightMap[z1 * 97 + x0];
  if (z1 * 97 + x1 < static_cast<int>(heightMap.size())) h11 = heightMap[z1 * 97 + x1];

  // Bilinear interpolation
  float h0 = h00 * (1.0f - tx) + h10 * tx;
  float h1 = h01 * (1.0f - tx) + h11 * tx;
  return h0 * (1.0f - tz) + h1 * tz;
}

static void DrawWireframeBox(LPDIRECT3DDEVICE9 device, const Vector3& center, float size, DWORD color) {
    float h = size * 0.5f;
    NvmRenderer::Vertex verts[24] = {
        // Top face
        { Vector3(center.x - h, center.y + h, center.z - h), color }, { Vector3(center.x + h, center.y + h, center.z - h), color },
        { Vector3(center.x + h, center.y + h, center.z - h), color }, { Vector3(center.x + h, center.y + h, center.z + h), color },
        { Vector3(center.x + h, center.y + h, center.z + h), color }, { Vector3(center.x - h, center.y + h, center.z + h), color },
        { Vector3(center.x - h, center.y + h, center.z + h), color }, { Vector3(center.x - h, center.y + h, center.z - h), color },
        // Bottom face
        { Vector3(center.x - h, center.y - h, center.z - h), color }, { Vector3(center.x + h, center.y - h, center.z - h), color },
        { Vector3(center.x + h, center.y - h, center.z - h), color }, { Vector3(center.x + h, center.y - h, center.z + h), color },
        { Vector3(center.x + h, center.y - h, center.z + h), color }, { Vector3(center.x - h, center.y - h, center.z + h), color },
        { Vector3(center.x - h, center.y - h, center.z + h), color }, { Vector3(center.x - h, center.y - h, center.z - h), color },
        // Verticals
        { Vector3(center.x - h, center.y - h, center.z - h), color }, { Vector3(center.x - h, center.y + h, center.z - h), color },
        { Vector3(center.x + h, center.y - h, center.z - h), color }, { Vector3(center.x + h, center.y + h, center.z - h), color },
        { Vector3(center.x + h, center.y - h, center.z + h), color }, { Vector3(center.x + h, center.y + h, center.z + h), color },
        { Vector3(center.x - h, center.y - h, center.z + h), color }, { Vector3(center.x - h, center.y + h, center.z + h), color }
    };
    device->DrawPrimitiveUP(D3DPT_LINELIST, 12, verts, sizeof(NvmRenderer::Vertex));
}

void NvmRenderer::DrawCellsAndSelection(const Matrix4& view, const Matrix4& proj, int centerRx, int centerRy,
                                         const sro::formats::NavMesh& nav, int rx, int ry,
                                         int selectedCellIdx, int selectedEdgeIdx, bool selectedEdgeIsGlobal) {
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX *>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX *>(&proj));

  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetFVF(Vertex::FVF);

  float bias = -0.00002f;
  float slopeBias = -0.8f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slopeBias);

  float xOffset = (ry - centerRy) * 1920.0f;
  float zOffset = (rx - centerRx) * 1920.0f;
  Matrix4 world = MatrixTranslation(xOffset, 0.0f, zOffset);
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX *>(&world));

  // 1. Draw all cells as white outlines
  DWORD cellColor = D3DCOLOR_ARGB(120, 255, 255, 255);
  for (int i = 0; i < (int)nav.Cells.size(); ++i) {
      const auto& cell = nav.Cells[i];
      float h00 = GetHeight(cell.MinX, cell.MinY, nav.HeightMap) + 0.1f;
      float h10 = GetHeight(cell.MaxX, cell.MinY, nav.HeightMap) + 0.1f;
      float h11 = GetHeight(cell.MaxX, cell.MaxY, nav.HeightMap) + 0.1f;
      float h01 = GetHeight(cell.MinX, cell.MaxY, nav.HeightMap) + 0.1f;

      Vertex lineVerts[8] = {
          { Vector3(cell.MinX, h00, cell.MinY), cellColor }, { Vector3(cell.MaxX, h10, cell.MinY), cellColor },
          { Vector3(cell.MaxX, h10, cell.MinY), cellColor }, { Vector3(cell.MaxX, h11, cell.MaxY), cellColor },
          { Vector3(cell.MaxX, h11, cell.MaxY), cellColor }, { Vector3(cell.MinX, h01, cell.MaxY), cellColor },
          { Vector3(cell.MinX, h01, cell.MaxY), cellColor }, { Vector3(cell.MinX, h00, cell.MinY), cellColor }
      };
      m_device->DrawPrimitiveUP(D3DPT_LINELIST, 4, lineVerts, sizeof(Vertex));

      if (i == selectedCellIdx) {
          DWORD fillCol = D3DCOLOR_ARGB(70, 255, 255, 0);
          Vertex fillVerts[6] = {
              { Vector3(cell.MinX, h00, cell.MinY), fillCol },
              { Vector3(cell.MinX, h01, cell.MaxY), fillCol },
              { Vector3(cell.MaxX, h10, cell.MinY), fillCol },
              { Vector3(cell.MaxX, h10, cell.MinY), fillCol },
              { Vector3(cell.MinX, h01, cell.MaxY), fillCol },
              { Vector3(cell.MaxX, h11, cell.MaxY), fillCol }
          };
          m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, fillVerts, sizeof(Vertex));

          DWORD handleCol = D3DCOLOR_ARGB(255, 230, 20, 20);
          DrawWireframeBox(m_device, Vector3(cell.MinX, h00, cell.MinY), 1.5f, handleCol);
          DrawWireframeBox(m_device, Vector3(cell.MaxX, h10, cell.MinY), 1.5f, handleCol);
          DrawWireframeBox(m_device, Vector3(cell.MaxX, h11, cell.MaxY), 1.5f, handleCol);
          DrawWireframeBox(m_device, Vector3(cell.MinX, h01, cell.MaxY), 1.5f, handleCol);
      }
  }

  // 2. Draw selected edge highlight
  if (selectedEdgeIdx >= 0) {
      float eMinX = 0, eMinY = 0, eMaxX = 0, eMaxY = 0;
      if (selectedEdgeIsGlobal) {
          if (selectedEdgeIdx < (int)nav.GlobalEdges.size()) {
              const auto& edge = nav.GlobalEdges[selectedEdgeIdx];
              eMinX = edge.MinX; eMinY = edge.MinY; eMaxX = edge.MaxX; eMaxY = edge.MaxY;
          }
      } else {
          if (selectedEdgeIdx < (int)nav.InternalEdges.size()) {
              const auto& edge = nav.InternalEdges[selectedEdgeIdx];
              eMinX = edge.MinX; eMinY = edge.MinY; eMaxX = edge.MaxX; eMaxY = edge.MaxY;
          }
      }

      if (eMinX != eMaxX || eMinY != eMaxY) {
          float h0 = GetHeight(eMinX, eMinY, nav.HeightMap) + 0.15f;
          float h1 = GetHeight(eMaxX, eMaxY, nav.HeightMap) + 0.15f;

          DWORD selectLineCol = D3DCOLOR_ARGB(255, 255, 0, 0);
          Vertex edgeVerts[2] = {
              { Vector3(eMinX, h0, eMinY), selectLineCol },
              { Vector3(eMaxX, h1, eMaxY), selectLineCol }
          };
          m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
          m_device->DrawPrimitiveUP(D3DPT_LINELIST, 1, edgeVerts, sizeof(Vertex));
          m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);

          DWORD handleCol = D3DCOLOR_ARGB(255, 255, 140, 0);
          DrawWireframeBox(m_device, Vector3(eMinX, h0, eMinY), 1.5f, handleCol);
          DrawWireframeBox(m_device, Vector3(eMaxX, h1, eMaxY), 1.5f, handleCol);
      }
  }

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  float zeroBias = 0.0f;
  m_device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&zeroBias);
  m_device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&zeroBias);
}

void NvmRenderer::RebuildNavmeshBuffers(const sro::formats::NavMesh *nav, int rx, int ry) {
  if (!nav) return;
  UnloadNavmesh(rx, ry);
  LoadNavmesh(*nav, rx, ry);
}
