#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>

namespace {

float HashSeed(float seed) {
    float x = sinf(seed * 12.9898f) * 43758.5453f;
    return x - floorf(x);
}

} // namespace

float ParticleSystem::ParticleLifetimeFade(float lifeT) {
    auto smoothstep = [](float edge0, float edge1, float x) {
        if (edge1 <= edge0) return x >= edge1 ? 1.0f : 0.0f;
        const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };
    const float fadeIn = smoothstep(0.0f, 0.15f, lifeT);
    const float fadeOut = 1.0f - smoothstep(0.7f, 1.0f, lifeT);
    return fadeIn * fadeOut;
}

void ParticleSystem::BeginDrawFrame() {
    m_batches.clear();
    m_lastActiveParticleCount = 0;
    m_lastParticleDrawCalls = 0;
}

ParticleSystem::InstanceKey ParticleSystem::MakeKey(const std::string& effectPath, const Vector3& pos,
                                                     const Vector3& anchor) {
    InstanceKey key;
    key.EffectPath = effectPath;
    key.PosX = static_cast<int>(pos.x * 10.0f);
    key.PosY = static_cast<int>(pos.y * 10.0f);
    key.PosZ = static_cast<int>(pos.z * 10.0f);
    key.AnchorX = static_cast<int>(anchor.x * 10.0f);
    key.AnchorY = static_cast<int>(anchor.y * 10.0f);
    key.AnchorZ = static_cast<int>(anchor.z * 10.0f);
    return key;
}

EfpEmitterDef ParticleSystem::DefaultEmitter() {
    EfpEmitterDef emit;
    emit.MinParticles = 4;
    emit.MaxParticles = 16;
    emit.BurstRate = 1;
    emit.SpawnRate = 1.0f;
    emit.Valid = true;
    return emit;
}

void ParticleSystem::Clear() {
    m_emitters.clear();
    m_batches.clear();
    ReleaseDynamicVb();
}

void ParticleSystem::OnDeviceLost() {
    ReleaseDynamicVb();
}

void ParticleSystem::ReleaseDynamicVb() {
    if (m_dynamicVb) {
        m_dynamicVb->Release();
        m_dynamicVb = nullptr;
    }
    m_dynamicVbCapacity = 0;
}

bool ParticleSystem::EnsureDynamicVb(LPDIRECT3DDEVICE9 device, UINT vertexCount) {
    if (!device) return false;
    if (m_dynamicVb && m_dynamicVbCapacity >= vertexCount) return true;

    if (m_dynamicVb) {
        m_dynamicVb->Release();
        m_dynamicVb = nullptr;
        m_dynamicVbCapacity = 0;
    }

    const UINT capacity = (std::max)(vertexCount, 4096u);
    const HRESULT hr = device->CreateVertexBuffer(
        capacity * sizeof(ParticleBillboardVertex),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        ParticleBillboardVertex::FVF,
        D3DPOOL_DEFAULT,
        &m_dynamicVb,
        nullptr);
    if (FAILED(hr)) {
        m_dynamicVb = nullptr;
        m_dynamicVbCapacity = 0;
        return false;
    }
    m_dynamicVbCapacity = capacity;
    return true;
}

void ParticleSystem::EnsurePool(EmitterState& state, int maxParticles) {
    const int poolSize = (std::max)((std::min)(maxParticles, kMaxParticlesPerEmitter), 1);
    if (static_cast<int>(state.Particles.size()) != poolSize) {
        state.Particles.assign(static_cast<size_t>(poolSize), LiveParticle{});
    }
}

