#include "MeshRenderer.h"
#include "Core/Log.h"
#include "Core/Utils.h"
#include "Parsers/ParserHelpers.h"
#include "Rendering/BitmapSpriteAnimator.h"
#include "Rendering/RenderStateGuard.h"
#include "assets/AssetPathResolver.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_set>

static inline Vector3 Vec3Transform(const Vector3& v, const Matrix4& m) {
    Vector3 out;
    out.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    out.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    out.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    return out;
}

static inline Vector3 Vec3TransformNormal(const Vector3& n, const Matrix4& m) {
    Vector3 out;
    out.x = n.x * m.m[0][0] + n.y * m.m[1][0] + n.z * m.m[2][0];
    out.y = n.x * m.m[0][1] + n.y * m.m[1][1] + n.z * m.m[2][1];
    out.z = n.x * m.m[0][2] + n.y * m.m[1][2] + n.z * m.m[2][2];
    return Vec3Normalize(out);
}

static void ApplyWaterTextureScroll(LPDIRECT3DDEVICE9 device, float vScroll) {
  D3DMATRIX texMat{};
  texMat._11 = 1.0f;
  texMat._22 = 1.0f;
  texMat._33 = 1.0f;
  texMat._44 = 1.0f;
  texMat._32 = vScroll;
  device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
  device->SetTransform(D3DTS_TEXTURE0, &texMat);
}

static void ApplyUVCellTransform(LPDIRECT3DDEVICE9 device, const UVCellTransform& cell) {
  // 2D affine UV transform: u' = ScaleU*u + OffsetU, v' = ScaleV*v + OffsetV.
  // For D3DTTFF_COUNT2, _11/_22 hold U/V scale and _31/_32 hold U/V translation.
  D3DMATRIX texMat{};
  texMat._11 = cell.ScaleU;
  texMat._22 = cell.ScaleV;
  texMat._33 = 1.0f;
  texMat._44 = 1.0f;
  texMat._31 = cell.OffsetU;
  texMat._32 = cell.OffsetV;
  device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
  device->SetTransform(D3DTS_TEXTURE0, &texMat);
}

static void ResetWaterTextureScroll(LPDIRECT3DDEVICE9 device) {
  D3DMATRIX identity{};
  identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
  device->SetTransform(D3DTS_TEXTURE0, &identity);
  device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

bool MeshRenderer::IsWaterSubMesh(const std::string& materialName, const std::wstring& texturePath,
                                  const std::string& meshSourcePath) {
  auto hasToken = [](std::string s, const char* tok) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s.find(tok) != std::string::npos;
  };
  auto hasTokenW = [](std::wstring s, const wchar_t* tok) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s.find(tok) != std::wstring::npos;
  };

  if (hasToken(meshSourcePath, "waterfall") || hasToken(meshSourcePath, "waterline") ||
      hasToken(meshSourcePath, "bigwaterfall")) {
    return true;
  }
  if (hasToken(materialName, "waterfall") || hasToken(materialName, "waterline") ||
      hasToken(materialName, "bigwaterfall")) {
    return true;
  }
  if (hasTokenW(texturePath, L"waterfall") || hasTokenW(texturePath, L"waterline") ||
      hasTokenW(texturePath, L"bigwaterfall")) {
    return true;
  }
  if (hasTokenW(texturePath, L"/water/water1") || hasTokenW(texturePath, L"\\water\\water1")) {
    return true;
  }
  return false;
}

bool MeshRenderer::IsAlphaBlendHintSubMesh(const std::string& materialName, const std::wstring& texturePath,
                                           const std::string& meshSourcePath) {
  auto hasToken = [](std::string s, const char* tok) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s.find(tok) != std::string::npos;
  };
  auto hasTokenW = [](std::wstring s, const wchar_t* tok) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s.find(tok) != std::wstring::npos;
  };

  return hasToken(meshSourcePath, "grs_water") ||
         hasToken(materialName, "grs_water") ||
         hasTokenW(texturePath, L"grs_water");
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

std::wstring MeshRenderer::ResolveAssetPath(const std::string& relPath, const std::string& baseRelPath) const {
  sro::AssetPathResolver resolver;
  resolver.SetClientPath(m_clientPath);
  if (!baseRelPath.empty())
    return resolver.ResolveRelativeToBase(baseRelPath, relPath);
  return resolver.ResolveRelativePath(relPath);
}

