#include "rendering/TerrainRenderer.h"
#include "parsers/MapMeshParser.h"
#include "cache/ClientDataCache.h"
#include "core/SceneSpace.h"
#include "Rendering/RenderManager.h"
#include "Core/Log.h"
#include "Core/Utils.h"
#include <cstring>
#include <fstream>
#include <set>

// ============================================================================
// TerrainRenderer implementation
// ============================================================================

TerrainRenderer::TerrainRenderer(LPDIRECT3DDEVICE9 device,
                                 const std::wstring &clientPath,
                                 TextureManager *texMgr)
    : m_device(device), m_clientPath(clientPath), m_texManager(texMgr) {
  LoadTileIfo();
}

TerrainRenderer::~TerrainRenderer() { ClearBatches(); }

void TerrainRenderer::ClearBatches() {
  for (auto &reg : m_loadedRegions) {
    for (auto &batch : reg.BaseBatches) {
      if (batch.Vb)
        batch.Vb->Release();
    }
    for (auto &batch : reg.BlendBatches) {
      if (batch.Vb)
        batch.Vb->Release();
    }
  }
  m_loadedRegions.clear();
  m_loadedRegionSet.clear();
  m_failedRegionSet.clear();
  m_loadedTextures.clear();
}

void TerrainRenderer::LoadTileIfo() {
  std::wstring ifoPath = m_clientPath + L"/Map/tile2d.ifo";
  std::ifstream fs(ifoPath, std::ios::binary);
  if (!fs.is_open())
    return;

  std::string line;
  while (std::getline(fs, line)) {
    if (line.empty() || line.rfind("JMXV", 0) == 0 || !isdigit(line[0]))
      continue;

    size_t q1 = line.find('\"');
    if (q1 == std::string::npos)
      continue;
    size_t q2 = line.find('\"', q1 + 1);
    if (q2 == std::string::npos)
      continue;
    size_t q3 = line.find('\"', q2 + 1);
    if (q3 == std::string::npos)
      continue;
    size_t q4 = line.find('\"', q3 + 1);
    if (q4 == std::string::npos)
      continue;

    std::string idStr = line.substr(0, q1);
    idStr = idStr.substr(0, idStr.find(' '));
    uint16_t id = static_cast<uint16_t>(std::stoi(idStr));

    std::string ddjName = line.substr(q3 + 1, q4 - q3 - 1);
    std::wstring wDdjName(ddjName.begin(), ddjName.end());
    m_tileTextures[id] = wDdjName;
  }
}

void TerrainRenderer::LoadRegion(int rx, int ry, int radius) {
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      int targetRx = rx + dx;
      int targetRy = ry + dy;

      // O(1) lookup instead of O(n) linear scan
      if (m_loadedRegionSet.count({targetRx, targetRy}) == 0 &&
          m_failedRegionSet.count({targetRx, targetRy}) == 0) {
        LoadSingleRegion(targetRx, targetRy);
      }
    }
  }
}

void TerrainRenderer::UnloadRegion(int rx, int ry) {
  auto it = std::remove_if(m_loadedRegions.begin(), m_loadedRegions.end(),
                           [&](const LoadedRegion &reg) {
                             if (reg.Rx == rx && reg.Ry == ry) {
                               for (auto &batch : reg.BaseBatches) {
                                 if (batch.Vb)
                                   batch.Vb->Release();
                               }
                               // Fix: also release blend batch vertex buffers
                               // (was leaking before)
                               for (auto &batch : reg.BlendBatches) {
                                 if (batch.Vb)
                                   batch.Vb->Release();
                               }
                               return true;
                             }
                             return false;
                           });
  if (it != m_loadedRegions.end()) {
    m_loadedRegions.erase(it, m_loadedRegions.end());
    m_loadedRegionSet.erase({rx, ry});
  }
}

