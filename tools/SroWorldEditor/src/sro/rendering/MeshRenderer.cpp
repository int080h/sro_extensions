#include "MeshRenderer.h"
#include "Core/Log.h"
#include "Core/Utils.h"
#include "Parsers/ParserHelpers.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <filesystem>

// Skeletal attachment helper structures and functions
struct BskBone {
    std::string Name;
    Vector4 RotationToOrigin;
    Vector3 TranslationToOrigin;
};

static inline Matrix4 QuaternionToMatrix(const Vector4& q) {
    Matrix4 m = Matrix4::Identity();
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    m.m[0][0] = 1.0f - 2.0f * (yy + zz);
    m.m[0][1] = 2.0f * (xy + wz);
    m.m[0][2] = 2.0f * (xz - wy);

    m.m[1][0] = 2.0f * (xy - wz);
    m.m[1][1] = 1.0f - 2.0f * (xx + zz);
    m.m[1][2] = 2.0f * (yz + wx);

    m.m[2][0] = 2.0f * (xz + wy);
    m.m[2][1] = 2.0f * (yz - wx);
    m.m[2][2] = 1.0f - 2.0f * (xx + yy);
    return m;
}

static inline Vector3 Vec3Transform(const Vector3& v, const Matrix4& m) {
    Vector3 out;
    out.x = v.x * m.m[0][0] + v.y * m.m[0][1] + v.z * m.m[0][2] + m.m[3][0];
    out.y = v.x * m.m[1][0] + v.y * m.m[1][1] + v.z * m.m[1][2] + m.m[3][1];
    out.z = v.x * m.m[2][0] + v.y * m.m[2][1] + v.z * m.m[2][2] + m.m[3][2];
    return out;
}

static inline Vector3 Vec3TransformNormal(const Vector3& n, const Matrix4& m) {
    Vector3 out;
    out.x = n.x * m.m[0][0] + n.y * m.m[0][1] + n.z * m.m[0][2];
    out.y = n.x * m.m[1][0] + n.y * m.m[1][1] + n.z * m.m[1][2];
    out.z = n.x * m.m[2][0] + n.y * m.m[2][1] + n.z * m.m[2][2];
    return Vec3Normalize(out);
}

// ============================================================================
// MeshRenderer implementation
// ============================================================================

MeshRenderer::MeshRenderer(LPDIRECT3DDEVICE9 device, const std::wstring &clientPath,
                           TextureManager *texMgr)
    : m_device(device), m_clientPath(clientPath), m_texManager(texMgr) {
  LoadObjectIfo();
}

MeshRenderer::~MeshRenderer() {
  for (auto &pair : m_modelCache) {
    if (pair.second) {
      for (auto &sub : pair.second->SubMeshes) {
        if (sub.Vb) sub.Vb->Release();
        if (sub.Ib) sub.Ib->Release();
      }
    }
  }
}

void MeshRenderer::LoadObjectIfo() {
  std::wstring ifoPath = m_clientPath + L"/Data/object.ifo";
  if (!FileExists(ifoPath)) {
    ifoPath = m_clientPath + L"/Map/object.ifo";
  }
  if (!FileExists(ifoPath)) {
    ifoPath = m_clientPath + L"/Data/navmesh/object.ifo";
  }

  std::ifstream fs(ifoPath);
  if (!fs.is_open()) {
    LogMsgW(L"[MeshRenderer] Warning: object.ifo not found in Data, Map, or Data/navmesh.");
    return;
  }
  LogMsgW(L"[MeshRenderer] Loading object mappings from: " + ifoPath);

  std::string line;
  while (std::getline(fs, line)) {
    if (line.empty() || !isdigit(line[0])) continue;

    size_t space1 = line.find_first_of(" \t");
    if (space1 == std::string::npos) continue;
    size_t space2 = line.find_first_of(" \t", space1 + 1);
    if (space2 == std::string::npos) continue;

    try {
      uint32_t objId = static_cast<uint32_t>(std::stoul(line.substr(0, space1)));
      size_t q1 = line.find('\"', space2 + 1);
      if (q1 == std::string::npos) continue;
      size_t q2 = line.find('\"', q1 + 1);
      if (q2 == std::string::npos) continue;
      std::string path = line.substr(q1 + 1, q2 - q1 - 1);

      while (!path.empty() && (path.back() == '\r' || path.back() == '\n' || path.back() == '\t' || path.back() == ' ')) {
        path.pop_back();
      }
      if (!path.empty()) {
        m_objectMapping[objId] = path;
      }
    } catch (...) {}
  }
}

