#include "EffectRenderer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include "assets/AssetPathResolver.h"
#include "core/FileSystem.h"
#include "core/Log.h"
#include "Parsers/EfpParser.h"

void EffectRenderer::Initialize(LPDIRECT3DDEVICE9 device, TextureManager* texManager,
                                const std::wstring& clientPath) {
    m_device = device;
    m_texManager = texManager;
    m_clientPath = clientPath;
    m_effectCache.clear();
    m_effectMeshCache.clear();
    m_effectGpuCache.clear();
    m_missingEffectMeshes.clear();
    m_particleSystem.Clear();
    m_lastTickMs = GetTickCount();
}

void EffectRenderer::InvalidateTextureCache() {
    // Texture pointers are owned by TextureManager; nothing to retain here.
}

void EffectRenderer::OnDeviceLost() {
    m_particleSystem.OnDeviceLost();
    m_effectGpuCache.clear();
    m_effectPassActive = false;
    m_passStateGuard.reset();
}

bool EffectRenderer::EndsWith(const std::string& text, const char* suffix) {
    const size_t len = std::strlen(suffix);
    return text.size() >= len && text.compare(text.size() - len, len, suffix) == 0;
}

std::string EffectRenderer::LowerPath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    std::transform(path.begin(), path.end(), path.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return path;
}

Vector3 EffectRenderer::RotateOffsetByYaw(const Vector3& offset, float yaw) {
    const float cosY = cosf(yaw);
    const float sinY = sinf(yaw);
    return Vector3(
        offset.x * cosY - offset.z * sinY,
        offset.y,
        offset.x * sinY + offset.z * cosY);
}

float EffectRenderer::ComputeEffectYaw(float placementYaw, const Vector3& localRotation) {
    return placementYaw + localRotation.y;
}

Vector3 EffectRenderer::ResolveEffectPosition(const Vector3& pos, const Vector3& localAnchor, float yaw,
                                              const Vector3& localRotation) {
    if (localAnchor.x == 0.0f && localAnchor.y == 0.0f && localAnchor.z == 0.0f &&
        localRotation.x == 0.0f && localRotation.y == 0.0f && localRotation.z == 0.0f) {
        return pos;
    }

    const float totalYaw = yaw + localRotation.y;
    Vector3 rotated = RotateOffsetByYaw(localAnchor, totalYaw);

    if (localRotation.x != 0.0f) {
        const float cosX = cosf(localRotation.x);
        const float sinX = sinf(localRotation.x);
        const float y = rotated.y * cosX - rotated.z * sinX;
        const float z = rotated.y * sinX + rotated.z * cosX;
        rotated.y = y;
        rotated.z = z;
    }
    if (localRotation.z != 0.0f) {
        const float cosZ = cosf(localRotation.z);
        const float sinZ = sinf(localRotation.z);
        const float x = rotated.x * cosZ - rotated.y * sinZ;
        const float y = rotated.x * sinZ + rotated.y * cosZ;
        rotated.x = x;
        rotated.y = y;
    }

    return pos + rotated;
}

ParticleSystem::InstanceKey EffectRenderer::MakeInstanceKey(const std::string& effectPath, const Vector3& pos,
                                                            const Vector3& anchor) {
    return ParticleSystem::MakeKey(LowerPath(effectPath), pos, anchor);
}

float EffectRenderer::AdvanceDeltaTime() {
    const DWORD now = GetTickCount();
    float dt = 0.016f;
    if (m_lastTickMs != 0) {
        dt = static_cast<float>(now - m_lastTickMs) * 0.001f;
        dt = std::clamp(dt, 0.001f, 0.1f);
    }
    m_lastTickMs = now;
    return dt;
}

bool EffectRenderer::IsLightningParticlePath(const std::string& path) {
    const std::string lower = LowerPath(path);
    return lower.find("lightning") != std::string::npos ||
           lower.find("thunder") != std::string::npos ||
           lower.find("electric") != std::string::npos ||
           lower.find("bolt") != std::string::npos;
}

bool EffectRenderer::IsWaterLikePath(const std::string& path) {
    const std::string lower = LowerPath(path);
    return lower.find("water") != std::string::npos ||
           lower.find("fountain") != std::string::npos ||
           lower.find("fall") != std::string::npos;
}

bool EffectRenderer::IsSplashTexturePath(const std::string& path) {
    const std::string lower = LowerPath(path);
    return lower.find("splash") != std::string::npos ||
           lower.find("spray") != std::string::npos ||
           lower.find("foam") != std::string::npos ||
           lower.find("drop") != std::string::npos;
}

bool EffectRenderer::IsFireLikePath(const std::string& path) {
    const std::string lower = LowerPath(path);
    return lower.find("fire") != std::string::npos ||
           lower.find("flame") != std::string::npos ||
           lower.find("torch") != std::string::npos ||
           lower.find("brazier") != std::string::npos ||
           lower.find("burn") != std::string::npos;
}

bool EffectRenderer::IsLightLikePath(const std::string& path) {
    const std::string lower = LowerPath(path);
    return lower.find("lamp") != std::string::npos ||
           lower.find("lantern") != std::string::npos ||
           lower.find("light") != std::string::npos ||
           lower.find("candle") != std::string::npos ||
           lower.find("orange") != std::string::npos;
}

bool EffectRenderer::IsSmokeLikePath(const std::string& path) {
    const std::string lower = LowerPath(path);
    return lower.find("smoke") != std::string::npos ||
           lower.find("mist") != std::string::npos ||
           lower.find("fog") != std::string::npos ||
           lower.find("steam") != std::string::npos ||
           lower.find("glow") != std::string::npos ||
           lower.find("haze") != std::string::npos;
}