std::wstring MeshRenderer::FindCaseInsensitiveFile(const std::wstring& dir, const std::wstring& fileName) {
  if (dir.empty() || fileName.empty()) return {};
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!fs::exists(dir, ec)) return {};
  for (const auto& entry : fs::directory_iterator(dir, ec)) {
    if (ec || !entry.is_regular_file()) continue;
    if (_wcsicmp(entry.path().filename().wstring().c_str(), fileName.c_str()) == 0)
      return entry.path().wstring();
  }
  return {};
}

std::wstring MeshRenderer::ResolveTexturePath(const std::string& texRelPath, const std::wstring& mtrlDir,
                                              const std::string& mtrlRelPath,
                                              const std::string& bsrRelPath) const {
  if (texRelPath.empty()) return {};

  std::string norm = texRelPath;
  std::replace(norm.begin(), norm.end(), '\\', '/');
  std::wstring wTexName(norm.begin(), norm.end());

  const auto pickExisting = [](const std::wstring& p) -> std::wstring {
    if (!p.empty() && FileExists(p)) return p;
    return {};
  };

  if (std::wstring p = pickExisting(mtrlDir + L"/" + wTexName); !p.empty()) return p;
  if (std::wstring p = pickExisting(ResolveAssetPath(norm, bsrRelPath)); !p.empty()) return p;
  if (std::wstring p = pickExisting(ResolveAssetPath(norm, mtrlRelPath)); !p.empty()) return p;
  if (std::wstring p = pickExisting(ResolveAssetPath(norm)); !p.empty()) return p;

  const std::wstring fileName = std::filesystem::path(wTexName).filename().wstring();
  if (std::wstring p = FindCaseInsensitiveFile(mtrlDir, fileName); !p.empty()) return p;

  const std::wstring bsrDir = std::filesystem::path(ResolveAssetPath(bsrRelPath)).parent_path().wstring();
  if (std::wstring p = FindCaseInsensitiveFile(bsrDir, fileName); !p.empty()) return p;

  return ResolveAssetPath(norm, bsrRelPath);
}

std::string MeshRenderer::AssetCacheKey(const std::string& relPath, const std::string& baseRelPath) {
  if (baseRelPath.empty()) return relPath;
  return baseRelPath + "|" + relPath;
}

const BsrResource* MeshRenderer::GetBsrResource(const std::string& bsrRelPath) {
  auto it = m_bsrCache.find(bsrRelPath);
  if (it != m_bsrCache.end()) return it->second.get();
  auto bsr = std::make_unique<BsrResource>();
  if (!bsr->Read(ResolveAssetPath(bsrRelPath))) return nullptr;
  const BsrResource* ptr = bsr.get();
  m_bsrCache[bsrRelPath] = std::move(bsr);
  return ptr;
}

const BmsNavMesh* MeshRenderer::LoadCollisionNavMeshByPath(const std::string& meshRelPath,
                                                            const std::string& baseRelPath) {
  if (meshRelPath.empty()) return nullptr;
  const std::string key = AssetCacheKey(meshRelPath, baseRelPath);
  auto it = m_collisionMeshCache.find(key);
  if (it != m_collisionMeshCache.end()) return it->second ? &it->second->NavMesh : nullptr;

  auto bms = std::make_unique<BmsMesh>();
  std::wstring meshPath = ResolveAssetPath(meshRelPath, baseRelPath);
  if (!bms->Read(meshPath) || !bms->NavMesh.HasData) {
    m_collisionMeshCache[key] = nullptr;
    return nullptr;
  }
  const BmsNavMesh* ptr = &bms->NavMesh;
  m_collisionMeshCache[key] = std::move(bms);
  return ptr;
}