std::string MeshRenderer::GetModelBsrPath(uint32_t objId) {
  auto it = m_objectMapping.find(objId);
  if (it != m_objectMapping.end()) return it->second;
  return "";
}

MeshRenderer::ModelResource *MeshRenderer::PreloadModel(const std::string &bsrRelPath) {
  auto it = m_modelCache.find(bsrRelPath);
  if (it != m_modelCache.end()) return it->second.get();

  std::string lower = bsrRelPath;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".cpd") == 0) {
    return LoadCompoundModelResource(bsrRelPath);
  }

  return LoadBsrModelResource(bsrRelPath);
}

void MeshRenderer::MergeModelResource(ModelResource& dst, const ModelResource& src) {
  if (src.SubMeshes.empty()) return;

  if (dst.SubMeshes.empty()) {
    dst.MinBounds = src.MinBounds;
    dst.MaxBounds = src.MaxBounds;
  } else {
    dst.MinBounds.x = (std::min)(dst.MinBounds.x, src.MinBounds.x);
    dst.MinBounds.y = (std::min)(dst.MinBounds.y, src.MinBounds.y);
    dst.MinBounds.z = (std::min)(dst.MinBounds.z, src.MinBounds.z);
    dst.MaxBounds.x = (std::max)(dst.MaxBounds.x, src.MaxBounds.x);
    dst.MaxBounds.y = (std::max)(dst.MaxBounds.y, src.MaxBounds.y);
    dst.MaxBounds.z = (std::max)(dst.MaxBounds.z, src.MaxBounds.z);
  }

  dst.SubMeshes.insert(dst.SubMeshes.end(), src.SubMeshes.begin(), src.SubMeshes.end());
  dst.SubMeshCount = static_cast<int>(dst.SubMeshes.size());
  dst.TotalVertices = 0;
  dst.TotalIndices = 0;
  for (const auto& sm : dst.SubMeshes) {
    dst.TotalVertices += sm.VertexCount;
    dst.TotalIndices += sm.IndexCount;
  }
}

MeshRenderer::ModelResource *MeshRenderer::LoadCompoundModelResource(const std::string &cpdRelPath) {
  std::wstring wRelPath(cpdRelPath.begin(), cpdRelPath.end());
  std::replace(wRelPath.begin(), wRelPath.end(), L'\\', L'/');
  std::wstring cpdPath = m_clientPath + L"/" + wRelPath;
  if (!FileExists(cpdPath)) {
    cpdPath = m_clientPath + L"/Data/" + wRelPath;
  }

  CpdResource cpd;
  if (!cpd.Read(cpdPath)) {
    LogMsgW(L"[MeshRenderer] Error: Failed to read CPD file: " + cpdPath);
    m_modelCache[cpdRelPath] = nullptr;
    return nullptr;
  }

  auto merged = std::make_unique<ModelResource>();
  for (const auto& resourcePath : cpd.ResourcePaths) {
    if (ModelResource* part = LoadBsrModelResource(resourcePath)) {
      MergeModelResource(*merged, *part);
    }
  }

  if (merged->SubMeshes.empty()) {
    LogMsgW(L"[MeshRenderer] Error: CPD contained no renderable BSR resources: " + cpdPath);
    m_modelCache[cpdRelPath] = nullptr;
    return nullptr;
  }

  ModelResource* ptr = merged.get();
  m_modelCache[cpdRelPath] = std::move(merged);
  return ptr;
}

