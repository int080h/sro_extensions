#include "ViewportFramebuffer.h"

void ViewportFramebuffer::Initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;
}

void ViewportFramebuffer::Release() {
    if (m_savedDepth) { m_savedDepth->Release(); m_savedDepth = nullptr; }
    if (m_savedColor) { m_savedColor->Release(); m_savedColor = nullptr; }
    if (m_depthSurf) { m_depthSurf->Release(); m_depthSurf = nullptr; }
    if (m_colorSurf) { m_colorSurf->Release(); m_colorSurf = nullptr; }
    if (m_colorTex) { m_colorTex->Release(); m_colorTex = nullptr; }
    m_width = 0;
    m_height = 0;
}

void ViewportFramebuffer::Shutdown() {
    Release();
    m_device = nullptr;
}

void ViewportFramebuffer::OnDeviceLost() {
    Release();
}

bool ViewportFramebuffer::EnsureSize(UINT width, UINT height) {
    if (!m_device || width < 1 || height < 1) return false;
    if (m_colorTex && m_width == width && m_height == height) return true;

    Release();

    if (FAILED(m_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_colorTex, nullptr))) {
        return false;
    }
    if (FAILED(m_colorTex->GetSurfaceLevel(0, &m_colorSurf))) {
        Release();
        return false;
    }
    if (FAILED(m_device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &m_depthSurf, nullptr))) {
        Release();
        return false;
    }
    m_width = width;
    m_height = height;
    return true;
}

bool ViewportFramebuffer::BeginRender() {
    if (!m_device || !m_colorSurf || !m_depthSurf) return false;

    if (FAILED(m_device->GetRenderTarget(0, &m_savedColor))) return false;
    if (FAILED(m_device->GetDepthStencilSurface(&m_savedDepth))) {
        m_savedColor->Release();
        m_savedColor = nullptr;
        return false;
    }
    if (FAILED(m_device->SetRenderTarget(0, m_colorSurf))) return false;
    if (FAILED(m_device->SetDepthStencilSurface(m_depthSurf))) return false;

    D3DVIEWPORT9 vp{};
    vp.Width = m_width;
    vp.Height = m_height;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    m_device->SetViewport(&vp);
    return true;
}

void ViewportFramebuffer::EndRender() {
    if (!m_device) return;
    if (m_savedColor) {
        m_device->SetRenderTarget(0, m_savedColor);
        m_savedColor->Release();
        m_savedColor = nullptr;
    }
    if (m_savedDepth) {
        m_device->SetDepthStencilSurface(m_savedDepth);
        m_savedDepth->Release();
        m_savedDepth = nullptr;
    }
}