bool EffectRenderer::IsParticleLikePath(const std::string& path) {
    const std::string lower = LowerPath(path);
    return EndsWith(lower, ".efp") ||
           EndsWith(lower, ".eff") ||
           lower.find("/effect/") != std::string::npos ||
           lower.find("/particle/") != std::string::npos ||
           lower.find("spark") != std::string::npos ||
           lower.find("smoke") != std::string::npos ||
           lower.find("flame") != std::string::npos ||
           lower.find("fire") != std::string::npos ||
           lower.find("waterfall") != std::string::npos ||
           lower.find("fountain") != std::string::npos ||
           IsLightningParticlePath(path);
}

std::wstring EffectRenderer::ResolveAssetPath(const std::string& path, const std::string& basePath) const {
    sro::AssetPathResolver resolver;
    resolver.SetClientPath(m_clientPath);
    if (!basePath.empty()) {
        return resolver.ResolveRelativeToBase(basePath, path);
    }
    return resolver.ResolveRelativePath(path);
}

std::wstring EffectRenderer::FindCaseInsensitiveFile(const std::wstring& dir, const std::wstring& fileName) {
    if (dir.empty() || fileName.empty()) return {};
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(dir, ec)) return {};
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec || !entry.is_regular_file()) continue;
        if (_wcsicmp(entry.path().filename().wstring().c_str(), fileName.c_str()) == 0) {
            return entry.path().wstring();
        }
    }
    return {};
}

std::wstring EffectRenderer::ResolveEffectTexturePath(const std::string& texRelPath,
                                                      const std::string& effectRelPath) const {
    if (texRelPath.empty()) return {};

    std::string norm = texRelPath;
    std::replace(norm.begin(), norm.end(), '\\', '/');
    std::wstring wTexName(norm.begin(), norm.end());

    const auto pickExisting = [](const std::wstring& p) -> std::wstring {
        if (!p.empty() && FileExists(p)) return p;
        return {};
    };

    const std::wstring effectAbs = ResolveAssetPath(effectRelPath);
    const std::wstring effectDir = std::filesystem::path(effectAbs).parent_path().wstring();

    if (std::wstring p = pickExisting(effectDir + L"/" + wTexName); !p.empty()) return p;
    if (std::wstring p = pickExisting(ResolveAssetPath(norm, effectRelPath)); !p.empty()) return p;
    if (std::wstring p = pickExisting(ResolveAssetPath(norm)); !p.empty()) return p;

    const std::wstring fileName = std::filesystem::path(wTexName).filename().wstring();
    if (std::wstring p = FindCaseInsensitiveFile(effectDir, fileName); !p.empty()) return p;

    return ResolveAssetPath(norm, effectRelPath);
}

const EfpResource* EffectRenderer::LoadEffectResource(const std::string& effectPath) {
    if (effectPath.empty()) return nullptr;
    const std::string key = LowerPath(effectPath);
    auto it = m_effectCache.find(key);
    if (it != m_effectCache.end()) return &it->second;

    EfpResource res;
    EfpParser parser;
    const std::wstring resolved = ResolveAssetPath(effectPath);
    if (!parser.Read(resolved, res)) {
        return nullptr;
    }

    auto inserted = m_effectCache.emplace(key, std::move(res));
    return &inserted.first->second;
}

const BmsMesh* EffectRenderer::LoadEffectMesh(const std::string& meshPath, const std::string& effectPath) {
    if (meshPath.empty()) return nullptr;
    const std::string key = LowerPath(effectPath + "|" + meshPath);
    auto it = m_effectMeshCache.find(key);
    if (it != m_effectMeshCache.end()) return &it->second;
    if (m_missingEffectMeshes.count(key) > 0) return nullptr;

    BmsMesh mesh;
    if (!mesh.Read(ResolveAssetPath(meshPath, effectPath)) || mesh.Vertices.empty() || mesh.Faces.empty()) {
        m_missingEffectMeshes.insert(key);
        return nullptr;
    }

    auto inserted = m_effectMeshCache.emplace(key, std::move(mesh));
    return &inserted.first->second;
}

EffectRenderer::EffectMeshGpu* EffectRenderer::LoadEffectMeshGpu(const std::string& meshPath,
                                                                 const std::string& effectPath) {
    if (meshPath.empty() || !m_device) return nullptr;
    const std::string key = LowerPath(effectPath + "|" + meshPath);
    auto it = m_effectGpuCache.find(key);
    if (it != m_effectGpuCache.end()) return it->second.get();
    if (m_missingEffectMeshes.count(key) > 0) return nullptr;

    const BmsMesh* mesh = LoadEffectMesh(meshPath, effectPath);
    if (!mesh || mesh->Vertices.empty() || mesh->Faces.empty()) return nullptr;

    auto gpu = std::make_unique<EffectMeshGpu>();
    gpu->VertexCount = static_cast<int>(mesh->Vertices.size());
    gpu->IndexCount = static_cast<int>(mesh->Faces.size() * 3);

    std::vector<EffectBillboardVertex> verts;
    verts.reserve(mesh->Vertices.size());
    for (const BmsVertex& v : mesh->Vertices) {
        verts.push_back({
            Vector3(v.X, v.Y, v.Z),
            D3DCOLOR_ARGB(255, 255, 255, 255),
            Vector2(v.U, v.V)
        });
    }

    std::vector<uint16_t> indices;
    indices.reserve(mesh->Faces.size() * 3);
    for (const BmsFace& f : mesh->Faces) {
        indices.push_back(f.A);
        indices.push_back(f.B);
        indices.push_back(f.C);
    }

    if (FAILED(m_device->CreateVertexBuffer(
            static_cast<UINT>(verts.size() * sizeof(EffectBillboardVertex)),
            D3DUSAGE_WRITEONLY, EffectBillboardVertex::FVF, D3DPOOL_MANAGED,
            &gpu->Vb, nullptr))) {
        m_missingEffectMeshes.insert(key);
        return nullptr;
    }

    void* vbData = nullptr;
    if (SUCCEEDED(gpu->Vb->Lock(0, 0, &vbData, 0))) {
        std::memcpy(vbData, verts.data(), verts.size() * sizeof(EffectBillboardVertex));
        gpu->Vb->Unlock();
    }

    if (FAILED(m_device->CreateIndexBuffer(
            static_cast<UINT>(indices.size() * sizeof(uint16_t)),
            D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED,
            &gpu->Ib, nullptr))) {
        m_missingEffectMeshes.insert(key);
        return nullptr;
    }

    void* ibData = nullptr;
    if (SUCCEEDED(gpu->Ib->Lock(0, 0, &ibData, 0))) {
        std::memcpy(ibData, indices.data(), indices.size() * sizeof(uint16_t));
        gpu->Ib->Unlock();
    }

    EffectMeshGpu* ptr = gpu.get();
    m_effectGpuCache.emplace(key, std::move(gpu));
    return ptr;
}

