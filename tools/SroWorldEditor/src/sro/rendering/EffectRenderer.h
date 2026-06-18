#pragma once

#include <d3d9.h>

#include <map>

#include <memory>

#include <set>

#include <string>

#include <vector>

#include "Core/Math.h"

#include "Parsers/BmsParser.h"

#include "Parsers/EfpParser.h"

#include "Rendering/BitmapSpriteAnimator.h"

#include "Rendering/Camera.h"

#include "Rendering/ParticleSystem.h"

#include "Rendering/RenderStateGuard.h"

#include "Rendering/Texture.h"



class EffectRenderer {

public:

    struct EffectDrawRequest {

        Vector3 Pos;

        std::string Path;

        float Yaw = 0.0f;

        bool Selected = false;

        Vector3 LocalAnchorOffset = Vector3(0.0f, 0.0f, 0.0f);

        Vector3 LocalAnchorRotation = Vector3(0.0f, 0.0f, 0.0f);

        bool DrawFountainBasin = false;

        Vector3 ModelMinBounds = Vector3(0.0f, 0.0f, 0.0f);

        Vector3 ModelMaxBounds = Vector3(0.0f, 0.0f, 0.0f);

    };



    EffectRenderer() = default;



    void Initialize(LPDIRECT3DDEVICE9 device, TextureManager* texManager, const std::wstring& clientPath);

    void SetClientPath(const std::wstring& clientPath) { m_clientPath = clientPath; }

    void OnDeviceLost();



    void DrawPlacementEffects(const std::vector<EffectDrawRequest>& requests,

                              const Vector3& cameraPos,

                              const Matrix4& view, const Matrix4& proj,

                              const CameraFrustum* frustum = nullptr);



    void InvalidateTextureCache();



    static bool IsParticleLikePath(const std::string& path);

    static bool IsLightningParticlePath(const std::string& path);

    static bool IsWaterLikePath(const std::string& path);

    static bool IsSplashTexturePath(const std::string& path);

    static bool IsFireLikePath(const std::string& path);

    static bool IsLightLikePath(const std::string& path);

    static bool IsSmokeLikePath(const std::string& path);

    static bool IsFountainBasinModel(const std::string& modelPath);



    static constexpr float kEffectMaxDistance = 850.0f;

    static constexpr int kMaxEffectLayers = 3;

    static constexpr int kMaxWaterEffectLayers = 2;

    static constexpr int kMaxSmokeEffectLayers = 2;

    struct FramePerfStats {
        int EffectRequests = 0;
        int ActiveParticles = 0;
        int ParticleDrawCalls = 0;
    };

    const FramePerfStats& GetLastFramePerf() const { return m_lastFramePerf; }

    static bool ShouldUseMeshForItem(const EfpRenderItem& item);

private:

    struct EffectVertex {

        float x, y, z;

        DWORD color;

        static constexpr DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;

    };



    struct EffectBillboardVertex {

        Vector3 Pos;

        DWORD Color;

        Vector2 UV;

        static constexpr DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

    };



    struct EffectMeshGpu {

        LPDIRECT3DVERTEXBUFFER9 Vb = nullptr;

        LPDIRECT3DINDEXBUFFER9 Ib = nullptr;

        int VertexCount = 0;

        int IndexCount = 0;

        ~EffectMeshGpu() {

            if (Vb) { Vb->Release(); Vb = nullptr; }

            if (Ib) { Ib->Release(); Ib = nullptr; }

        }

    };



    LPDIRECT3DDEVICE9 m_device = nullptr;

    TextureManager* m_texManager = nullptr;

    std::wstring m_clientPath;

    std::map<std::string, EfpResource> m_effectCache;

    std::map<std::string, BmsMesh> m_effectMeshCache;

    std::map<std::string, std::unique_ptr<EffectMeshGpu>> m_effectGpuCache;

    std::set<std::string> m_missingEffectMeshes;

    ParticleSystem m_particleSystem;

    DWORD m_lastTickMs = 0;
    float m_frameDt = 0.016f;

    bool m_effectPassActive = false;

    std::unique_ptr<RenderStateGuard> m_passStateGuard;

    FramePerfStats m_lastFramePerf{};



    static bool EndsWith(const std::string& text, const char* suffix);

    static std::string LowerPath(std::string path);

    static Vector3 RotateOffsetByYaw(const Vector3& offset, float yaw);

    static Vector3 ResolveEffectPosition(const Vector3& pos, const Vector3& localAnchor, float yaw,

                                         const Vector3& localRotation);

