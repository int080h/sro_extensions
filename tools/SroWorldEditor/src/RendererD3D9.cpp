#include "RendererD3D9.h"

RendererD3D9::~RendererD3D9() { Shutdown(); }

bool RendererD3D9::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_d3d) return false;

    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_width = rect.right - rect.left;
    m_height = rect.bottom - rect.top;

    ZeroMemory(&m_presentParams, sizeof(m_presentParams));
    m_presentParams.Windowed = TRUE;
    m_presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_presentParams.BackBufferFormat = D3DFMT_UNKNOWN;
    m_presentParams.EnableAutoDepthStencil = TRUE;
    m_presentParams.AutoDepthStencilFormat = D3DFMT_D24S8;
    m_presentParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    m_presentParams.BackBufferWidth = m_width;
    m_presentParams.BackBufferHeight = m_height;

    if (FAILED(m_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_presentParams, &m_device))) {
        return false;
    }
    return true;
}

void RendererD3D9::Shutdown() {
    if (m_device) { m_device->Release(); m_device = nullptr; }
    if (m_d3d) { m_d3d->Release(); m_d3d = nullptr; }
}

bool RendererD3D9::Reset() {
    if (!m_device) return false;
    return SUCCEEDED(m_device->Reset(&m_presentParams));
}

void RendererD3D9::BeginFrame(const float clearColor[4]) {
    if (!m_device) return;
    m_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        D3DCOLOR_RGBA((int)(clearColor[0] * 255), (int)(clearColor[1] * 255),
                      (int)(clearColor[2] * 255), (int)(clearColor[3] * 255)), 1.0f, 0);
    m_device->BeginScene();
}

void RendererD3D9::EndFrame() {
    if (!m_device) return;
    m_device->EndScene();
    m_device->Present(nullptr, nullptr, nullptr, nullptr);
}

void RendererD3D9::HandleResize(UINT width, UINT height) {
    if (width == 0 || height == 0) return;
    m_width = width;
    m_height = height;
    m_presentParams.BackBufferWidth = width;
    m_presentParams.BackBufferHeight = height;
}