bool EffectRenderer::IsFountainBasinModel(const std::string& modelPath) {
    const std::string lower = LowerPath(modelPath);
    return lower.find("cj_jang_gate.bsr") != std::string::npos;
}

Texture* EffectRenderer::GetBasinWaterTexture() {
    if (!m_texManager) return nullptr;

    const float timeSeconds = static_cast<float>(GetTickCount() % 100000) * 0.001f;
    const int frame = static_cast<int>(timeSeconds * 12.0f) % 30;
    const int texId = 101 + frame;

    wchar_t fileName[32]{};
    swprintf_s(fileName, L"water%u.ddj", texId);
    const std::wstring roots[] = {
        m_clientPath + L"/Map/water",
        m_clientPath + L"/map/water",
        m_clientPath + L"/Data/shader/water",
    };
    for (const auto& root : roots) {
        const std::wstring path = root + L"/" + fileName;
        if (FileExists(path)) {
            return m_texManager->GetTexture(path, false);
        }
    }

    for (const auto& root : roots) {
        const std::wstring path = root + L"/water101.ddj";
        if (FileExists(path)) {
            return m_texManager->GetTexture(path, false);
        }
    }
    return nullptr;
}

float EffectRenderer::MeshBoundsSize(const BmsMesh& mesh) const {
    if (mesh.Vertices.empty()) return 32.0f;
    float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
    float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
    for (const BmsVertex& v : mesh.Vertices) {
        minX = (std::min)(minX, v.X);
        minY = (std::min)(minY, v.Y);
        minZ = (std::min)(minZ, v.Z);
        maxX = (std::max)(maxX, v.X);
        maxY = (std::max)(maxY, v.Y);
        maxZ = (std::max)(maxZ, v.Z);
    }
    const float sx = maxX - minX;
    const float sy = maxY - minY;
    const float sz = maxZ - minZ;
    const float maxAxis = (std::max)(sx, (std::max)(sy, sz));
    return std::clamp(maxAxis * 2.0f, 8.0f, 320.0f);
}

void EffectRenderer::MeshBillboardExtents(const BmsMesh& mesh, const std::string& viewMode,
                                         float& outWidth, float& outHeight) const {
    outWidth = 32.0f;
    outHeight = 32.0f;
    if (mesh.Vertices.empty()) return;
    float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
    float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
    for (const BmsVertex& v : mesh.Vertices) {
        minX = (std::min)(minX, v.X);
        minY = (std::min)(minY, v.Y);
        minZ = (std::min)(minZ, v.Z);
        maxX = (std::max)(maxX, v.X);
        maxY = (std::max)(maxY, v.Y);
        maxZ = (std::max)(maxZ, v.Z);
    }
    float sx = maxX - minX;
    float sy = maxY - minY;
    float sz = maxZ - minZ;
    if (sx < 0.001f) sx = 0.001f;
    if (sy < 0.001f) sy = 0.001f;
    if (sz < 0.001f) sz = 0.001f;

    if (viewMode == "ViewVBillboard") {
        outWidth = (std::max)(sx, sz);
        outHeight = sy;
    } else {
        const float maxH = (std::max)(sx, sz);
        outWidth = maxH;
        outHeight = sy;
    }
    outWidth = std::clamp(outWidth, 6.0f, 320.0f);
    outHeight = std::clamp(outHeight, 6.0f, 320.0f);
}

void EffectRenderer::TextureBillboardExtents(Texture* texture, const std::string& viewMode,
                                             float baseSize, float& outWidth, float& outHeight) const {
    float aspect = 1.0f;
    if (texture && texture->Width > 0 && texture->Height > 0) {
        aspect = static_cast<float>(texture->Width) / static_cast<float>(texture->Height);
        if (aspect < 0.05f) aspect = 0.05f;
        if (aspect > 20.0f) aspect = 20.0f;
    }

    if (viewMode == "ViewVBillboard") {
        if (aspect >= 1.0f) {
            outWidth = baseSize;
            outHeight = baseSize / aspect;
        } else {
            outWidth = baseSize * aspect;
            outHeight = baseSize;
        }
    } else if (aspect >= 1.0f) {
        outWidth = baseSize;
        outHeight = baseSize / aspect;
    } else {
        outWidth = baseSize * aspect;
        outHeight = baseSize;
    }
    outWidth = std::clamp(outWidth, 6.0f, 320.0f);
    outHeight = std::clamp(outHeight, 6.0f, 320.0f);
}

void EffectRenderer::BeginEffectPass() {
    if (!m_device || m_effectPassActive) return;
    m_effectPassActive = true;
    m_passStateGuard = std::make_unique<RenderStateGuard>(m_device);
    m_particleSystem.BeginDrawFrame();
    m_lastFramePerf = {};

    m_device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    ApplyEffectTextureStages(m_device);
}

void EffectRenderer::EndEffectPass() {
    if (!m_device || !m_effectPassActive) return;
    m_effectPassActive = false;

    ResetTextureScroll(m_device);
    m_passStateGuard.reset();
    ApplyOpaqueBaseline(m_device);
}