void TerrainRenderer::LoadSingleRegion(int rx, int ry) {
  auto pair = std::make_pair(rx, ry);
    sro::formats::MapMesh* pMesh = nullptr;

  auto it = m_regionMeshes.find(pair);
  if (it != m_regionMeshes.end()) {
    pMesh = &it->second;
  } else {
    std::wstring mPath = m_clientPath + L"/Map/" + std::to_wstring(rx) + L"/" +
                         std::to_wstring(ry) + L".m";
    if (!FileExists(mPath)) {
      LogMsgW(L"[Terrain] Missing: " + mPath);
      m_failedRegionSet.insert({rx, ry});
      return;
    }

    sro::formats::MapMesh mesh;
    if (!sro::ClientDataCache::Instance().LoadMapMesh(mPath, mesh)) {
      LogMsgW(L"[Terrain] Failed to parse: " + mPath);
      m_failedRegionSet.insert({rx, ry});
      return;
    }

    m_regionMeshes[pair] = mesh;
    pMesh = &m_regionMeshes[pair];
  }

  const auto &mapMesh = *pMesh;

  std::map<uint16_t, std::vector<Vertex>> baseBatchData;
  std::map<uint16_t, std::vector<Vertex>> blendBatchData;

  for (int bz = 0; bz < 6; ++bz) {
    for (int bx = 0; bx < 6; ++bx) {
      int blockIdx = bz * 6 + bx;
      const auto &block = mapMesh.Blocks[blockIdx];
      float blockX = bx * 320.0f;
      float blockZ = bz * 320.0f;

      for (int tz = 0; tz < 16; ++tz) {
        for (int tx = 0; tx < 16; ++tx) {
          uint16_t idBL = block.Vertices[tz * 17 + tx].TextureData & 0x03FF;
          uint16_t idBR =
              block.Vertices[tz * 17 + (tx + 1)].TextureData & 0x03FF;
          uint16_t idTL =
              block.Vertices[(tz + 1) * 17 + tx].TextureData & 0x03FF;
          uint16_t idTR =
              block.Vertices[(tz + 1) * 17 + (tx + 1)].TextureData & 0x03FF;

          uint16_t tileVal = block.Tiles[tz * 16 + tx];
          int rot = (tileVal >> 14) & 3;

          int scaleBL = block.Vertices[tz * 17 + tx].TextureData >> 10;
          int scaleBR = block.Vertices[tz * 17 + (tx + 1)].TextureData >> 10;
          int scaleTL = block.Vertices[(tz + 1) * 17 + tx].TextureData >> 10;
          int scaleTR =
              block.Vertices[(tz + 1) * 17 + (tx + 1)].TextureData >> 10;

          float scaleValBL =
              (scaleBL == 0) ? 16.0f : static_cast<float>(scaleBL);
          float scaleValBR =
              (scaleBR == 0) ? 16.0f : static_cast<float>(scaleBR);
          float scaleValTL =
              (scaleTL == 0) ? 16.0f : static_cast<float>(scaleTL);
          float scaleValTR =
              (scaleTR == 0) ? 16.0f : static_cast<float>(scaleTR);

          float hBL = block.Vertices[tz * 17 + tx].Height;
          float hBR = block.Vertices[tz * 17 + (tx + 1)].Height;
          float hTL = block.Vertices[(tz + 1) * 17 + tx].Height;
          float hTR = block.Vertices[(tz + 1) * 17 + (tx + 1)].Height;

          auto getCol = [](float b, float a) -> DWORD {
            int br = static_cast<int>(b * 255.0f);
            int alpha = static_cast<int>(a * 255.0f);
            return D3DCOLOR_ARGB(alpha, br, br, br);
          };

          float bBL =
              0.8f + (block.Vertices[tz * 17 + tx].Brightness / 255.0f) * 0.2f;
          float bBR =
              0.8f +
              (block.Vertices[tz * 17 + (tx + 1)].Brightness / 255.0f) * 0.2f;
          float bTL =
              0.8f +
              (block.Vertices[(tz + 1) * 17 + tx].Brightness / 255.0f) * 0.2f;
          float bTR =
              0.8f +
              (block.Vertices[(tz + 1) * 17 + (tx + 1)].Brightness / 255.0f) *
                  0.2f;

          float x0 = blockX + tx * 20.0f;
          float z0 = blockZ + tz * 20.0f;
          float x1 = x0 + 20.0f;
          float z1 = z0 + 20.0f;

          // Base Pass
          float baseUvDiv = scaleValBL * 20.0f;
          Vector2 uvBL(x0 / baseUvDiv, z0 / baseUvDiv);
          Vector2 uvBR(x1 / baseUvDiv, z0 / baseUvDiv);
          Vector2 uvTL(x0 / baseUvDiv, z1 / baseUvDiv);
          Vector2 uvTR(x1 / baseUvDiv, z1 / baseUvDiv);

          if (rot == 1) {
            Vector2 tBL = uvBL, tBR = uvBR, tTL = uvTL, tTR = uvTR;
            uvBL = tTL;
            uvBR = tBL;
            uvTL = tTR;
            uvTR = tBR;
          } else if (rot == 2) {
            Vector2 tBL = uvBL, tBR = uvBR, tTL = uvTL, tTR = uvTR;
            uvBL = tTR;
            uvBR = tTL;
            uvTL = tBR;
            uvTR = tBL;
          } else if (rot == 3) {
            Vector2 tBL = uvBL, tBR = uvBR, tTL = uvTL, tTR = uvTR;
            uvBL = tBR;
            uvBR = tTR;
            uvTL = tBL;
            uvTR = tTL;
          }

          auto &baseList = baseBatchData[idBL];
          baseList.push_back({Vector3(x0, hBL, z0), getCol(bBL, 1.0f), uvBL});
          baseList.push_back({Vector3(x0, hTL, z1), getCol(bTL, 1.0f), uvTL});
          baseList.push_back({Vector3(x1, hBR, z0), getCol(bBR, 1.0f), uvBR});
          baseList.push_back({Vector3(x1, hBR, z0), getCol(bBR, 1.0f), uvBR});
          baseList.push_back({Vector3(x0, hTL, z1), getCol(bTL, 1.0f), uvTL});
          baseList.push_back({Vector3(x1, hTR, z1), getCol(bTR, 1.0f), uvTR});

          // Blend Pass
          std::set<uint16_t> uniqueDetailIds;
          if (idBR != idBL)
            uniqueDetailIds.insert(idBR);
          if (idTL != idBL)
            uniqueDetailIds.insert(idTL);
          if (idTR != idBL)
            uniqueDetailIds.insert(idTR);

          for (uint16_t blendTexId : uniqueDetailIds) {
            float blendScaleVal = 16.0f;
            if (idBR == blendTexId)
              blendScaleVal = scaleValBR;
            else if (idTL == blendTexId)
              blendScaleVal = scaleValTL;
            else if (idTR == blendTexId)
              blendScaleVal = scaleValTR;

            float blendUvDiv = blendScaleVal * 20.0f;
            Vector2 uvBlendBL(x0 / blendUvDiv, z0 / blendUvDiv);
            Vector2 uvBlendBR(x1 / blendUvDiv, z0 / blendUvDiv);
            Vector2 uvBlendTL(x0 / blendUvDiv, z1 / blendUvDiv);
            Vector2 uvBlendTR(x1 / blendUvDiv, z1 / blendUvDiv);

            if (rot == 1) {
              Vector2 tBL = uvBlendBL, tBR = uvBlendBR, tTL = uvBlendTL,
                      tTR = uvBlendTR;
              uvBlendBL = tTL;
              uvBlendBR = tBL;
              uvBlendTL = tTR;
              uvBlendTR = tBR;
            } else if (rot == 2) {
              Vector2 tBL = uvBlendBL, tBR = uvBlendBR, tTL = uvBlendTL,
                      tTR = uvBlendTR;
              uvBlendBL = tTR;
              uvBlendBR = tTL;
              uvBlendTL = tBR;
              uvBlendTR = tBL;
            } else if (rot == 3) {
              Vector2 tBL = uvBlendBL, tBR = uvBlendBR, tTL = uvBlendTL,
                      tTR = uvBlendTR;
              uvBlendBL = tBR;
              uvBlendBR = tTR;
              uvBlendTL = tBL;
              uvBlendTR = tTL;
            }

            float aBL = (idBL == blendTexId) ? 1.0f : 0.0f;
            float aBR = (idBR == blendTexId) ? 1.0f : 0.0f;
            float aTL = (idTL == blendTexId) ? 1.0f : 0.0f;
            float aTR = (idTR == blendTexId) ? 1.0f : 0.0f;

            auto &blendList = blendBatchData[blendTexId];
            blendList.push_back(
                {Vector3(x0, hBL, z0), getCol(bBL, aBL), uvBlendBL});
            blendList.push_back(
                {Vector3(x0, hTL, z1), getCol(bTL, aTL), uvBlendTL});
            blendList.push_back(
                {Vector3(x1, hBR, z0), getCol(bBR, aBR), uvBlendBR});
            blendList.push_back(
                {Vector3(x1, hBR, z0), getCol(bBR, aBR), uvBlendBR});
            blendList.push_back(
                {Vector3(x0, hTL, z1), getCol(bTL, aTL), uvBlendTL});
            blendList.push_back(
                {Vector3(x1, hTR, z1), getCol(bTR, aTR), uvBlendTR});
          }
        }
      }
    }
  }

  LoadedRegion loadedReg{rx, ry};

  for (auto &kvp : baseBatchData) {
    uint16_t texId = kvp.first;
    const auto &vertices = kvp.second;
    if (vertices.empty())
      continue;

    auto it = m_tileTextures.find(texId);
    if (it != m_tileTextures.end()) {
      std::wstring texPath = m_clientPath + L"/Map/tile2d/" + it->second;
      m_loadedTextures[texId] = m_texManager->GetTexture(texPath, false);
    }

    LPDIRECT3DVERTEXBUFFER9 vb = nullptr;
    m_device->CreateVertexBuffer(
        static_cast<UINT>(vertices.size() * sizeof(Vertex)), D3DUSAGE_WRITEONLY,
        Vertex::FVF, D3DPOOL_MANAGED, &vb, nullptr);

    if (vb) {
      void *pVoid = nullptr;
      vb->Lock(0, 0, &pVoid, 0);
      std::memcpy(pVoid, vertices.data(), vertices.size() * sizeof(Vertex));
      vb->Unlock();
      loadedReg.BaseBatches.push_back(
          {vb, static_cast<int>(vertices.size()), texId});
    }
  }

  for (auto &kvp : blendBatchData) {
    uint16_t texId = kvp.first;
    const auto &vertices = kvp.second;
    if (vertices.empty())
      continue;

    auto it = m_tileTextures.find(texId);
    if (it != m_tileTextures.end()) {
      std::wstring texPath = m_clientPath + L"/Map/tile2d/" + it->second;
      m_loadedTextures[texId] = m_texManager->GetTexture(texPath);
    }

    LPDIRECT3DVERTEXBUFFER9 vb = nullptr;
    m_device->CreateVertexBuffer(
        static_cast<UINT>(vertices.size() * sizeof(Vertex)), D3DUSAGE_WRITEONLY,
        Vertex::FVF, D3DPOOL_MANAGED, &vb, nullptr);

    if (vb) {
      void *pVoid = nullptr;
      vb->Lock(0, 0, &pVoid, 0);
      std::memcpy(pVoid, vertices.data(), vertices.size() * sizeof(Vertex));
      vb->Unlock();
      loadedReg.BlendBatches.push_back(
          {vb, static_cast<int>(vertices.size()), texId});
    }
  }

  m_loadedRegions.push_back(loadedReg);
  m_loadedRegionSet.insert({rx, ry});
}