MeshRenderer::ModelResource *MeshRenderer::LoadBsrModelResource(const std::string &bsrRelPath) {
  auto it = m_modelCache.find(bsrRelPath);
  if (it != m_modelCache.end()) return it->second.get();

  std::wstring wRelPath(bsrRelPath.begin(), bsrRelPath.end());
  std::replace(wRelPath.begin(), wRelPath.end(), L'\\', L'/');
  std::wstring bsrPath = m_clientPath + L"/" + wRelPath;
  if (!FileExists(bsrPath)) {
    bsrPath = m_clientPath + L"/Data/" + wRelPath;
  }

  BsrResource bsr;
  if (!bsr.Read(bsrPath)) {
    LogMsgW(L"[MeshRenderer] Error: Failed to read BSR file: " + bsrPath);
    m_modelCache[bsrRelPath] = nullptr;
    return nullptr;
  }

  auto model = std::make_unique<ModelResource>();

  // Load materials
  std::map<std::string, std::wstring> materialTextureMap;
  for (const auto &mtrlRelPath : bsr.MaterialPaths) {
    if (mtrlRelPath.empty()) continue;
    std::wstring wMtrlRelPath(mtrlRelPath.begin(), mtrlRelPath.end());
    std::replace(wMtrlRelPath.begin(), wMtrlRelPath.end(), L'\\', L'/');
    std::wstring mtrlPath = m_clientPath + L"/Data/" + wMtrlRelPath;

    BmtParser bmt;
    if (bmt.Read(mtrlPath)) {
      std::wstring mtrlDir = std::filesystem::path(mtrlPath).parent_path().wstring();
      for (const auto &entry : bmt.Entries) {
        if (!entry.TextureName.empty()) {
          std::wstring wTexName(entry.TextureName.begin(), entry.TextureName.end());
          std::replace(wTexName.begin(), wTexName.end(), L'\\', L'/');
          std::string nameKey = entry.Name;
          std::transform(nameKey.begin(), nameKey.end(), nameKey.begin(), ::tolower);

          std::wstring resolvedPath = mtrlDir + L"/" + wTexName;
          std::wstring dataRelativePath = m_clientPath + L"/Data/" + wTexName;
          if (FileExists(dataRelativePath) ||
              (wTexName.length() >= 5 && _wcsnicmp(wTexName.c_str(), L"prim/", 5) == 0)) {
            resolvedPath = dataRelativePath;
          }
          materialTextureMap[nameKey] = resolvedPath;
        }
      }
    } else {
      LogMsgW(L"[MeshRenderer] Warning: Failed to read material file: " + mtrlPath);
    }
  }

  // Load skeleton and build mesh-to-bone maps
  std::map<uint32_t, std::string> meshIdxToBoneName;
  for (const auto& group : bsr.MeshGroups) {
    for (uint32_t idx : group.MeshIndices) {
      meshIdxToBoneName[idx] = group.BoneName;
    }
  }

  std::map<std::string, Matrix4> boneTransforms;
  if (!bsr.SkeletonPath.empty()) {
    std::wstring wSkelPath(bsr.SkeletonPath.begin(), bsr.SkeletonPath.end());
    std::replace(wSkelPath.begin(), wSkelPath.end(), L'\\', L'/');
    std::wstring skelPath = m_clientPath + L"/" + wSkelPath;
    if (!FileExists(skelPath)) {
      skelPath = m_clientPath + L"/Data/" + wSkelPath;
    }

    std::ifstream fs(skelPath, std::ios::binary);
    if (fs.is_open()) {
      char sig[13] = {0};
      fs.read(sig, 12);
      if (std::string(sig).rfind("JMXVBSK", 0) == 0) {
        uint32_t boneCount = 0;
        ReadVal(fs, boneCount);
        if (boneCount < 10000) {
          for (uint32_t i = 0; i < boneCount; ++i) {
            uint8_t type = 0;
            ReadVal(fs, type);
            uint32_t nameLen = 0;
            ReadVal(fs, nameLen);
            std::string boneName = ReadString(fs, nameLen);
            uint32_t parentLen = 0;
            ReadVal(fs, parentLen);
            std::string parentName = ReadString(fs, parentLen);

            // rotationToParent, translationToParent (28 bytes)
            fs.seekg(28, std::ios::cur);

            Vector4 rotToOrigin;
            ReadVal(fs, rotToOrigin.x);
            ReadVal(fs, rotToOrigin.y);
            ReadVal(fs, rotToOrigin.z);
            ReadVal(fs, rotToOrigin.w);

            Vector3 transToOrigin;
            ReadVal(fs, transToOrigin.x);
            ReadVal(fs, transToOrigin.y);
            ReadVal(fs, transToOrigin.z);

            // rotationToLocal, translationToLocal (28 bytes)
            fs.seekg(28, std::ios::cur);

            uint32_t childCount = 0;
            ReadVal(fs, childCount);
            for (uint32_t c = 0; c < childCount; ++c) {
              uint32_t childNameLen = 0;
              ReadVal(fs, childNameLen);
              fs.seekg(childNameLen, std::ios::cur);
            }

            Matrix4 transform = QuaternionToMatrix(rotToOrigin);
            transform.m[3][0] = transToOrigin.x;
            transform.m[3][1] = transToOrigin.y;
            transform.m[3][2] = transToOrigin.z;

            boneTransforms[boneName] = transform;
          }
        }
      }
    }
  }

  // Load meshes
  uint32_t meshIdx = 0;
  for (const auto &meshRelPath : bsr.MeshPaths) {
    if (meshRelPath.empty()) continue;
    std::wstring wMeshRelPath(meshRelPath.begin(), meshRelPath.end());
    std::replace(wMeshRelPath.begin(), wMeshRelPath.end(), L'\\', L'/');
    std::wstring meshPath = m_clientPath + L"/Data/" + wMeshRelPath;

    BmsMesh bms;
    if (!bms.Read(meshPath)) {
      LogMsgW(L"[MeshRenderer] Error: Failed to read BMS mesh file: " + meshPath);
      meshIdx++;
      continue;
    }

    auto flatVerts = bms.GetFlattenedVertices();
    auto flatIndices = bms.GetFlattenedIndices();
    if (flatVerts.empty() || flatIndices.empty()) {
      meshIdx++;
      continue;
    }

    bool hasBoneTransform = false;
    Matrix4 boneMatrix;
    auto boneIt = meshIdxToBoneName.find(meshIdx);
    if (boneIt != meshIdxToBoneName.end()) {
      auto transIt = boneTransforms.find(boneIt->second);
      if (transIt != boneTransforms.end()) {
        hasBoneTransform = true;
        boneMatrix = transIt->second;
      }
    }

    SubMesh sub;
    sub.VertexCount = bms.Vertices.size();
    sub.IndexCount = flatIndices.size();
    sub.MaterialName = bms.Material;

    // Vertex Buffer
    m_device->CreateVertexBuffer(
        static_cast<UINT>(sub.VertexCount * sizeof(Vertex)), D3DUSAGE_WRITEONLY,
        Vertex::FVF, D3DPOOL_MANAGED, &sub.Vb, nullptr);
    if (sub.Vb) {
      void *pVoid = nullptr;
      sub.Vb->Lock(0, 0, &pVoid, 0);
      Vertex *vertexData = static_cast<Vertex *>(pVoid);
      for (int i = 0; i < sub.VertexCount; ++i) {
        Vector3 pos = Vector3(flatVerts[i * 8], flatVerts[i * 8 + 1], flatVerts[i * 8 + 2]);
        Vector3 normal = Vector3(flatVerts[i * 8 + 3], flatVerts[i * 8 + 4], flatVerts[i * 8 + 5]);

        if (hasBoneTransform) {
          pos = Vec3Transform(pos, boneMatrix);
          normal = Vec3TransformNormal(normal, boneMatrix);
        }

        vertexData[i].Pos = pos;
        vertexData[i].Normal = normal;
        vertexData[i].UV = Vector2(flatVerts[i * 8 + 6], flatVerts[i * 8 + 7]);

        model->MinBounds.x = (std::min)(model->MinBounds.x, pos.x);
        model->MinBounds.y = (std::min)(model->MinBounds.y, pos.y);
        model->MinBounds.z = (std::min)(model->MinBounds.z, pos.z);
        model->MaxBounds.x = (std::max)(model->MaxBounds.x, pos.x);
        model->MaxBounds.y = (std::max)(model->MaxBounds.y, pos.y);
        model->MaxBounds.z = (std::max)(model->MaxBounds.z, pos.z);
      }
      sub.Vb->Unlock();
    }

    // Index Buffer
    m_device->CreateIndexBuffer(
        static_cast<UINT>(sub.IndexCount * sizeof(uint32_t)),
        D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &sub.Ib, nullptr);
    if (sub.Ib) {
      void *pVoid = nullptr;
      sub.Ib->Lock(0, 0, &pVoid, 0);
      std::memcpy(pVoid, flatIndices.data(), flatIndices.size() * sizeof(uint32_t));
      sub.Ib->Unlock();
    }

    // Texture
    std::string matKey = bms.Material;
    std::transform(matKey.begin(), matKey.end(), matKey.begin(), ::tolower);
    auto texIt = materialTextureMap.find(matKey);
    if (texIt != materialTextureMap.end()) {
      sub.TexturePath = texIt->second;
    } else {
      LogMsgW(L"[MeshRenderer] Warning: Material '" + std::wstring(matKey.begin(), matKey.end()) + L"' NOT found in materialTextureMap (BSR: " + std::wstring(bsrRelPath.begin(), bsrRelPath.end()) + L")");
    }

    model->SubMeshes.push_back(sub);
    meshIdx++;
  }

  model->SubMeshCount = static_cast<int>(model->SubMeshes.size());
  model->TotalVertices = 0;
  model->TotalIndices = 0;
  for (const auto& sm : model->SubMeshes) {
    model->TotalVertices += sm.VertexCount;
    model->TotalIndices += sm.IndexCount;
  }

  ModelResource *ptr = model.get();
  m_modelCache[bsrRelPath] = std::move(model);
  return ptr;
}