void ParticleSystem::SpawnParticle(LiveParticle& p, const EfpEmitterDef& emit, const Vector3& anchor,
                                   float yaw, bool fireLike, bool waterLike, bool smokeLike) {
    p.Active = true;
    p.Age = 0.0f;
    p.Lifetime = 0.6f + HashSeed(p.Seed) * 0.9f;
    p.Seed = HashSeed(p.Seed + 17.31f);

    const float cosY = cosf(yaw);
    const float sinY = sinf(yaw);
    const float spread = 4.0f + HashSeed(p.Seed * 3.1f) * 5.0f;
    const float ox = (HashSeed(p.Seed * 1.7f) - 0.5f) * spread;
    const float oz = (HashSeed(p.Seed * 2.3f) - 0.5f) * spread;
    p.Position = anchor + Vector3(ox * cosY - oz * sinY, HashSeed(p.Seed * 4.9f) * 3.0f,
                                  ox * sinY + oz * cosY);

    if (fireLike) {
        p.Velocity = Vector3((HashSeed(p.Seed * 5.7f) - 0.5f) * 2.0f,
                             6.0f + HashSeed(p.Seed * 6.1f) * 4.0f,
                             (HashSeed(p.Seed * 7.3f) - 0.5f) * 2.0f);
        p.Size = 18.0f + HashSeed(p.Seed * 8.2f) * 14.0f;
    } else if (waterLike) {
        p.Velocity = Vector3((HashSeed(p.Seed * 5.7f) - 0.5f) * 1.5f,
                             3.0f + HashSeed(p.Seed * 6.1f) * 3.0f,
                             (HashSeed(p.Seed * 7.3f) - 0.5f) * 1.5f);
        p.Size = 18.0f + HashSeed(p.Seed * 8.2f) * 14.0f;
    } else if (smokeLike) {
        p.Velocity = Vector3((HashSeed(p.Seed * 5.7f) - 0.5f) * 2.0f,
                             5.0f + HashSeed(p.Seed * 6.1f) * 5.0f,
                             (HashSeed(p.Seed * 7.3f) - 0.5f) * 2.0f);
        p.Size = 28.0f + HashSeed(p.Seed * 8.2f) * 18.0f;
    } else {
        p.Velocity = Vector3(0.0f, 5.0f, 0.0f);
        p.Size = 22.0f;
    }
    (void)emit;
}

bool ParticleSystem::IsParticleOutOfBounds(const LiveParticle& p, const Vector3& anchor,
                                           bool fireLike, bool waterLike, bool smokeLike) const {
    const float dy = p.Position.y - anchor.y;
    const float dx = p.Position.x - anchor.x;
    const float dz = p.Position.z - anchor.z;
    const float horiz = sqrtf(dx * dx + dz * dz);

    if (fireLike) {
        return dy > kMaxFireHeight || dy < -4.0f || horiz > kMaxFireHorizontal;
    }
    if (waterLike) {
        return dy > kMaxWaterHeight || dy < -4.0f || horiz > kMaxFireHorizontal;
    }
    if (smokeLike) {
        return dy > kMaxSmokeHeight || dy < -6.0f || horiz > kMaxFireHorizontal * 1.5f;
    }
    return dy > kMaxSmokeHeight || horiz > kMaxFireHorizontal * 2.0f;
}

void ParticleSystem::RespawnParticle(LiveParticle& p, EmitterState& state, const EfpEmitterDef& emit) {
    SpawnParticle(p, emit, state.AnchorPos, state.Yaw, state.FireLike, state.WaterLike, state.SmokeLike);
}

