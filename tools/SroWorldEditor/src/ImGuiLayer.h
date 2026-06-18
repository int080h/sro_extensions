#pragma once
#include <windows.h>
#include <string>
#include "RendererD3D9.h"
#include "imgui.h"

struct ImGuiLayer {
    bool Initialize(HWND hwnd, LPDIRECT3DDEVICE9 device);
    void Shutdown();
    void NewFrame();
    void Render();
    void ApplyDarkTheme();
    void SetupDocking(ImGuiID dockspaceId);
    bool WantCaptureKeyboard() const;
    bool WantCaptureMouse() const;

    static bool HasSavedLayout();

private:
    std::string m_iniPath;
};