const BmsNavMesh* MeshRenderer::LoadCollisionNavMesh(const std::string& bsrRelPath,
                                                      std::string* outResolvedPath) {
  if (outResolvedPath) outResolvedPath->clear();
  const BsrResource* bsr = GetBsrResource(bsrRelPath);
  if (!bsr) return nullptr;

  std::string colPath = bsr->CollisionPath;
  // Compound collision: BSR.CollisionPath may point to a .cpd, whose
  // collisionResourcePath is the actual collision .bms.
  if (!colPath.empty()) {
    auto endsWith = [](const std::string& s, const char* suf) {
      size_t n = strlen(suf);
      return s.size() >= n && _stricmp(s.c_str() + s.size() - n, suf) == 0;
    };
    if (endsWith(colPath, ".cpd")) {
      CpdResource cpd;
      if (cpd.Read(ResolveAssetPath(colPath, bsrRelPath)) && !cpd.CollisionPath.empty()) {
        colPath = cpd.CollisionPath;
        const BmsNavMesh* nav = LoadCollisionNavMeshByPath(colPath, bsrRelPath);
        if (nav && outResolvedPath) *outResolvedPath = colPath;
        return nav;
      }
      return nullptr;
    }
    const BmsNavMesh* nav = LoadCollisionNavMeshByPath(colPath, bsrRelPath);
    if (nav && outResolvedPath) *outResolvedPath = colPath;
    return nav;
  }

  // Some compound BSRs (path is a .cpd) carry no CollisionPath themselves; their
  // parts may. Walk the CPD resource list for the first part with collision.
  auto endsWith = [](const std::string& s, const char* suf) {
    size_t n = strlen(suf);
    return s.size() >= n && _stricmp(s.c_str() + s.size() - n, suf) == 0;
  };
  if (endsWith(bsrRelPath, ".cpd")) {
    CpdResource cpd;
    if (cpd.Read(ResolveAssetPath(bsrRelPath))) {
      for (const auto& part : cpd.ResourcePaths) {
        const BsrResource* partBsr = GetBsrResource(part);
        if (partBsr && !partBsr->CollisionPath.empty()) {
          const BmsNavMesh* nav = LoadCollisionNavMeshByPath(partBsr->CollisionPath, part);
          if (nav) {
            if (outResolvedPath) *outResolvedPath = partBsr->CollisionPath;
            return nav;
          }
        }
      }
    }
  }
  return nullptr;
}

SkeletonResource* MeshRenderer::LoadSkeleton(const std::string& skelRelPath, const std::string& baseRelPath) {
  if (skelRelPath.empty()) return nullptr;
  const std::string key = AssetCacheKey(skelRelPath, baseRelPath);
  auto it = m_skeletonCache.find(key);
  if (it != m_skeletonCache.end()) return it->second.get();
  auto skel = std::make_unique<SkeletonResource>();
  if (!skel->LoadFromFile(ResolveAssetPath(skelRelPath, baseRelPath))) return nullptr;
  SkeletonResource* ptr = skel.get();
  m_skeletonCache[key] = std::move(skel);
  return ptr;
}

BanResource* MeshRenderer::LoadBanClip(const std::string& banRelPath, const std::string& baseRelPath) {
  if (banRelPath.empty()) return nullptr;
  const std::string key = AssetCacheKey(banRelPath, baseRelPath);
  auto it = m_banCache.find(key);
  if (it != m_banCache.end()) return it->second.get();
  auto ban = std::make_unique<BanResource>();
  if (!ban->Read(ResolveAssetPath(banRelPath, baseRelPath))) return nullptr;
  BanResource* ptr = ban.get();
  m_banCache[key] = std::move(ban);
  return ptr;
}

void MeshRenderer::UploadSubMeshVertices(SubMesh& sub, const Matrix4* boneMatrix) {
  if (!sub.Vb || sub.BindVertices.empty()) return;
  void* pVoid = nullptr;
  if (FAILED(sub.Vb->Lock(0, 0, &pVoid, 0))) return;
  Vertex* vertexData = static_cast<Vertex*>(pVoid);
  for (size_t i = 0; i < sub.BindVertices.size(); ++i) {
    Vertex v = sub.BindVertices[i];
    if (boneMatrix) {
      v.Pos = Vec3Transform(v.Pos, *boneMatrix);
      v.Normal = Vec3TransformNormal(v.Normal, *boneMatrix);
    }
    vertexData[i] = v;
  }
  sub.Vb->Unlock();
}


