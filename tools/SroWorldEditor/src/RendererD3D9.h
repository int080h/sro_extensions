#pragma once
#include <d3d9.h>
#include <windows.h>

class RendererD3D9 {
public:
    RendererD3D9() = default;
    ~RendererD3D9();

    bool Initialize(HWND hwnd);
    void Shutdown();
    bool Reset();
    void BeginFrame(const float clearColor[4]);
    void EndFrame();
    void HandleResize(UINT width, UINT height);

    LPDIRECT3DDEVICE9 Device() const { return m_device; }
    UINT Width() const { return m_width; }
    UINT Height() const { return m_height; }

private:
    LPDIRECT3D9 m_d3d = nullptr;
    LPDIRECT3DDEVICE9 m_device = nullptr;
    D3DPRESENT_PARAMETERS m_presentParams{};
    HWND m_hwnd = nullptr;
    UINT m_width = 0;
    UINT m_height = 0;
};