void ParticleSystem::UpdateInstance(const InstanceKey& key, const EfpObjectDef& def, const Vector3& anchorPos,
                                    float yaw, float dt, bool fireLike, bool waterLike, bool smokeLike,
                                    const Vector3& cameraPos, const CameraFrustum* frustum, bool highQuality) {
    EmitterState& state = m_emitters[key];
    state.Def = def;
    state.AnchorPos = anchorPos;
    state.Yaw = yaw;
    state.FireLike = fireLike;
    state.WaterLike = waterLike;
    state.SmokeLike = smokeLike;

    EfpEmitterDef emit = def.Emitter.Valid ? def.Emitter : DefaultEmitter();
    const int poolCap = highQuality ? kMaxParticlesPerEmitter : kMaxParticlesPerEmitterLo;
    emit.MaxParticles = (std::min)(emit.MaxParticles, poolCap);
    emit.MinParticles = (std::min)(emit.MinParticles, emit.MaxParticles);
    EnsurePool(state, emit.MaxParticles);

    const float dx = anchorPos.x - cameraPos.x;
    const float dy = anchorPos.y - cameraPos.y;
    const float dz = anchorPos.z - cameraPos.z;
    const float distSq = dx * dx + dy * dy + dz * dz;
    const bool inRange = distSq <= kEffectMaxDrawDistance * kEffectMaxDrawDistance;

    bool inFrustum = true;
    if (frustum) {
        const float r = fireLike ? 80.0f : 48.0f;
        inFrustum = frustum->IsBoxVisible(
            Vector3(anchorPos.x - r, anchorPos.y - 8.0f, anchorPos.z - r),
            Vector3(anchorPos.x + r, anchorPos.y + kMaxFireHeight + 16.0f, anchorPos.z + r));
    }

    state.SpawnPaused = !inRange || !inFrustum;
    if (state.SpawnPaused) {
        state.OffscreenTime += dt;
    } else {
        state.OffscreenTime = 0.0f;
    }

    int activeCount = 0;
    for (LiveParticle& p : state.Particles) {
        if (!p.Active) continue;
        ++activeCount;
        p.Age += dt;
        p.Position = p.Position + p.Velocity * dt;

        const bool expired = p.Age >= p.Lifetime;
        const bool outOfBounds = IsParticleOutOfBounds(p, anchorPos, fireLike, waterLike, smokeLike);

        if (def.LifeCommand == "NormalTimeExtinct" && expired) {
            p.Active = false;
            --activeCount;
        } else if (expired || outOfBounds) {
            RespawnParticle(p, state, emit);
        }
    }

    if (state.SpawnPaused) {
        PruneOffscreenEmitters(dt);
        return;
    }

    const float spawnRate = (std::max)(0.1f, emit.SpawnRate);
    state.SpawnAccumulator += dt * spawnRate * static_cast<float>(emit.BurstRate);
    while (state.SpawnAccumulator >= 1.0f && activeCount < emit.MaxParticles) {
        state.SpawnAccumulator -= 1.0f;
        for (LiveParticle& p : state.Particles) {
            if (p.Active) continue;
            p.Seed = HashSeed(static_cast<float>(reinterpret_cast<uintptr_t>(&p) & 0xFFFF) + p.Seed + 0.31f);
            SpawnParticle(p, emit, anchorPos, yaw, fireLike, waterLike, smokeLike);
            ++activeCount;
            break;
        }
    }

    while (activeCount < emit.MinParticles) {
        bool spawned = false;
        for (LiveParticle& p : state.Particles) {
            if (p.Active) continue;
            p.Seed = HashSeed(static_cast<float>(reinterpret_cast<uintptr_t>(&p) & 0xFFFF) + 0.73f);
            SpawnParticle(p, emit, anchorPos, yaw, fireLike, waterLike, smokeLike);
            ++activeCount;
            spawned = true;
            break;
        }
        if (!spawned) break;
    }
}

void ParticleSystem::PruneOffscreenEmitters(float dt) {
    (void)dt;
    for (auto it = m_emitters.begin(); it != m_emitters.end();) {
        if (it->second.OffscreenTime >= kOffscreenPruneSeconds) {
            it = m_emitters.erase(it);
        } else {
            ++it;
        }
    }
}

bool ParticleSystem::ShouldCullParticle(const LiveParticle& p, const Vector3& anchor,
                                        const DrawContext& ctx) const {
    if (!ctx.CameraPos) return false;

    const float dy = p.Position.y - anchor.y;
    if (dy > kMaxFireHeight + 8.0f || dy < -12.0f) {
        return true;
    }

    const float dx = p.Position.x - ctx.CameraPos->x;
    const float dyCam = p.Position.y - ctx.CameraPos->y;
    const float dz = p.Position.z - ctx.CameraPos->z;
    const float distSq = dx * dx + dyCam * dyCam + dz * dz;
    if (distSq > ctx.MaxDrawDistance * ctx.MaxDrawDistance) {
        return true;
    }

    if (ctx.Frustum) {
        const float r = p.Size * 0.6f;
        if (!ctx.Frustum->IsBoxVisible(
                Vector3(p.Position.x - r, p.Position.y - r, p.Position.z - r),
                Vector3(p.Position.x + r, p.Position.y + r, p.Position.z + r))) {
            return true;
        }
    }
    return false;
}

ParticleSystem::SimDebugInfo ParticleSystem::GetSimDebug(const InstanceKey& key) const {
    SimDebugInfo info;
    auto it = m_emitters.find(key);
    if (it == m_emitters.end()) return info;

    const EmitterState& state = it->second;
    for (const LiveParticle& p : state.Particles) {
        if (!p.Active) continue;
        ++info.ActiveCount;
        const float dy = p.Position.y - state.AnchorPos.y;
        const float dx = p.Position.x - state.AnchorPos.x;
        const float dz = p.Position.z - state.AnchorPos.z;
        const float horiz = sqrtf(dx * dx + dz * dz);
        info.MaxHeightAboveAnchor = (std::max)(info.MaxHeightAboveAnchor, dy);
        info.MaxHorizontalFromAnchor = (std::max)(info.MaxHorizontalFromAnchor, horiz);
    }
    return info;
}