void EffectRenderer::ApplyTextureScroll(LPDIRECT3DDEVICE9 device, const UVCellTransform& cell) {
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

void EffectRenderer::ResetTextureScroll(LPDIRECT3DDEVICE9 device) {
    D3DMATRIX identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
    device->SetTransform(D3DTS_TEXTURE0, &identity);
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

D3DCOLOR EffectRenderer::ParticleColorForPath(const std::string& path, bool selected) {
    if (selected) return D3DCOLOR_ARGB(255, 255, 220, 90);
    const std::string lower = LowerPath(path);
    if (IsLightningParticlePath(lower)) return D3DCOLOR_ARGB(235, 150, 220, 255);
    if (IsWaterLikePath(lower)) return D3DCOLOR_ARGB(210, 80, 190, 255);
    if (lower.find("fire") != std::string::npos || lower.find("flame") != std::string::npos) {
        return D3DCOLOR_ARGB(220, 255, 135, 55);
    }
    if (lower.find("smoke") != std::string::npos) {
        return D3DCOLOR_ARGB(190, 170, 175, 180);
    }
    return D3DCOLOR_ARGB(210, 120, 210, 255);
}

void EffectRenderer::DrawParticleMarker(const Vector3& pos, const std::string& path, bool selected,
                                        const Matrix4& view, const Matrix4& proj) {
    if (!m_device) return;

    std::vector<EffectVertex> verts;
    verts.reserve(80);
    const bool lightning = IsLightningParticlePath(path);
    const DWORD color = ParticleColorForPath(path, selected);
    const float t = static_cast<float>(GetTickCount() % 100000) * 0.001f;

    auto addLine = [&](float x0, float y0, float z0, float x1, float y1, float z1, DWORD c) {
        verts.push_back({x0, y0, z0, c});
        verts.push_back({x1, y1, z1, c});
    };

    if (lightning) {
        Vector3 p = pos + Vector3(0.0f, 12.0f, 0.0f);
        for (int i = 0; i < 7; ++i) {
            const float nextY = pos.y + 35.0f + i * 18.0f;
            const float jitterA = sinf(t * 11.0f + i * 1.7f) * 12.0f;
            const float jitterB = cosf(t * 9.0f + i * 1.3f) * 12.0f;
            Vector3 n(pos.x + jitterA, nextY, pos.z + jitterB);
            addLine(p.x, p.y, p.z, n.x, n.y, n.z, color);
            p = n;
        }
    } else {
        const float radius = selected ? 34.0f : 26.0f;
        for (int i = 0; i < 12; ++i) {
            const float a0 = (static_cast<float>(i) / 12.0f) * 6.2831853f + t * 0.8f;
            const float a1 = a0 + 0.35f;
            const float y0 = pos.y + 8.0f + i * 4.0f;
            const float y1 = y0 + 10.0f;
            addLine(pos.x + cosf(a0) * radius, y0, pos.z + sinf(a0) * radius,
                    pos.x + cosf(a1) * radius, y1, pos.z + sinf(a1) * radius, color);
        }
    }

    if (verts.empty()) return;

    Matrix4 identity = Matrix4::Identity();
    m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&identity));
    m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
    m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
    m_device->SetFVF(EffectVertex::FVF);
    m_device->SetTexture(0, nullptr);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, lightning ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
    m_device->DrawPrimitiveUP(D3DPT_LINELIST, static_cast<UINT>(verts.size() / 2),
                              verts.data(), sizeof(EffectVertex));
}

void EffectRenderer::ComputeBillboardAxes(const std::string& viewMode, const Vector3& pos,
                                          const Vector3& cameraPos, float yaw,
                                          Vector3& outRight, Vector3& outUp) {
    if (viewMode == "ViewVBillboard") {
        const float cosY = cosf(yaw);
        const float sinY = sinf(yaw);
        outRight = Vector3(cosY, 0.0f, -sinY);
        outUp = Vector3(0.0f, 1.0f, 0.0f);
        return;
    }

    Vector3 facing = cameraPos - pos;
    if (viewMode == "ViewYBillboard") {
        facing.y = 0.0f;
    }

    facing = Vec3Normalize(facing);
    if (facing.x == 0.0f && facing.y == 0.0f && facing.z == 0.0f) {
        facing = Vector3(0.0f, 0.0f, 1.0f);
    }

    if (viewMode == "ViewYBillboard") {
        outUp = Vector3(0.0f, 1.0f, 0.0f);
        outRight = Vec3Normalize(Vec3Cross(outUp, facing));
        if (outRight.x == 0.0f && outRight.y == 0.0f && outRight.z == 0.0f) {
            outRight = Vector3(1.0f, 0.0f, 0.0f);
        }
        return;
    }

    outRight = Vec3Normalize(Vec3Cross(Vector3(0.0f, 1.0f, 0.0f), facing));
    if (outRight.x == 0.0f && outRight.y == 0.0f && outRight.z == 0.0f) {
        outRight = Vector3(1.0f, 0.0f, 0.0f);
    }
    outUp = Vec3Normalize(Vec3Cross(facing, outRight));
}