void MeshRenderer::BeginBatch(const Matrix4 &view, const Matrix4 &proj, int timeOfDay, bool wireframe) {
  m_batchWireframe = wireframe;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX *>(&view));
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX *>(&proj));

  // Set lighting once for all objects
  m_device->SetRenderState(D3DRS_LIGHTING, TRUE);
  
  D3DLIGHT9 light = {};
  light.Type = D3DLIGHT_DIRECTIONAL;
  light.Direction = {-0.3f, -0.9f, -0.3f};

  if (timeOfDay == 0) {
    // Warm Sunny Day Mode
    m_device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(180, 185, 195));
    light.Diffuse = {1.0f, 0.95f, 0.85f, 1.0f};  // Warm direct sunlight
    light.Ambient = {0.4f, 0.4f, 0.45f, 1.0f};    // Sky-bounce fill ambient
  } else {
    // Midnight Night Mode
    m_device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(25, 28, 38)); // Dark midnight blue ambient
    light.Diffuse = {0.38f, 0.40f, 0.52f, 1.0f};  // Soft dim blue moonlight
    light.Ambient = {0.12f, 0.12f, 0.18f, 1.0f};  // Dark ambient moonlight shadow
  }

  m_device->SetLight(0, &light);
  m_device->LightEnable(0, TRUE);

  D3DMATERIAL9 material = {};
  material.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
  material.Ambient = {1.0f, 1.0f, 1.0f, 1.0f};
  m_device->SetMaterial(&material);

  // Set sampler states once
  m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
  m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 8);

  m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

  m_device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  m_device->SetRenderState(D3DRS_ALPHAREF, 0x80);
  m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
  m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
  if (wireframe) {
    m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  }

  m_device->SetFVF(Vertex::FVF);
  m_batchActive = true;
}