void TerrainRenderer::Draw(const Matrix4 &view, const Matrix4 &proj,
                           int centerRx, int centerRy, int radius,
                           const CameraFrustum &frustum, int timeOfDay, bool wireframe) {
  m_device->SetTransform(D3DTS_VIEW,
                         reinterpret_cast<const D3DMATRIX *>(&view));
  m_device->SetTransform(D3DTS_PROJECTION,
                         reinterpret_cast<const D3DMATRIX *>(&proj));

  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
  m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
  if (wireframe) {
    m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  }
  
  if (timeOfDay == 1) {
    m_device->SetRenderState(D3DRS_TEXTUREFACTOR,
                             D3DCOLOR_RGBA(110, 120, 145, 255));
    m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
    m_device->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_device->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
  } else {
    m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
  }
  m_device->SetFVF(Vertex::FVF);

  m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
  m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 8);

  m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

  for (const auto &reg : m_loadedRegions) {
    if (std::abs(reg.Rx - centerRx) > radius || std::abs(reg.Ry - centerRy) > radius) {
      continue;
    }

    float offsetX = (reg.Ry - centerRy) * 1920.0f;
    float offsetZ = (reg.Rx - centerRx) * 1920.0f;

    if (!frustum.IsBoxVisible(
            Vector3(offsetX, -1000.0f, offsetZ),
            Vector3(offsetX + 1920.0f, 3000.0f, offsetZ + 1920.0f))) {
      continue;
    }

    Matrix4 world = MatrixTranslation(offsetX, 0.0f, offsetZ);
    m_device->SetTransform(D3DTS_WORLD,
                           reinterpret_cast<const D3DMATRIX *>(&world));

    // Base Pass
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    for (const auto &batch : reg.BaseBatches) {
      Texture *tex = m_loadedTextures[batch.TextureId];
      if (!tex) {
        auto it = m_tileTextures.find(batch.TextureId);
        if (it != m_tileTextures.end()) {
          std::wstring texPath = m_clientPath + L"/Map/tile2d/" + it->second;
          tex = m_texManager->GetTexture(texPath, true);
          if (tex) {
            m_loadedTextures[batch.TextureId] = tex;
          }
        }
      }
      Texture* drawTex = tex ? tex : m_texManager->GetDefaultTexture();
      m_device->SetTexture(0, drawTex ? drawTex->pTexture : nullptr);
      m_device->SetStreamSource(0, batch.Vb, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batch.VertexCount / 3);
    }

    // Blend Pass
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

    for (const auto &batch : reg.BlendBatches) {
      Texture *tex = m_loadedTextures[batch.TextureId];
      if (!tex) {
        auto it = m_tileTextures.find(batch.TextureId);
        if (it != m_tileTextures.end()) {
          std::wstring texPath = m_clientPath + L"/Map/tile2d/" + it->second;
          tex = m_texManager->GetTexture(texPath, true);
          if (tex) {
            m_loadedTextures[batch.TextureId] = tex;
          }
        }
      }
      Texture* drawTex = tex ? tex : m_texManager->GetDefaultTexture();
      m_device->SetTexture(0, drawTex ? drawTex->pTexture : nullptr);
      m_device->SetStreamSource(0, batch.Vb, 0, sizeof(Vertex));
      m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batch.VertexCount / 3);
    }
  }

  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
  m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
  if (wireframe) {
    m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
  }
}