void EffectRenderer::DrawEffectBillboard(Texture* texture, const Vector3& pos, const Vector3& cameraPos,
                                         const Matrix4& view, const Matrix4& proj, float width, float height,
                                         const SpriteAnimSample& animSample, DWORD color, bool additive,
                                         const std::string& viewMode, float yaw, int srcBlend, int dstBlend) {
    if (!texture || !texture->pTexture || !m_device) return;

    auto drawPass = [&](const UVCellTransform& uvCell, BYTE alpha) {
        if (alpha == 0) return;
        const DWORD passColor = (color & 0x00FFFFFF) | (static_cast<DWORD>(alpha) << 24);

        Vector3 right, up;
        ComputeBillboardAxes(viewMode, pos, cameraPos, yaw, right, up);
        const float halfW = width * 0.5f;
        const float halfH = height * 0.5f;

        const float u0 = uvCell.OffsetU;
        const float v0 = uvCell.OffsetV;
        const float u1 = uvCell.OffsetU + uvCell.ScaleU;
        const float v1 = uvCell.OffsetV + uvCell.ScaleV;

        EffectBillboardVertex verts[6] = {
            {pos - right * halfW - up * halfH, passColor, Vector2(u0, v1)},
            {pos - right * halfW + up * halfH, passColor, Vector2(u0, v0)},
            {pos + right * halfW - up * halfH, passColor, Vector2(u1, v1)},
            {pos + right * halfW - up * halfH, passColor, Vector2(u1, v1)},
            {pos - right * halfW + up * halfH, passColor, Vector2(u0, v0)},
            {pos + right * halfW + up * halfH, passColor, Vector2(u1, v0)},
        };

        Matrix4 identity = Matrix4::Identity();
        m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&identity));
        m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
        m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
        m_device->SetFVF(EffectBillboardVertex::FVF);
        m_device->SetTexture(0, texture->pTexture);
        ApplyEffectBlendState(m_device, srcBlend, dstBlend, additive);
        ResetTextureScroll(m_device);
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, verts, sizeof(EffectBillboardVertex));
    };

    const BYTE baseAlpha = static_cast<BYTE>((color >> 24) & 0xFF);
    if (animSample.Blend > 0.001f && animSample.Blend < 0.999f) {
        drawPass(animSample.Cell, static_cast<BYTE>(static_cast<float>(baseAlpha) * (1.0f - animSample.Blend)));
        drawPass(animSample.NextCell, static_cast<BYTE>(static_cast<float>(baseAlpha) * animSample.Blend));
    } else {
        drawPass(animSample.Cell, baseAlpha);
    }
}

bool EffectRenderer::DrawEffectMeshWorld(const EfpRenderItem& item, const std::string& effectPath,
                                         Texture* texture, const Vector3& pos, float yaw,
                                         const Matrix4& view, const Matrix4& proj,
                                         const SpriteAnimSample& animSample, DWORD color, bool additive) {
    if (!texture || !texture->pTexture || !m_device) return false;

    EffectMeshGpu* gpu = LoadEffectMeshGpu(item.MeshPath, effectPath);
    if (!gpu || !gpu->Vb || !gpu->Ib || gpu->IndexCount < 3) return false;

    Matrix4 world = MatrixRotationY(yaw) * MatrixTranslation(pos.x, pos.y, pos.z);
    m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&world));
    m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
    m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
    m_device->SetFVF(EffectBillboardVertex::FVF);
    m_device->SetTexture(0, texture->pTexture);
    m_device->SetRenderState(D3DRS_TEXTUREFACTOR, color);
    ApplyEffectBlendState(m_device, item.SrcBlend, item.DstBlend, additive);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    ApplyTextureScroll(m_device, animSample.Cell);
    m_device->SetStreamSource(0, gpu->Vb, 0, sizeof(EffectBillboardVertex));
    m_device->SetIndices(gpu->Ib);
    m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, gpu->VertexCount, 0, gpu->IndexCount / 3);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    ResetTextureScroll(m_device);
    return true;
}

bool EffectRenderer::ShouldUseMeshForItem(const EfpRenderItem& item) {
    return EfpItemUsesMeshGeometry(item);
}

bool EffectRenderer::ShouldUseMesh(const EfpRenderItem& item) const {
    return ShouldUseMeshForItem(item);
}

bool EffectRenderer::ItemUsesBillboard(const EfpRenderItem& item) const {
    if (ShouldUseMesh(item)) return false;
    if (item.ViewMode == "ViewNone") return false;
    return true;
}

std::string EffectRenderer::ResolveViewMode(const EfpRenderItem& item, bool waterLike,
                                            bool fireLike, bool smokeLike) const {
    if (fireLike || smokeLike) {
        return "ViewYBillboard";
    }
    if (!item.ViewMode.empty() && item.ViewMode != "ViewBillboard") {
        return item.ViewMode;
    }
    if (waterLike && !item.MeshPath.empty()) {
        return "ViewVBillboard";
    }
    return item.ViewMode.empty() ? "ViewBillboard" : item.ViewMode;
}

int EffectRenderer::ScoreRenderItem(const EfpRenderItem& item, const std::string& effectPath,
                                    bool fireLike, bool waterLike, bool smokeLike) {
    if (item.TexturePaths.empty()) return -1;

    int score = 0;
    const std::string texLower = LowerPath(item.TexturePaths.front());
    const std::string meshLower = LowerPath(item.MeshPath);

    if (item.Shape == "RenderMesh" && !item.MeshPath.empty()) score += 100;
    if (!item.MeshPath.empty()) score += 40;

    if (fireLike) {
        if (IsFireLikePath(texLower) || IsFireLikePath(meshLower)) score += 80;
        if (texLower.find("spark") != std::string::npos) score -= 40;
    }
    if (waterLike) {
        if (IsWaterLikePath(texLower)) score += 60;
        if (IsSplashTexturePath(texLower)) score += 30;
    }
    if (smokeLike) {
        if (IsSmokeLikePath(texLower)) score += 70;
    }

    if (const BmsMesh* mesh = LoadEffectMesh(item.MeshPath, effectPath)) {
        score += static_cast<int>(MeshBoundsSize(*mesh));
    }

    return score;
}

