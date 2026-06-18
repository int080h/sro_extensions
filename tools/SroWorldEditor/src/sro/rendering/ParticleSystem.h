#pragma once

#include <d3d9.h>

#include <cstdint>

#include <functional>

#include <map>

#include <string>

#include <vector>

#include "Core/Math.h"

#include "Parsers/EfpParser.h"

#include "Rendering/BitmapSpriteAnimator.h"

#include "Rendering/Camera.h"

#include "Rendering/Texture.h"



struct ParticleBillboardVertex {

    Vector3 Pos;

    DWORD Color;

    Vector2 UV;

    static constexpr DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

};



class ParticleSystem {

public:

    static constexpr float kMaxFireHeight = 55.0f;

    static constexpr float kMaxFireHorizontal = 14.0f;

    static constexpr float kMaxWaterHeight = 30.0f;

    static constexpr float kMaxSmokeHeight = 70.0f;

    static constexpr float kOffscreenPruneSeconds = 8.0f;

    static constexpr float kEffectMaxDrawDistance = 850.0f;

    static constexpr int kMaxParticlesPerEmitter = 64;

    static constexpr int kMaxParticlesPerEmitterLo = 32;



    struct DrawContext {

        LPDIRECT3DDEVICE9 Device = nullptr;

        TextureManager* TexManager = nullptr;

        const Vector3* CameraPos = nullptr;

        const Matrix4* View = nullptr;

        const Matrix4* Proj = nullptr;

        const CameraFrustum* Frustum = nullptr;

        float MaxDrawDistance = kEffectMaxDrawDistance;

        std::function<std::wstring(const std::string& texRel, const std::string& effectPath)> ResolveTexture;

        std::function<void(const std::string& viewMode, const Vector3& pos, const Vector3& cameraPos,

                           float yaw, Vector3& right, Vector3& up)> ComputeAxes;

        std::function<void(LPDIRECT3DDEVICE9)> ResetTexTransform;

    };



    struct InstanceKey {

        std::string EffectPath;

        int PosX = 0;

        int PosY = 0;

        int PosZ = 0;

        int AnchorX = 0;

        int AnchorY = 0;

        int AnchorZ = 0;



        bool operator<(const InstanceKey& o) const {

            if (EffectPath != o.EffectPath) return EffectPath < o.EffectPath;

            if (PosX != o.PosX) return PosX < o.PosX;

            if (PosY != o.PosY) return PosY < o.PosY;

            if (PosZ != o.PosZ) return PosZ < o.PosZ;

            if (AnchorX != o.AnchorX) return AnchorX < o.AnchorX;

            if (AnchorY != o.AnchorY) return AnchorY < o.AnchorY;

            return AnchorZ < o.AnchorZ;

        }

    };



    struct LiveParticle {

        float Age = 0.0f;

        float Lifetime = 1.0f;

        Vector3 Position;

        Vector3 Velocity;

        float Size = 32.0f;

        float Seed = 0.0f;

        bool Active = false;

    };



    struct EmitterState {

        std::vector<LiveParticle> Particles;

        float SpawnAccumulator = 0.0f;

        float OffscreenTime = 0.0f;

        Vector3 AnchorPos;

        float Yaw = 0.0f;

        bool FireLike = false;

        bool WaterLike = false;

        bool SmokeLike = false;

        bool SpawnPaused = false;

        EfpObjectDef Def;

    };



    struct SimDebugInfo {

        int ActiveCount = 0;

        float MaxHeightAboveAnchor = 0.0f;

        float MaxHorizontalFromAnchor = 0.0f;

    };



    void Clear();



    void BeginDrawFrame();



    void OnDeviceLost();



    void UpdateInstance(const InstanceKey& key, const EfpObjectDef& def, const Vector3& anchorPos,

                        float yaw, float dt, bool fireLike, bool waterLike, bool smokeLike,

                        const Vector3& cameraPos, const CameraFrustum* frustum, bool highQuality = true);



    bool DrawInstance(const InstanceKey& key, const DrawContext& ctx, const std::string& effectPath,

                      float yaw, DWORD tintColor, bool additive, int srcBlend = 5, int dstBlend = 2);



    void FlushBatches(const DrawContext& ctx);



    int GetLastActiveParticleCount() const { return m_lastActiveParticleCount; }

    int GetLastParticleDrawCalls() const { return m_lastParticleDrawCalls; }



    SimDebugInfo GetSimDebug(const InstanceKey& key) const;



    static EfpEmitterDef DefaultEmitter();

    static InstanceKey MakeKey(const std::string& effectPath, const Vector3& pos, const Vector3& anchor);

    static float ParticleLifetimeFade(float lifeT);



private:

    struct ParticleBatchKey {

        IDirect3DTexture9* Texture = nullptr;

        DWORD SrcBlend = 0;

        DWORD DstBlend = 0;



        bool operator<(const ParticleBatchKey& o) const {

            if (Texture != o.Texture) return Texture < o.Texture;

            if (SrcBlend != o.SrcBlend) return SrcBlend < o.SrcBlend;

            return DstBlend < o.DstBlend;

        }

    };



    std::map<InstanceKey, EmitterState> m_emitters;

    std::map<ParticleBatchKey, std::vector<ParticleBillboardVertex>> m_batches;

    int m_lastActiveParticleCount = 0;

    int m_lastParticleDrawCalls = 0;



    LPDIRECT3DVERTEXBUFFER9 m_dynamicVb = nullptr;

    UINT m_dynamicVbCapacity = 0;



    void EnsurePool(EmitterState& state, int maxParticles);

    void SpawnParticle(LiveParticle& p, const EfpEmitterDef& emit, const Vector3& anchor,

                       float yaw, bool fireLike, bool waterLike, bool smokeLike);

    bool IsParticleOutOfBounds(const LiveParticle& p, const Vector3& anchor,

                               bool fireLike, bool waterLike, bool smokeLike) const;

    void RespawnParticle(LiveParticle& p, EmitterState& state, const EfpEmitterDef& emit);

    bool ShouldCullParticle(const LiveParticle& p, const Vector3& anchor, const DrawContext& ctx) const;

    void QueueParticleBillboard(const LiveParticle& particle, Texture* texture,

                                const FrameTextureSlideData& slide, const DrawContext& ctx,

                                const std::string& viewMode, float yaw, DWORD tintColor,

                                int srcBlend, int dstBlend, bool additive);

    void PruneOffscreenEmitters(float dt);

    bool EnsureDynamicVb(LPDIRECT3DDEVICE9 device, UINT vertexCount);

    void ReleaseDynamicVb();

};