void TerrainRenderer::RebuildTerrainBuffers(int rx, int ry) {
  UnloadRegion(rx, ry);
  LoadSingleRegion(rx, ry);
}

float TerrainRenderer::GetHeight(float x, float z, int centerRx,
                                 int centerRy) const {
  sro::LocalPos lp = sro::SceneSpace::SceneToLocal(x, z, centerRx, centerRy);
  auto pair = std::make_pair(lp.rx, lp.ry);
  auto it = m_regionMeshes.find(pair);
  if (it == m_regionMeshes.end()) return 0.0f;

  const auto& mesh = it->second;
  float lx = lp.localX;
  float lz = lp.localZ;

  int bx = std::clamp(static_cast<int>(floorf(lx / 320.0f)), 0, 5);
  int bz = std::clamp(static_cast<int>(floorf(lz / 320.0f)), 0, 5);

  float blockLx = lx - bx * 320.0f;
  float blockLz = lz - bz * 320.0f;

  float fx = blockLx / 20.0f;
  float fz = blockLz / 20.0f;

  int x0 = std::clamp(static_cast<int>(floorf(fx)), 0, 16);
  int z0 = std::clamp(static_cast<int>(floorf(fz)), 0, 16);
  int x1 = std::clamp(x0 + 1, 0, 16);
  int z1 = std::clamp(z0 + 1, 0, 16);

  float tx = fx - floorf(fx);
  float tz = fz - floorf(fz);

  const auto& block = mesh.Blocks[bz * 6 + bx];
  float h00 = block.Vertices[z0 * 17 + x0].Height;
  float h10 = block.Vertices[z0 * 17 + x1].Height;
  float h01 = block.Vertices[z1 * 17 + x0].Height;
  float h11 = block.Vertices[z1 * 17 + x1].Height;

  float h0 = h00 * (1.0f - tx) + h10 * tx;
  float h1 = h01 * (1.0f - tx) + h11 * tx;
  return h0 * (1.0f - tz) + h1 * tz;
}