void EffectRenderer::CollectRenderItems(const EfpResource& effect, const std::string& effectPath,
                                        bool fireLike, bool waterLike, bool smokeLike,
                                        std::vector<const EfpRenderItem*>& out) {
    out.clear();
    if (effect.RenderItems.empty()) return;

    int maxLayers = kMaxEffectLayers;
    if (waterLike) maxLayers = kMaxWaterEffectLayers;
    else if (smokeLike) maxLayers = kMaxSmokeEffectLayers;

    std::vector<std::pair<int, const EfpRenderItem*>> ranked;
    ranked.reserve(effect.RenderItems.size());
    for (const EfpRenderItem& item : effect.RenderItems) {
        const int score = ScoreRenderItem(item, effectPath, fireLike, waterLike, smokeLike);
        if (score >= 0) {
            ranked.push_back({score, &item});
        }
    }

    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::set<std::string> seenFamilies;
    for (const auto& [score, item] : ranked) {
        (void)score;
        if (static_cast<int>(out.size()) >= maxLayers) break;
        if (item->TexturePaths.empty()) continue;

        std::string family = LowerPath(item->TexturePaths.front());
        const size_t slash = family.find_last_of('/');
        if (slash != std::string::npos) family = family.substr(slash + 1);
        while (!family.empty() && std::isdigit(static_cast<unsigned char>(family.back()))) {
            family.pop_back();
        }
        while (!family.empty() && (family.back() == '_' || family.back() == '-')) {
            family.pop_back();
        }
        if (!seenFamilies.insert(family).second) continue;
        out.push_back(item);
    }
}

const EfpObjectDef* EffectRenderer::PickPrimaryParticleObject(const EfpResource& effect, bool fireLike,
                                                              bool waterLike, bool smokeLike) const {
    const EfpObjectDef* best = nullptr;
    int bestScore = -1;
    for (const EfpObjectDef& obj : effect.Objects) {
        if (obj.Render.TexturePaths.empty()) continue;
        if (!obj.Render.TextureSlide.Valid && !obj.Render.TextureSlide.HasKeys()) continue;

        int score = 10;
        const std::string texLower = LowerPath(obj.Render.TexturePaths.front());
        if (fireLike && IsFireLikePath(texLower)) score += 80;
        if (waterLike && IsWaterLikePath(texLower)) score += 80;
        if (smokeLike && IsSmokeLikePath(texLower)) score += 60;
        if (obj.Emitter.Valid) score += 40;
        if (obj.Render.TextureSlide.HasKeys()) score += static_cast<int>(obj.Render.TextureSlide.Keys.size());

        if (score > bestScore) {
            bestScore = score;
            best = &obj;
        }
    }
    return best;
}

bool EffectRenderer::DrawParticleObjects(const EfpObjectDef& primary, const std::string& effectPath,
                                         const Vector3& pos, const Vector3& cameraPos,
                                         const Matrix4& view, const Matrix4& proj, float yaw, bool selected,
                                         bool fireLike, bool waterLike, bool smokeLike, float dt,
                                         const CameraFrustum* frustum) {
    const ParticleSystem::InstanceKey key = MakeInstanceKey(effectPath, pos, pos);
    m_particleSystem.UpdateInstance(key, primary, pos, yaw, dt, fireLike, waterLike, smokeLike,
                                    cameraPos, frustum, selected);

    DWORD color = selected
        ? D3DCOLOR_ARGB(255, 255, 225, 95)
        : D3DCOLOR_ARGB(230, 255, 205, 80);
    if (!selected) {
        if (waterLike) color = D3DCOLOR_ARGB(220, 180, 225, 255);
        else if (smokeLike) color = D3DCOLOR_ARGB(170, 180, 185, 190);
    }

    const bool additive = fireLike || smokeLike || waterLike || IsLightLikePath(effectPath);
    ParticleSystem::DrawContext ctx;
    ctx.Device = m_device;
    ctx.TexManager = m_texManager;
    ctx.CameraPos = &cameraPos;
    ctx.View = &view;
    ctx.Proj = &proj;
    ctx.Frustum = frustum;
    ctx.MaxDrawDistance = kEffectMaxDistance;
    ctx.ResolveTexture = [this](const std::string& texRel, const std::string& base) {
        return ResolveEffectTexturePath(texRel, base);
    };
    ctx.ComputeAxes = [](const std::string& viewMode, const Vector3& p, const Vector3& cam, float y,
                         Vector3& right, Vector3& up) {
        EffectRenderer::ComputeBillboardAxes(viewMode, p, cam, y, right, up);
    };
    ctx.ResetTexTransform = [](LPDIRECT3DDEVICE9 dev) { EffectRenderer::ResetTextureScroll(dev); };

    return m_particleSystem.DrawInstance(key, ctx, effectPath, yaw, color, additive,
                                         primary.Render.SrcBlend, primary.Render.DstBlend);
}