void MeshRenderer::DrawModel(const std::string &bsrRelPath, const Vector3 &pos,
                              float yaw, bool isSelected, bool isHidden,
                              const Matrix4 &view, const Matrix4 &proj) {
  ModelResource *model = PreloadModel(bsrRelPath);
  if (!model) return;

  // If not in batch mode, set up render state per-model (legacy path)
  if (!m_batchActive) {
    BeginBatch(view, proj);
  }

  if (isHidden) {
    m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 128, 128, 128));
  }

  Matrix4 worldRot = MatrixRotationY(yaw);
  Matrix4 worldTrans = MatrixTranslation(pos.x, pos.y, pos.z);
  Matrix4 world = worldRot * worldTrans;
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX *>(&world));

  for (const auto &sub : model->SubMeshes) {
    if (!sub.Vb || !sub.Ib) continue;

    if (!sub.TexturePath.empty() && !isHidden) {
      Texture *tex = m_texManager->GetTexture(sub.TexturePath, false);
      m_device->SetTexture(0, tex ? tex->pTexture : nullptr);
      if (!tex) {
        LogMsgW(L"[MeshRenderer] Warning: Failed to load submesh texture: " + sub.TexturePath);
      }
    } else {
      m_device->SetTexture(0, nullptr);
    }

    m_device->SetStreamSource(0, sub.Vb, 0, sizeof(Vertex));
    m_device->SetIndices(sub.Ib);
    m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, sub.VertexCount, 0, sub.IndexCount / 3);
  }

  if (isHidden) {
    m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  }

  if (isSelected) {
    DWORD fillCol = D3DCOLOR_ARGB(255, 235, 175, 38);
    m_device->LightEnable(0, FALSE);
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_device->SetTexture(0, nullptr);
    m_device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);

    // Bypass texture stages and output vertex diffuse color
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    struct SelectionVertex {
        float x, y, z;
        DWORD color;
    };

    Vector3 bMin = model->MinBounds;
    Vector3 bMax = model->MaxBounds;
    float padding = 0.5f;
    bMin.x -= padding; bMin.y -= padding; bMin.z -= padding;
    bMax.x += padding; bMax.y += padding; bMax.z += padding;

    SelectionVertex boxVerts[24] = {
        { bMin.x, bMin.y, bMin.z, fillCol }, { bMax.x, bMin.y, bMin.z, fillCol },
        { bMax.x, bMin.y, bMin.z, fillCol }, { bMax.x, bMin.y, bMax.z, fillCol },
        { bMax.x, bMin.y, bMax.z, fillCol }, { bMin.x, bMin.y, bMax.z, fillCol },
        { bMin.x, bMin.y, bMax.z, fillCol }, { bMin.x, bMin.y, bMin.z, fillCol },
        { bMin.x, bMax.y, bMin.z, fillCol }, { bMax.x, bMax.y, bMin.z, fillCol },
        { bMax.x, bMax.y, bMin.z, fillCol }, { bMax.x, bMax.y, bMax.z, fillCol },
        { bMax.x, bMax.y, bMax.z, fillCol }, { bMin.x, bMax.y, bMax.z, fillCol },
        { bMin.x, bMax.y, bMax.z, fillCol }, { bMin.x, bMax.y, bMin.z, fillCol },
        { bMin.x, bMin.y, bMin.z, fillCol }, { bMin.x, bMax.y, bMin.z, fillCol },
        { bMax.x, bMin.y, bMin.z, fillCol }, { bMax.x, bMax.y, bMin.z, fillCol },
        { bMax.x, bMin.y, bMax.z, fillCol }, { bMax.x, bMax.y, bMax.z, fillCol },
        { bMin.x, bMin.y, bMax.z, fillCol }, { bMin.x, bMax.y, bMax.z, fillCol }
    };

    m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
    m_device->DrawPrimitiveUP(D3DPT_LINELIST, 12, boxVerts, sizeof(SelectionVertex));
    m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);

    m_device->SetRenderState(D3DRS_LIGHTING, TRUE);
    m_device->LightEnable(0, TRUE);
    m_device->SetFVF(Vertex::FVF);

    // Restore default texture stage states
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  }

  if (!m_batchActive) {
    EndBatch();
  }
}

void MeshRenderer::EndBatch() {
  if (m_batchWireframe) {
    m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    m_batchWireframe = false;
  }
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  m_device->SetTexture(0, nullptr);
  m_batchActive = false;
}