    static float ComputeEffectYaw(float placementYaw, const Vector3& localRotation);

    static ParticleSystem::InstanceKey MakeInstanceKey(const std::string& effectPath, const Vector3& pos,
                                                       const Vector3& anchor);



    std::wstring ResolveAssetPath(const std::string& path, const std::string& basePath = {}) const;

    std::wstring ResolveEffectTexturePath(const std::string& texRelPath, const std::string& effectRelPath) const;

    static std::wstring FindCaseInsensitiveFile(const std::wstring& dir, const std::wstring& fileName);



    const EfpResource* LoadEffectResource(const std::string& effectPath);

    const BmsMesh* LoadEffectMesh(const std::string& meshPath, const std::string& effectPath);

    EffectMeshGpu* LoadEffectMeshGpu(const std::string& meshPath, const std::string& effectPath);

    Texture* GetBasinWaterTexture();

    float MeshBoundsSize(const BmsMesh& mesh) const;

    void MeshBillboardExtents(const BmsMesh& mesh, const std::string& viewMode,

                               float& outWidth, float& outHeight) const;

    void TextureBillboardExtents(Texture* texture, const std::string& viewMode,

                                 float baseSize, float& outWidth, float& outHeight) const;



    void BeginEffectPass();

    void EndEffectPass();

    float AdvanceDeltaTime();



    static void ApplyTextureScroll(LPDIRECT3DDEVICE9 device, const UVCellTransform& cell);

    static void ResetTextureScroll(LPDIRECT3DDEVICE9 device);



    static D3DCOLOR ParticleColorForPath(const std::string& path, bool selected);

    void DrawParticleMarker(const Vector3& pos, const std::string& path, bool selected,

                            const Matrix4& view, const Matrix4& proj);



    static void ComputeBillboardAxes(const std::string& viewMode, const Vector3& pos,

                                   const Vector3& cameraPos, float yaw,

                                   Vector3& outRight, Vector3& outUp);



    void DrawEffectBillboard(Texture* texture, const Vector3& pos, const Vector3& cameraPos,

                             const Matrix4& view, const Matrix4& proj, float width, float height,

                             const SpriteAnimSample& animSample, DWORD color, bool additive,

                             const std::string& viewMode, float yaw, int srcBlend = 5, int dstBlend = 2);



    bool DrawEffectMeshWorld(const EfpRenderItem& item, const std::string& effectPath, Texture* texture,

                             const Vector3& pos, float yaw, const Matrix4& view, const Matrix4& proj,

                             const SpriteAnimSample& animSample, DWORD color, bool additive);



    bool ShouldUseMesh(const EfpRenderItem& item) const;

    bool ItemUsesBillboard(const EfpRenderItem& item) const;

    std::string ResolveViewMode(const EfpRenderItem& item, bool waterLike, bool fireLike, bool smokeLike) const;



    int ScoreRenderItem(const EfpRenderItem& item, const std::string& effectPath,

                        bool fireLike, bool waterLike, bool smokeLike);

    void CollectRenderItems(const EfpResource& effect, const std::string& effectPath,

                            bool fireLike, bool waterLike, bool smokeLike,

                            std::vector<const EfpRenderItem*>& out);

    const EfpObjectDef* PickPrimaryParticleObject(const EfpResource& effect, bool fireLike,

                                                  bool waterLike, bool smokeLike) const;



    bool DrawEffectLayer(const EfpRenderItem& item, const std::string& effectPath,

                         const Vector3& pos, const Vector3& cameraPos,

                         const Matrix4& view, const Matrix4& proj, float yaw, bool selected,

                         bool fireLike, bool waterLike, bool smokeLike, float timeSeconds);



    bool DrawParticleObjects(const EfpObjectDef& primary, const std::string& effectPath,

                             const Vector3& pos, const Vector3& cameraPos,

                             const Matrix4& view, const Matrix4& proj, float yaw, bool selected,

                             bool fireLike, bool waterLike, bool smokeLike, float dt,

                             const CameraFrustum* frustum);



    void DrawFountainBasinDisc(const Vector3& pos, float yaw, const Vector3& minBounds, const Vector3& maxBounds,

                               const Matrix4& view, const Matrix4& proj, float timeSeconds);



    bool DrawEffectResource(const std::string& effectPath, const Vector3& pos,

                            const Vector3& cameraPos, const Matrix4& view, const Matrix4& proj,

                            float yaw, bool selected, const CameraFrustum* frustum);

};