bool EffectRenderer::DrawEffectLayer(const EfpRenderItem& item, const std::string& effectPath,
                                     const Vector3& pos, const Vector3& cameraPos,
                                     const Matrix4& view, const Matrix4& proj, float yaw, bool selected,
                                     bool fireLike, bool waterLike, bool smokeLike, float timeSeconds) {
    if (item.TexturePaths.empty() || !m_texManager) return false;

    const std::string& texRel = item.TexturePaths.front();
    const std::wstring texPath = ResolveEffectTexturePath(texRel, effectPath);
    Texture* tex = m_texManager->GetTexture(texPath, false);
    if (!tex || !tex->pTexture) return false;

    const std::string texLower = LowerPath(texRel);
    const bool lightLike = IsLightLikePath(effectPath) || IsLightLikePath(texLower);
    const bool additive = fireLike || lightLike || smokeLike || IsLightningParticlePath(effectPath) ||
                          waterLike;

    SpriteAnimSample animSample;
    if (item.SlideData.Valid || item.SlideData.HasKeys()) {
        animSample = SampleSpriteAnimAtTime(item.SlideData, timeSeconds);
    } else if (item.TextureSlide || fireLike || waterLike) {
        FrameTextureSlideData fallback;
        const SpriteAtlasLayout layout = InferAtlasLayoutFromTexture(tex);
        fallback.Left = Vector3(static_cast<float>(layout.Cols),
                                static_cast<float>(layout.Rows), 1.0f);
        fallback.Valid = layout.FrameCount > 1;
        animSample = SampleSpriteAnimAtTime(fallback, timeSeconds);
    } else {
        UVCellTransform scroll = UVCellTransform::Identity();
        scroll.OffsetU = timeSeconds * 0.08f;
        scroll.OffsetV = waterLike ? -timeSeconds * 0.65f : timeSeconds * 0.12f;
        animSample.Cell = scroll;
    }

    DWORD color = selected
        ? D3DCOLOR_ARGB(255, 255, 225, 95)
        : D3DCOLOR_ARGB(additive ? 235 : 200, 255, 255, 255);
    if (!selected) {
        if (fireLike || lightLike) {
            color = D3DCOLOR_ARGB(230, 255, 205, 80);
        } else if (smokeLike) {
            color = D3DCOLOR_ARGB(170, 180, 185, 190);
        } else if (waterLike) {
            color = D3DCOLOR_ARGB(220, 180, 225, 255);
        }
    }

    const std::string viewMode = ResolveViewMode(item, waterLike, fireLike, smokeLike);

    float baseSize = 32.0f;
    if (fireLike) baseSize = 80.0f;
    else if (smokeLike) baseSize = 56.0f;
    else if (waterLike) baseSize = 48.0f;
    else if (lightLike) baseSize = 40.0f;

    float width = baseSize;
    float height = baseSize;
    bool hasMeshExtents = false;
    if (!item.MeshPath.empty()) {
        if (const BmsMesh* mesh = LoadEffectMesh(item.MeshPath, effectPath)) {
            MeshBillboardExtents(*mesh, viewMode, width, height);
            hasMeshExtents = true;
            baseSize = (std::max)(baseSize, (std::max)(width, height));
        }
    }
    if (!hasMeshExtents) {
        TextureBillboardExtents(tex, viewMode, baseSize, width, height);
    }

    if (fireLike) {
        const float maxW = (std::max)(width, height);
        width = maxW * 0.55f * 1.4f;
        height = maxW * 1.4f;
    } else if (smokeLike) {
        const float maxW = (std::max)(width, height);
        width = maxW * 1.15f;
        height = maxW * 1.1f * 1.15f;
    }

    bool drew = false;
    if (ShouldUseMesh(item)) {
        drew = DrawEffectMeshWorld(item, effectPath, tex, pos, yaw, view, proj, animSample, color, additive);
    } else if (ItemUsesBillboard(item)) {
        DrawEffectBillboard(tex, pos, cameraPos, view, proj, width, height, animSample, color, additive,
                            viewMode, yaw, item.SrcBlend, item.DstBlend);
        drew = true;
    } else if (!item.MeshPath.empty()) {
        drew = DrawEffectMeshWorld(item, effectPath, tex, pos, yaw, view, proj, animSample, color, additive);
    } else {
        DrawEffectBillboard(tex, pos, cameraPos, view, proj, width, height, animSample, color, additive,
                            viewMode, yaw, item.SrcBlend, item.DstBlend);
        drew = true;
    }

    return drew;
}

void EffectRenderer::DrawFountainBasinDisc(const Vector3& pos, float yaw, const Vector3& minBounds,
                                           const Vector3& maxBounds, const Matrix4& view,
                                           const Matrix4& proj, float timeSeconds) {
    if (!m_device || minBounds.x > maxBounds.x) return;

    Texture* waterTex = GetBasinWaterTexture();
    if (!waterTex || !waterTex->pTexture) return;

    const float cx = (minBounds.x + maxBounds.x) * 0.5f;
    const float cz = (minBounds.z + maxBounds.z) * 0.5f;
    const float hx = (maxBounds.x - minBounds.x) * 0.42f;
    const float hz = (maxBounds.z - minBounds.z) * 0.42f;
    const float y = minBounds.y + 0.15f;

    const float cosY = cosf(yaw);
    const float sinY = sinf(yaw);
    auto rotateXZ = [&](float lx, float lz) {
        return Vector3(
            pos.x + lx * cosY - lz * sinY,
            pos.y + y,
            pos.z + lx * sinY + lz * cosY);
    };

    const DWORD color = D3DCOLOR_ARGB(190, 80, 170, 230);
    const Vector3 p0 = rotateXZ(-hx, -hz);
    const Vector3 p1 = rotateXZ(-hx, hz);
    const Vector3 p2 = rotateXZ(hx, -hz);
    const Vector3 p3 = rotateXZ(hx, hz);

    EffectBillboardVertex verts[6] = {
        {p0, color, Vector2(0.0f, 0.0f)},
        {p1, color, Vector2(0.0f, 1.0f)},
        {p2, color, Vector2(1.0f, 0.0f)},
        {p2, color, Vector2(1.0f, 0.0f)},
        {p1, color, Vector2(0.0f, 1.0f)},
        {p3, color, Vector2(1.0f, 1.0f)},
    };

    Matrix4 identity = Matrix4::Identity();
    m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&identity));
    m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&view));
    m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&proj));
    m_device->SetFVF(EffectBillboardVertex::FVF);
    m_device->SetTexture(0, waterTex->pTexture);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    const float scroll = timeSeconds * 0.05f;
    UVCellTransform basinCell;
    basinCell.OffsetU = scroll;
    basinCell.OffsetV = scroll * 0.75f;
    ApplyTextureScroll(m_device, basinCell);
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, verts, sizeof(EffectBillboardVertex));
    ResetTextureScroll(m_device);
}