static std::string SkinBoneNameKey(const std::string& name) {
  std::string key = name;
  std::transform(key.begin(), key.end(), key.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return key;
}

static Matrix4 SkinMatrixForBone(const std::string& boneName,
                                 const std::map<std::string, Matrix4>& skinMats,
                                 std::unordered_set<std::string>& warnedMissing) {
  auto it = skinMats.find(boneName);
  if (it != skinMats.end()) return it->second;
  it = skinMats.find(SkinBoneNameKey(boneName));
  if (it != skinMats.end()) return it->second;
  if (warnedMissing.insert(boneName).second) {
    LogMsg("[MeshRenderer] Warning: missing skin bone '" + boneName + "'");
  }
  return Matrix4::Identity();
}

void MeshRenderer::UploadSkinnedSubMeshVertices(SubMesh& sub,
    const std::map<std::string, Matrix4>& skinMats) {
  if (!sub.Vb || sub.BindVertices.empty() || sub.SkinWeights.size() != sub.BindVertices.size()) return;

  static std::unordered_set<std::string> warnedMissingBones;

  void* pVoid = nullptr;
  if (FAILED(sub.Vb->Lock(0, 0, &pVoid, 0))) return;
  Vertex* vertexData = static_cast<Vertex*>(pVoid);

  for (size_t i = 0; i < sub.BindVertices.size(); ++i) {
    const Vertex& bind = sub.BindVertices[i];
    const BmsVertexSkin& skin = sub.SkinWeights[i];
    Vertex v = bind;

    Vector3 pos = bind.Pos;
    Vector3 normal = bind.Normal;

    if (skin.weight1 > 0.f && skin.boneIdx1 < sub.SkinBoneNames.size()) {
      const Matrix4 m1 = SkinMatrixForBone(sub.SkinBoneNames[skin.boneIdx1], skinMats, warnedMissingBones);
      pos = pos + (Vec3Transform(bind.Pos, m1) - bind.Pos) * skin.weight1;
      normal = normal + (Vec3TransformNormal(bind.Normal, m1) - bind.Normal) * skin.weight1;
    }
    if (skin.weight2 > 0.f && skin.boneIdx2 < sub.SkinBoneNames.size()) {
      const Matrix4 m2 = SkinMatrixForBone(sub.SkinBoneNames[skin.boneIdx2], skinMats, warnedMissingBones);
      pos = pos + (Vec3Transform(bind.Pos, m2) - bind.Pos) * skin.weight2;
      normal = normal + (Vec3TransformNormal(bind.Normal, m2) - bind.Normal) * skin.weight2;
    }

    v.Pos = pos;
    v.Normal = Vec3Normalize(normal);
    vertexData[i] = v;
  }
  sub.Vb->Unlock();
}

void MeshRenderer::UploadRigidSubMeshVertices(SubMesh& sub, const Matrix4& skinMat) {
  UploadSubMeshVertices(sub, &skinMat);
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
  if (src.SubMeshes.empty() && src.EffectPaths.empty()) return;

  if (dst.SubMeshes.empty() && !src.SubMeshes.empty()) {
    dst.MinBounds = src.MinBounds;
    dst.MaxBounds = src.MaxBounds;
  } else if (!src.SubMeshes.empty()) {
    dst.MinBounds.x = (std::min)(dst.MinBounds.x, src.MinBounds.x);
    dst.MinBounds.y = (std::min)(dst.MinBounds.y, src.MinBounds.y);
    dst.MinBounds.z = (std::min)(dst.MinBounds.z, src.MinBounds.z);
    dst.MaxBounds.x = (std::max)(dst.MaxBounds.x, src.MaxBounds.x);
    dst.MaxBounds.y = (std::max)(dst.MaxBounds.y, src.MaxBounds.y);
    dst.MaxBounds.z = (std::max)(dst.MaxBounds.z, src.MaxBounds.z);
  }

  dst.SubMeshes.insert(dst.SubMeshes.end(), src.SubMeshes.begin(), src.SubMeshes.end());
  dst.EffectPaths.insert(dst.EffectPaths.end(), src.EffectPaths.begin(), src.EffectPaths.end());
  dst.ParticleAttachments.insert(dst.ParticleAttachments.end(), src.ParticleAttachments.begin(), src.ParticleAttachments.end());
  dst.SubMeshCount = static_cast<int>(dst.SubMeshes.size());
  dst.TotalVertices = 0;
  dst.TotalIndices = 0;
  for (const auto& sm : dst.SubMeshes) {
    dst.TotalVertices += sm.VertexCount;
    dst.TotalIndices += sm.IndexCount;
  }
}

MeshRenderer::ModelResource *MeshRenderer::LoadCompoundModelResource(const std::string &cpdRelPath) {
  std::wstring cpdPath = ResolveAssetPath(cpdRelPath);

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

  if (merged->SubMeshes.empty() && merged->EffectPaths.empty()) {
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

  std::wstring bsrPath = ResolveAssetPath(bsrRelPath);

  BsrResource bsr;
  if (!bsr.Read(bsrPath)) {
    LogMsgW(L"[MeshRenderer] Error: Failed to read BSR file: " + bsrPath);
    m_modelCache[bsrRelPath] = nullptr;
    return nullptr;
  }
  m_bsrCache[bsrRelPath] = std::make_unique<BsrResource>(bsr);

  auto model = std::make_unique<ModelResource>();
  model->SourceBsrPath = bsrRelPath;
  model->SkeletonPath = bsr.SkeletonPath;
  model->EffectPaths = bsr.EffectPaths;
  model->ParticleAttachments = bsr.ParticleAttachments;

  // Load materials
  std::map<std::string, std::wstring> materialTextureMap;
  static std::unordered_set<std::string> s_loggedMissingTextures;
  for (const auto &mtrlRelPath : bsr.MaterialPaths) {
    if (mtrlRelPath.empty()) continue;
    std::wstring mtrlPath = ResolveAssetPath(mtrlRelPath, bsrRelPath);

    BmtParser bmt;
    if (bmt.Read(mtrlPath)) {
      std::wstring mtrlDir = std::filesystem::path(mtrlPath).parent_path().wstring();
      for (const auto &entry : bmt.Entries) {
        if (!entry.TextureName.empty()) {
          std::string nameKey = entry.Name;
          std::transform(nameKey.begin(), nameKey.end(), nameKey.begin(), ::tolower);
          materialTextureMap[nameKey] = ResolveTexturePath(entry.TextureName, mtrlDir, mtrlRelPath, bsrRelPath);
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

  SkeletonResource* skeleton = nullptr;
  if (!bsr.SkeletonPath.empty())
    skeleton = LoadSkeleton(bsr.SkeletonPath, bsrRelPath);

  // Load meshes
  uint32_t meshIdx = 0;
  for (const auto &meshRelPath : bsr.MeshPaths) {
    if (meshRelPath.empty()) continue;
    std::wstring meshPath = ResolveAssetPath(meshRelPath, bsrRelPath);

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
    if (boneIt != meshIdxToBoneName.end() && skeleton) {
      if (const SkeletonBone* skelBone = skeleton->FindBoneCaseInsensitive(boneIt->second)) {
        hasBoneTransform = true;
        boneMatrix = skelBone->BindPose;
      }
    }

    SubMesh sub;
    sub.VertexCount = bms.Vertices.size();
    sub.IndexCount = flatIndices.size();
    sub.MaterialName = bms.Material;

    std::string matLower = sub.MaterialName;
    std::transform(matLower.begin(), matLower.end(), matLower.begin(), ::tolower);
    sub.IsAlphaBlend = (matLower.find("alpha") != std::string::npos || matLower.find("_al") != std::string::npos);
    sub.MeshSourcePath = meshRelPath;

    if (!bms.Skin.empty() && !bms.SkinBones.empty()) {
      sub.HasVertexSkin = true;
      sub.Skinned = true;
      sub.SkinBoneNames = bms.SkinBones;
      if (skeleton) {
        for (std::string& skinBoneName : sub.SkinBoneNames) {
          if (const SkeletonBone* skelBone = skeleton->FindBoneCaseInsensitive(skinBoneName))
            skinBoneName = skelBone->Name;
        }
      }
      sub.SkinWeights = bms.Skin;
    } else if (boneIt != meshIdxToBoneName.end() && !boneIt->second.empty() && hasBoneTransform) {
      sub.HasRigidBone = true;
      sub.Skinned = true;
      sub.BoneName = boneIt->second;
      if (skeleton) {
        if (const SkeletonBone* skelBone = skeleton->FindBoneCaseInsensitive(sub.BoneName))
          sub.BoneName = skelBone->Name;
      }
      sub.BindBoneMatrix = boneMatrix;
    }

    sub.BindVertices.resize(sub.VertexCount);
    for (int i = 0; i < sub.VertexCount; ++i) {
      sub.BindVertices[i].Pos = Vector3(flatVerts[i * 8], flatVerts[i * 8 + 1], flatVerts[i * 8 + 2]);
      sub.BindVertices[i].Normal = Vector3(flatVerts[i * 8 + 3], flatVerts[i * 8 + 4], flatVerts[i * 8 + 5]);
      sub.BindVertices[i].UV = Vector2(flatVerts[i * 8 + 6], flatVerts[i * 8 + 7]);
    }

    // Vertex Buffer
    m_device->CreateVertexBuffer(
        static_cast<UINT>(sub.VertexCount * sizeof(Vertex)), D3DUSAGE_WRITEONLY,
        Vertex::FVF, D3DPOOL_MANAGED, &sub.Vb, nullptr);
    if (sub.Vb) {
      if (sub.HasRigidBone) {
        UploadSubMeshVertices(sub, &sub.BindBoneMatrix);
        sub.RigidBaked = true;
      } else {
        UploadSubMeshVertices(sub, nullptr);
      }

      {
        for (int i = 0; i < sub.VertexCount; ++i) {
          Vector3 transformed = sub.BindVertices[i].Pos;
          if (sub.HasRigidBone)
            transformed = Vec3Transform(transformed, sub.BindBoneMatrix);
          model->MinBounds.x = (std::min)(model->MinBounds.x, transformed.x);
          model->MinBounds.y = (std::min)(model->MinBounds.y, transformed.y);
          model->MinBounds.z = (std::min)(model->MinBounds.z, transformed.z);
          model->MaxBounds.x = (std::max)(model->MaxBounds.x, transformed.x);
          model->MaxBounds.y = (std::max)(model->MaxBounds.y, transformed.y);
          model->MaxBounds.z = (std::max)(model->MaxBounds.z, transformed.z);
        }
      }
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

    std::string matKey = bms.Material;
    std::transform(matKey.begin(), matKey.end(), matKey.begin(), ::tolower);
    auto texIt = materialTextureMap.find(matKey);
    if (texIt != materialTextureMap.end() && FileExists(texIt->second)) {
      sub.TexturePath = texIt->second;
    } else {
      ++model->MissingTextureCount;
      const std::string logKey = bsrRelPath + "|" + std::to_string(meshIdx) + "|" + matKey;
      if (s_loggedMissingTextures.insert(logKey).second) {
        const std::wstring attempted = (texIt != materialTextureMap.end()) ? texIt->second : L"(none)";
        LogMsgW(L"[MeshRenderer] Missing texture submesh=" + std::to_wstring(meshIdx)
            + L" material=" + std::wstring(matKey.begin(), matKey.end())
            + L" bsr=" + std::wstring(bsrRelPath.begin(), bsrRelPath.end())
            + L" path=" + attempted);
      }
    }

    if (IsAlphaBlendHintSubMesh(sub.MaterialName, sub.TexturePath, sub.MeshSourcePath)) {
      sub.IsAlphaBlend = true;
    }

    if (IsWaterSubMesh(sub.MaterialName, sub.TexturePath, sub.MeshSourcePath)) {
      sub.IsWaterEffect = true;
      sub.IsAlphaBlend = true;
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

  if (model->MinBounds.x > model->MaxBounds.x) {
    model->MinBounds = Vector3(99999.0f, 99999.0f, 99999.0f);
    model->MaxBounds = Vector3(-99999.0f, -99999.0f, -99999.0f);
    for (const auto& sm : model->SubMeshes) {
      for (int i = 0; i < sm.VertexCount; ++i) {
        Vector3 transformed = sm.BindVertices[i].Pos;
        if (sm.HasRigidBone)
          transformed = Vec3Transform(transformed, sm.BindBoneMatrix);
        model->MinBounds.x = (std::min)(model->MinBounds.x, transformed.x);
        model->MinBounds.y = (std::min)(model->MinBounds.y, transformed.y);
        model->MinBounds.z = (std::min)(model->MinBounds.z, transformed.z);
        model->MaxBounds.x = (std::max)(model->MaxBounds.x, transformed.x);
        model->MaxBounds.y = (std::max)(model->MaxBounds.y, transformed.y);
        model->MaxBounds.z = (std::max)(model->MaxBounds.z, transformed.z);
      }
    }
  }

  ModelResource *ptr = model.get();
  m_modelCache[bsrRelPath] = std::move(model);
  return ptr;
}

void MeshRenderer::BeginBatch(const Matrix4 &view, const Matrix4 &proj, int timeOfDay, bool wireframe, bool doubleSided) {
  m_batchWireframe = wireframe;
  m_batchDoubleSided = doubleSided;
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
  m_device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 2);

  m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

  m_device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  m_device->SetRenderState(D3DRS_ALPHAREF, doubleSided ? 0x01 : 0x80);
  m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
  m_device->SetRenderState(D3DRS_CULLMODE, doubleSided ? D3DCULL_NONE : D3DCULL_CCW);
  if (doubleSided) {
    m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
  }
  if (wireframe) {
    m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  }

  m_device->SetFVF(Vertex::FVF);
  m_batchActive = true;
}

void MeshRenderer::DrawModel(const std::string &bsrRelPath, const Vector3 &pos,
                              float yaw, bool isSelected, bool isHidden,
                              const Matrix4 &view, const Matrix4 &proj, ModelResource *preloaded) {
  ModelResource *model = preloaded ? preloaded : PreloadModel(bsrRelPath);
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

  const float timeSeconds = static_cast<float>(GetTickCount() % 100000) * 0.001f;
  const float waterVScroll = -timeSeconds * 0.65f;

  auto drawSubMesh = [&](SubMesh& sub, int pass) {
    if (!sub.Vb || !sub.Ib) return;
    const bool isWater = sub.IsWaterEffect;
    const bool isAlpha = sub.IsAlphaBlend;
    if (pass == 0) {
      if (isAlpha) return;
    } else if (pass == 1) {
      if (!isAlpha || isWater) return;
    } else {
      if (!isWater) return;
    }

    bool useAdditiveAndNoLight = false;
    if (isAlpha && !isWater) {
      std::string matLower = sub.MaterialName;
      std::transform(matLower.begin(), matLower.end(), matLower.begin(), ::tolower);
      std::wstring texLower = sub.TexturePath;
      std::transform(texLower.begin(), texLower.end(), texLower.begin(), ::tolower);

      if (matLower.find("fire") != std::string::npos ||
          matLower.find("flame") != std::string::npos ||
          matLower.find("glow") != std::string::npos ||
          matLower.find("light") != std::string::npos ||
          matLower.find("deedy") != std::string::npos ||
          matLower.find("brazier") != std::string::npos ||
          matLower.find("lamp") != std::string::npos ||
          matLower.find("candle") != std::string::npos ||
          texLower.find(L"fire") != std::wstring::npos ||
          texLower.find(L"flame") != std::wstring::npos ||
          texLower.find(L"glow") != std::wstring::npos ||
          texLower.find(L"light") != std::wstring::npos ||
          texLower.find(L"deedy") != std::wstring::npos ||
          texLower.find(L"brazier") != std::wstring::npos ||
          texLower.find(L"lamp") != std::wstring::npos ||
          texLower.find(L"candle") != std::wstring::npos) {
        useAdditiveAndNoLight = true;
      }
    }

    if (sub.HasRigidBone && !sub.RigidBaked) {
      UploadSubMeshVertices(sub, &sub.BindBoneMatrix);
    }

    Texture* subTex = nullptr;
    if (!sub.TexturePath.empty() && !isHidden) {
      subTex = m_texManager->GetTexture(sub.TexturePath, false);
      m_device->SetTexture(0, subTex ? subTex->pTexture : nullptr);
      if (!subTex) {
        LogMsgW(L"[MeshRenderer] Warning: Failed to load submesh texture: " + sub.TexturePath);
      }
    } else {
      m_device->SetTexture(0, nullptr);
    }

    // For fire submeshes (e.g. a brazier body that uses a spritesheet texture
    // like fire001.ddj), step through the atlas by remapping the mesh's [0,1]
    // UVs onto the current cell, and clamp so adjacent cells don't bleed
    // through bilinear filtering. For non-atlas textures the cell transform
    // is the identity and only the CLAMP address mode change applies.
    UVCellTransform fireCell = UVCellTransform::Identity();
    bool fireCellIsolated = false;
    if (useAdditiveAndNoLight && subTex) {
      const SpriteAtlasLayout layout = InferAtlasLayoutFromTexture(subTex);
      if (layout.FrameCount > 1) {
        FrameTextureSlideData slide;
        slide.Left = Vector3(static_cast<float>(layout.Cols),
                             static_cast<float>(layout.Rows), 1.0f);
        slide.Valid = true;
        const float timeSeconds = static_cast<float>(GetTickCount() % 100000) * 0.001f;
        fireCell = SampleSpriteAnimAtTime(slide, timeSeconds).Cell;
        fireCellIsolated = true;
      }
    }

    if (isWater) {
      m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
      m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
      m_device->LightEnable(0, FALSE);
      m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
      ApplyWaterTextureScroll(m_device, waterVScroll);
    } else if (isAlpha) {
      m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      if (useAdditiveAndNoLight) {
        m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        m_device->LightEnable(0, FALSE);
        m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
        // Fire/light alpha meshes: clamp and remap UVs to the active cell of
        // the spritesheet so we animate by stepping cells rather than sliding
        // the whole atlas through the quad.
        m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        if (fireCellIsolated) {
          ApplyUVCellTransform(m_device, fireCell);
        } else {
          ResetWaterTextureScroll(m_device);
        }
      } else {
        m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      }
      m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
      m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }

    m_device->SetStreamSource(0, sub.Vb, 0, sizeof(Vertex));
    m_device->SetIndices(sub.Ib);
    m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, sub.VertexCount, 0, sub.IndexCount / 3);

    if (isWater) {
      ResetWaterTextureScroll(m_device);
      m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
      m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
      m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
      m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
      m_device->SetRenderState(D3DRS_LIGHTING, TRUE);
      m_device->LightEnable(0, TRUE);
    } else if (isAlpha) {
      if (useAdditiveAndNoLight) {
        ResetWaterTextureScroll(m_device);
        m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
      }
      m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
      m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
      m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
      m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
      m_device->SetRenderState(D3DRS_LIGHTING, TRUE);
      m_device->LightEnable(0, TRUE);
    }
  };

  for (int pass = 0; pass < 3; ++pass) {
    for (auto &sub : model->SubMeshes) {
      drawSubMesh(sub, pass);
    }
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

void MeshRenderer::DrawModelAnimated(const std::string& bsrRelPath, const Vector3& pos, float yaw, bool wireframe,
                                     const SkeletonAnimator* animator, const SkeletonResource* skeleton,
                                     const Matrix4& view, const Matrix4& proj) {
  ModelResource* model = PreloadModel(bsrRelPath);
  if (!model) return;

  if (!m_batchActive) {
    BeginBatch(view, proj, 0, wireframe);
  }

  Matrix4 worldRot = MatrixRotationY(yaw);
  Matrix4 worldTrans = MatrixTranslation(pos.x, pos.y, pos.z);
  Matrix4 world = worldRot * worldTrans;
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));

  for (auto& sub : model->SubMeshes) {
    if (!sub.Vb || !sub.Ib) continue;

    bool useAdditiveAndNoLight = false;
    if (sub.IsAlphaBlend) {
      std::string matLower = sub.MaterialName;
      std::transform(matLower.begin(), matLower.end(), matLower.begin(), ::tolower);
      std::wstring texLower = sub.TexturePath;
      std::transform(texLower.begin(), texLower.end(), texLower.begin(), ::towlower);

      if (matLower.find("fire") != std::string::npos ||
          matLower.find("flame") != std::string::npos ||
          matLower.find("glow") != std::string::npos ||
          matLower.find("light") != std::string::npos ||
          matLower.find("deedy") != std::string::npos ||
          matLower.find("brazier") != std::string::npos ||
          matLower.find("lamp") != std::string::npos ||
          matLower.find("candle") != std::string::npos ||
          texLower.find(L"fire") != std::wstring::npos ||
          texLower.find(L"flame") != std::wstring::npos ||
          texLower.find(L"glow") != std::wstring::npos ||
          texLower.find(L"light") != std::wstring::npos ||
          texLower.find(L"deedy") != std::wstring::npos ||
          texLower.find(L"brazier") != std::wstring::npos ||
          texLower.find(L"lamp") != std::wstring::npos ||
          texLower.find(L"candle") != std::wstring::npos) {
        useAdditiveAndNoLight = true;
      }
    }

    if (animator && skeleton) {
      const std::map<std::string, Matrix4>& skinMats = animator->SkinMatrices();
      if (sub.HasVertexSkin) {
        UploadSkinnedSubMeshVertices(sub, skinMats);
      } else if (sub.HasRigidBone && !sub.BoneName.empty()) {
        const std::map<std::string, Matrix4>& boneMats = animator->BoneMatrices();
        auto it = boneMats.find(sub.BoneName);
        if (it == boneMats.end())
          it = boneMats.find(SkinBoneNameKey(sub.BoneName));
        if (it != boneMats.end()) {
          UploadRigidSubMeshVertices(sub, it->second);
        }
      }
    }

    if (!sub.TexturePath.empty()) {
      Texture* tex = m_texManager->GetTexture(sub.TexturePath, false);
      m_device->SetTexture(0, tex ? tex->pTexture : nullptr);
    } else {
      m_device->SetTexture(0, nullptr);
    }

    // Alpha submeshes use standard alpha blending
    if (sub.IsAlphaBlend) {
      m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      if (useAdditiveAndNoLight) {
        m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        m_device->LightEnable(0, FALSE);
        m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
      } else {
        m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      }
      m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
      m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }

    m_device->SetStreamSource(0, sub.Vb, 0, sizeof(Vertex));
    m_device->SetIndices(sub.Ib);
    m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, sub.VertexCount, 0, sub.IndexCount / 3);

    // Restore render states after alpha submesh
    if (sub.IsAlphaBlend) {
      m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
      m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
      m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
      m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
      if (useAdditiveAndNoLight) {
        m_device->SetRenderState(D3DRS_LIGHTING, TRUE);
        m_device->LightEnable(0, TRUE);
      }
    }
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
  if (m_batchDoubleSided) {
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    m_batchDoubleSided = false;
  }
  ApplyOpaqueBaseline(m_device);
  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  m_batchActive = false;
}
