#include "ParticleSystem.h"

#include "Rendering/RenderStateGuard.h"

#include <cstring>

#include <vector>



void ParticleSystem::QueueParticleBillboard(const LiveParticle& particle, Texture* texture,

                                            const FrameTextureSlideData& slide, const DrawContext& ctx,

                                            const std::string& viewMode, float yaw, DWORD tintColor,

                                            int srcBlend, int dstBlend, bool additive) {

    if (!texture || !texture->pTexture || !ctx.Device || !ctx.CameraPos || !ctx.View || !ctx.Proj) {

        return;

    }



    const float lifeT = (particle.Lifetime > 0.0f) ? (particle.Age / particle.Lifetime) : 0.0f;

    const SpriteAnimSample sample = SampleSpriteAnim(slide, lifeT);

    const float lifeFade = ParticleLifetimeFade(lifeT);



    Vector3 right, up;

    ctx.ComputeAxes(viewMode, particle.Position, *ctx.CameraPos, yaw, right, up);



    const float halfW = particle.Size * 0.5f;

    const float halfH = particle.Size * 0.55f;



    const BYTE baseAlpha = static_cast<BYTE>(

        static_cast<float>((tintColor >> 24) & 0xFF) * lifeFade);



    auto appendPass = [&](const UVCellTransform& cell, BYTE alpha) {

        if (alpha == 0) return;

        const DWORD color = (tintColor & 0x00FFFFFF) | (static_cast<DWORD>(alpha) << 24);

        const float halfTexU = (texture->Width > 0) ? (0.5f / static_cast<float>(texture->Width)) : 0.0f;

        const float halfTexV = (texture->Height > 0) ? (0.5f / static_cast<float>(texture->Height)) : 0.0f;

        const float u0 = cell.OffsetU + halfTexU;

        const float v0 = cell.OffsetV + halfTexV;

        const float u1 = cell.OffsetU + cell.ScaleU - halfTexU;

        const float v1 = cell.OffsetV + cell.ScaleV - halfTexV;



        ParticleBatchKey key;

        key.Texture = texture->pTexture;

        key.SrcBlend = MapEfpBlendFactor(srcBlend, D3DBLEND_SRCALPHA);

        key.DstBlend = MapEfpBlendFactor(dstBlend, additive ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);



        std::vector<ParticleBillboardVertex>& batch = m_batches[key];

        const size_t base = batch.size();

        batch.resize(base + 6);

        batch[base + 0] = {particle.Position - right * halfW - up * halfH, color, Vector2(u0, v1)};

        batch[base + 1] = {particle.Position - right * halfW + up * halfH, color, Vector2(u0, v0)};

        batch[base + 2] = {particle.Position + right * halfW - up * halfH, color, Vector2(u1, v1)};

        batch[base + 3] = batch[base + 2];

        batch[base + 4] = batch[base + 1];

        batch[base + 5] = {particle.Position + right * halfW + up * halfH, color, Vector2(u1, v0)};

    };



    if (sample.Blend > 0.001f && sample.Blend < 0.999f) {

        const BYTE alphaA = static_cast<BYTE>(static_cast<float>(baseAlpha) * (1.0f - sample.Blend));

        const BYTE alphaB = static_cast<BYTE>(static_cast<float>(baseAlpha) * sample.Blend);

        appendPass(sample.Cell, alphaA);

        appendPass(sample.NextCell, alphaB);

    } else {

        appendPass(sample.Cell, baseAlpha);

    }

}