bool EffectRenderer::DrawEffectResource(const std::string& effectPath, const Vector3& pos,
                                        const Vector3& cameraPos, const Matrix4& view, const Matrix4& proj,
                                        float yaw, bool selected, const CameraFrustum* frustum) {
    const EfpResource* effect = LoadEffectResource(effectPath);
    if (!effect || !m_texManager) return false;
    if (effect->RenderItems.empty() && effect->TexturePaths.empty()) return false;

    const std::string lower = LowerPath(effectPath);
    const bool fireLike = IsFireLikePath(lower);
    const bool smokeLike = IsSmokeLikePath(lower);
    const bool waterfall = IsWaterLikePath(lower);
    const float timeSeconds = static_cast<float>(GetTickCount() % 100000) * 0.001f;

    const EfpObjectDef* primary = nullptr;
    if (!effect->Objects.empty()) {
        primary = PickPrimaryParticleObject(*effect, fireLike, waterfall, smokeLike);
    }

    bool drewParticles = false;
    if (primary) {
        drewParticles = DrawParticleObjects(*primary, effectPath, pos, cameraPos, view, proj, yaw, selected,
                                            fireLike, waterfall, smokeLike, m_frameDt, frustum);
    }

    if (!effect->RenderItems.empty()) {
        std::vector<const EfpRenderItem*> layers;
        CollectRenderItems(*effect, effectPath, fireLike, waterfall, smokeLike, layers);

        bool drewAny = drewParticles;
        int complementLayers = 0;
        for (const EfpRenderItem* layer : layers) {
            if (drewParticles && primary && !primary->Render.TexturePaths.empty() &&
                !layer->TexturePaths.empty() &&
                LowerPath(layer->TexturePaths.front()) ==
                    LowerPath(primary->Render.TexturePaths.front())) {
                continue;
            }
            if (drewParticles && complementLayers >= 2) break;
            if (drewParticles) ++complementLayers;

            drewAny |= DrawEffectLayer(*layer, effectPath, pos, cameraPos, view, proj, yaw, selected,
                                       fireLike, waterfall, smokeLike, timeSeconds);
        }
        return drewAny;
    }

    const std::string& texRel = effect->TexturePaths.front();
    const std::wstring texPath = ResolveEffectTexturePath(texRel, effectPath);
    Texture* tex = m_texManager->GetTexture(texPath, false);
    if (!tex || !tex->pTexture) return false;

    EfpRenderItem fallback;
    fallback.Shape = "RenderPlate";
    fallback.TexturePaths.push_back(texRel);
    fallback.TextureSlide = effect->HasTextureSlide;
    fallback.ViewMode = fireLike || smokeLike ? "ViewYBillboard" : "ViewBillboard";
    return DrawEffectLayer(fallback, effectPath, pos, cameraPos, view, proj, yaw, selected,
                           fireLike, waterfall, smokeLike, timeSeconds);
}

void EffectRenderer::DrawPlacementEffects(const std::vector<EffectDrawRequest>& requests,
                                          const Vector3& cameraPos,
                                          const Matrix4& view, const Matrix4& proj,
                                          const CameraFrustum* frustum) {
    if (!m_device || requests.empty()) return;

    const float timeSeconds = static_cast<float>(GetTickCount() % 100000) * 0.001f;
    m_frameDt = AdvanceDeltaTime();
    BeginEffectPass();

    std::vector<const EffectDrawRequest*> sorted;
    sorted.reserve(requests.size());
    for (const EffectDrawRequest& req : requests) {
        sorted.push_back(&req);
    }
    std::sort(sorted.begin(), sorted.end(),
              [&cameraPos](const EffectDrawRequest* a, const EffectDrawRequest* b) {
                  auto distSq = [&cameraPos](const EffectDrawRequest* r) {
                      const float dx = r->Pos.x - cameraPos.x;
                      const float dy = r->Pos.y - cameraPos.y;
                      const float dz = r->Pos.z - cameraPos.z;
                      return dx * dx + dy * dy + dz * dz;
                  };
                  return distSq(a) > distSq(b);
              });

    for (const EffectDrawRequest* reqPtr : sorted) {
        const EffectDrawRequest& req = *reqPtr;
        const float dx = req.Pos.x - cameraPos.x;
        const float dy = req.Pos.y - cameraPos.y;
        const float dz = req.Pos.z - cameraPos.z;
        const float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > kEffectMaxDistance * kEffectMaxDistance) continue;

        const float effectYaw = ComputeEffectYaw(req.Yaw, req.LocalAnchorRotation);
        const Vector3 effectPos = ResolveEffectPosition(req.Pos, req.LocalAnchorOffset, req.Yaw,
                                                        req.LocalAnchorRotation);
        const bool effectFireLike = IsFireLikePath(req.Path);
        if (frustum && !req.Selected) {
            const float r = req.DrawFountainBasin ? 180.0f : 96.0f;
            const float yExtent = effectFireLike ? 80.0f : r;
            const float yBase = effectFireLike ? 12.0f : r;
            if (!frustum->IsBoxVisible(
                    Vector3(effectPos.x - r, effectPos.y - yBase, effectPos.z - r),
                    Vector3(effectPos.x + r, effectPos.y + yExtent, effectPos.z + r))) {
                continue;
            }
        }

        if (req.DrawFountainBasin && req.ModelMinBounds.x <= req.ModelMaxBounds.x) {
            DrawFountainBasinDisc(req.Pos, req.Yaw, req.ModelMinBounds, req.ModelMaxBounds,
                                  view, proj, timeSeconds);
        }

        if (req.Path.empty()) continue;

        if (!DrawEffectResource(req.Path, effectPos, cameraPos, view, proj, effectYaw, req.Selected, frustum)) {
            if (req.Selected) {
                DrawParticleMarker(effectPos, req.Path, req.Selected, view, proj);
            }
        }
    }

    ParticleSystem::DrawContext flushCtx;
    flushCtx.Device = m_device;
    flushCtx.View = &view;
    flushCtx.Proj = &proj;
    flushCtx.ResetTexTransform = [](LPDIRECT3DDEVICE9 dev) { EffectRenderer::ResetTextureScroll(dev); };
    m_particleSystem.FlushBatches(flushCtx);

    m_lastFramePerf.EffectRequests = static_cast<int>(requests.size());
    m_lastFramePerf.ActiveParticles = m_particleSystem.GetLastActiveParticleCount();
    m_lastFramePerf.ParticleDrawCalls = m_particleSystem.GetLastParticleDrawCalls();

    EndEffectPass();
}
