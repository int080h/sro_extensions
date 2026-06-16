#pragma once
#include <d3d9.h>

class ViewportFramebuffer {
public:
    void Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();
    void OnDeviceLost();
    bool EnsureSize(UINT width, UINT height);
    bool BeginRender();
    void EndRender();

    LPDIRECT3DTEXTURE9 ColorTexture() const { return m_colorTex; }
    UINT Width() const { return m_width; }
    UINT Height() const { return m_height; }

private:
    void Release();

    LPDIRECT3DDEVICE9 m_device = nullptr;
    LPDIRECT3DTEXTURE9 m_colorTex = nullptr;
    LPDIRECT3DSURFACE9 m_colorSurf = nullptr;
    LPDIRECT3DSURFACE9 m_depthSurf = nullptr;
    LPDIRECT3DSURFACE9 m_savedColor = nullptr;
    LPDIRECT3DSURFACE9 m_savedDepth = nullptr;
    UINT m_width = 0;
    UINT m_height = 0;
};