void ParticleSystem::FlushBatches(const DrawContext& ctx) {

    if (!ctx.Device || !ctx.View || !ctx.Proj || m_batches.empty()) return;



    UINT totalVerts = 0;

    for (const auto& [key, verts] : m_batches) {

        totalVerts += static_cast<UINT>(verts.size());

    }

    if (totalVerts == 0) {

        m_batches.clear();

        return;

    }



    Matrix4 identity = Matrix4::Identity();

    ctx.Device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&identity));

    ctx.Device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(ctx.View));

    ctx.Device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(ctx.Proj));

    ctx.Device->SetFVF(ParticleBillboardVertex::FVF);

    ApplyEffectTextureStages(ctx.Device);

    if (ctx.ResetTexTransform) ctx.ResetTexTransform(ctx.Device);



    struct BatchRange {

        IDirect3DTexture9* Texture;

        DWORD SrcBlend;

        DWORD DstBlend;

        UINT StartVertex;

        UINT PrimitiveCount;

    };



    IDirect3DTexture9* lastTex = nullptr;

    DWORD lastSrcBlend = 0xFFFFFFFF;

    DWORD lastDstBlend = 0xFFFFFFFF;



    const bool useVb = EnsureDynamicVb(ctx.Device, totalVerts);

    if (useVb) {

        void* vbData = nullptr;

        if (FAILED(m_dynamicVb->Lock(0, 0, &vbData, D3DLOCK_DISCARD))) {

            m_batches.clear();

            return;

        }

        std::vector<BatchRange> ranges;

        ranges.reserve(m_batches.size());

        UINT offset = 0;

        for (const auto& [key, verts] : m_batches) {

            if (verts.empty() || !key.Texture) continue;

            std::memcpy(static_cast<BYTE*>(vbData) + offset * sizeof(ParticleBillboardVertex),

                        verts.data(), verts.size() * sizeof(ParticleBillboardVertex));

            BatchRange r;

            r.Texture = key.Texture;

            r.SrcBlend = key.SrcBlend;

            r.DstBlend = key.DstBlend;

            r.StartVertex = offset;

            r.PrimitiveCount = static_cast<UINT>(verts.size() / 3);

            ranges.push_back(r);

            offset += static_cast<UINT>(verts.size());

        }

        m_dynamicVb->Unlock();

        ctx.Device->SetStreamSource(0, m_dynamicVb, 0, sizeof(ParticleBillboardVertex));



        for (const BatchRange& r : ranges) {

            if (r.Texture != lastTex) {

                ctx.Device->SetTexture(0, r.Texture);

                lastTex = r.Texture;

            }

            if (r.SrcBlend != lastSrcBlend) {

                ctx.Device->SetRenderState(D3DRS_SRCBLEND, r.SrcBlend);

                lastSrcBlend = r.SrcBlend;

            }

            if (r.DstBlend != lastDstBlend) {

                ctx.Device->SetRenderState(D3DRS_DESTBLEND, r.DstBlend);

                lastDstBlend = r.DstBlend;

            }

            ctx.Device->DrawPrimitive(D3DPT_TRIANGLELIST, r.StartVertex, r.PrimitiveCount);

            ++m_lastParticleDrawCalls;

        }

    } else {

        for (const auto& [key, verts] : m_batches) {

            if (verts.empty() || !key.Texture) continue;

            if (key.Texture != lastTex) {

                ctx.Device->SetTexture(0, key.Texture);

                lastTex = key.Texture;

            }

            if (key.SrcBlend != lastSrcBlend) {

                ctx.Device->SetRenderState(D3DRS_SRCBLEND, key.SrcBlend);

                lastSrcBlend = key.SrcBlend;

            }

            if (key.DstBlend != lastDstBlend) {

                ctx.Device->SetRenderState(D3DRS_DESTBLEND, key.DstBlend);

                lastDstBlend = key.DstBlend;

            }

            ctx.Device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(verts.size() / 3),

                                        verts.data(), sizeof(ParticleBillboardVertex));

            ++m_lastParticleDrawCalls;

        }

    }



    m_batches.clear();

}



bool ParticleSystem::DrawInstance(const InstanceKey& key, const DrawContext& ctx,

                                  const std::string& effectPath, float yaw,

                                  DWORD tintColor, bool additive, int srcBlend, int dstBlend) {

    auto it = m_emitters.find(key);

    if (it == m_emitters.end()) return false;



    const EmitterState& state = it->second;

    if (state.Def.Render.TexturePaths.empty() || !ctx.ResolveTexture) return false;



    const std::wstring texPath = ctx.ResolveTexture(state.Def.Render.TexturePaths.front(), effectPath);

    if (texPath.empty() || !ctx.TexManager) return false;

    Texture* tex = ctx.TexManager->GetTexture(texPath, false);

    if (!tex || !tex->pTexture) return false;



    FrameTextureSlideData slide = state.Def.Render.TextureSlide;

    if (!slide.Valid) {

        const SpriteAtlasLayout layout = InferAtlasLayoutFromTexture(tex);

        slide.Left = Vector3(static_cast<float>(layout.Cols),

                             static_cast<float>(layout.Rows), 1.0f);

        slide.Valid = layout.FrameCount > 1;

    }



    const std::string viewMode = state.Def.Render.ViewMode.empty() ? "ViewYBillboard"

                                                                   : state.Def.Render.ViewMode;

    const int blendSrc = state.Def.Render.SrcBlend > 0 ? state.Def.Render.SrcBlend : srcBlend;

    const int blendDst = state.Def.Render.DstBlend > 0 ? state.Def.Render.DstBlend : dstBlend;



    bool queued = false;

    for (const LiveParticle& p : state.Particles) {

        if (!p.Active) continue;

        ++m_lastActiveParticleCount;

        if (ShouldCullParticle(p, state.AnchorPos, ctx)) continue;

        QueueParticleBillboard(p, tex, slide, ctx, viewMode, yaw, tintColor, blendSrc, blendDst, additive);

        queued = true;

    }

    return queued;

}

