#pragma once
#include <d3d9.h>

struct SavedD3DState {
    DWORD fogEnable = 0;
    DWORD lighting = 0;
    DWORD alphaTest = 0;
    DWORD alphaBlend = 0;
    DWORD zWrite = 0;
    DWORD zEnable = 0;
    DWORD cullMode = 0;
    DWORD srcBlend = 0;
    DWORD destBlend = 0;
    DWORD fvf = 0;
    DWORD colorOp = 0;
    DWORD colorArg1 = 0;
    DWORD colorArg2 = 0;
    DWORD alphaOp = 0;
    DWORD alphaArg1 = 0;
    DWORD alphaArg2 = 0;
    DWORD addressU = 0;
    DWORD addressV = 0;
    DWORD texTransformFlags = 0;
    D3DMATRIX textureMatrix{};
    IDirect3DBaseTexture9* boundTexture = nullptr;
};

inline void CaptureD3DState(LPDIRECT3DDEVICE9 device, SavedD3DState& out) {
    if (!device) return;
    device->GetRenderState(D3DRS_FOGENABLE, &out.fogEnable);
    device->GetRenderState(D3DRS_LIGHTING, &out.lighting);
    device->GetRenderState(D3DRS_ALPHATESTENABLE, &out.alphaTest);
    device->GetRenderState(D3DRS_ALPHABLENDENABLE, &out.alphaBlend);
    device->GetRenderState(D3DRS_ZWRITEENABLE, &out.zWrite);
    device->GetRenderState(D3DRS_ZENABLE, &out.zEnable);
    device->GetRenderState(D3DRS_CULLMODE, &out.cullMode);
    device->GetRenderState(D3DRS_SRCBLEND, &out.srcBlend);
    device->GetRenderState(D3DRS_DESTBLEND, &out.destBlend);
    device->GetFVF(&out.fvf);
    device->GetTextureStageState(0, D3DTSS_COLOROP, &out.colorOp);
    device->GetTextureStageState(0, D3DTSS_COLORARG1, &out.colorArg1);
    device->GetTextureStageState(0, D3DTSS_COLORARG2, &out.colorArg2);
    device->GetTextureStageState(0, D3DTSS_ALPHAOP, &out.alphaOp);
    device->GetTextureStageState(0, D3DTSS_ALPHAARG1, &out.alphaArg1);
    device->GetTextureStageState(0, D3DTSS_ALPHAARG2, &out.alphaArg2);
    device->GetSamplerState(0, D3DSAMP_ADDRESSU, &out.addressU);
    device->GetSamplerState(0, D3DSAMP_ADDRESSV, &out.addressV);
    device->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &out.texTransformFlags);
    device->GetTransform(D3DTS_TEXTURE0, &out.textureMatrix);
    device->GetTexture(0, &out.boundTexture);
}

inline void RestoreD3DState(LPDIRECT3DDEVICE9 device, const SavedD3DState& state) {
    if (!device) return;
    device->SetRenderState(D3DRS_FOGENABLE, state.fogEnable);
    device->SetRenderState(D3DRS_LIGHTING, state.lighting);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, state.alphaTest);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, state.alphaBlend);
    device->SetRenderState(D3DRS_ZWRITEENABLE, state.zWrite);
    device->SetRenderState(D3DRS_ZENABLE, state.zEnable);
    device->SetRenderState(D3DRS_CULLMODE, state.cullMode);
    device->SetRenderState(D3DRS_SRCBLEND, state.srcBlend);
    device->SetRenderState(D3DRS_DESTBLEND, state.destBlend);
    device->SetFVF(state.fvf);
    device->SetTextureStageState(0, D3DTSS_COLOROP, state.colorOp);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, state.colorArg1);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, state.colorArg2);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, state.alphaOp);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, state.alphaArg1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, state.alphaArg2);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, state.addressU);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, state.addressV);
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, state.texTransformFlags);
    device->SetTransform(D3DTS_TEXTURE0, &state.textureMatrix);
    device->SetTexture(0, state.boundTexture);
    if (state.boundTexture) {
        state.boundTexture->Release();
    }
}

inline D3DBLEND MapEfpBlendFactor(int value, D3DBLEND fallback) {
    switch (value) {
        case 1: return D3DBLEND_ZERO;
        case 2: return D3DBLEND_ONE;
        case 3: return D3DBLEND_SRCCOLOR;
        case 4: return D3DBLEND_INVSRCCOLOR;
        case 5: return D3DBLEND_SRCALPHA;
        case 6: return D3DBLEND_INVSRCALPHA;
        case 7: return D3DBLEND_DESTALPHA;
        case 8: return D3DBLEND_INVDESTALPHA;
        case 9: return D3DBLEND_DESTCOLOR;
        case 10: return D3DBLEND_INVDESTCOLOR;
        case 11: return D3DBLEND_SRCALPHASAT;
        default: return fallback;
    }
}

inline void ApplyEffectTextureStages(LPDIRECT3DDEVICE9 device) {
    if (!device) return;
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
}

inline void ApplyEffectBlendState(LPDIRECT3DDEVICE9 device, int srcBlend, int dstBlend, bool additiveFallback) {
    if (!device) return;
    const D3DBLEND src = MapEfpBlendFactor(srcBlend, D3DBLEND_SRCALPHA);
    const D3DBLEND dst = MapEfpBlendFactor(dstBlend,
        additiveFallback ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_SRCBLEND, src);
    device->SetRenderState(D3DRS_DESTBLEND, dst);
    ApplyEffectTextureStages(device);
}

inline void ApplyOpaqueBaseline(LPDIRECT3DDEVICE9 device) {
    if (!device) return;
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

    D3DMATRIX identity{};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
    device->SetTransform(D3DTS_TEXTURE0, &identity);
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 2);

    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    device->SetTexture(0, nullptr);
}

class RenderStateGuard {
public:
    explicit RenderStateGuard(LPDIRECT3DDEVICE9 device) : m_device(device) {
        CaptureD3DState(m_device, m_saved);
    }

    ~RenderStateGuard() {
        RestoreD3DState(m_device, m_saved);
    }

    RenderStateGuard(const RenderStateGuard&) = delete;
    RenderStateGuard& operator=(const RenderStateGuard&) = delete;

private:
    LPDIRECT3DDEVICE9 m_device = nullptr;
    SavedD3DState m_saved{};
};
