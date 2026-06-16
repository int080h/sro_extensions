#pragma once
#include <windows.h>
#include <memory>
#include "RendererD3D9.h"
#include "ImGuiLayer.h"
#include "EditorPublic.h"

class App {
public:
    int Run(HINSTANCE hInstance);
    static App* Instance() { return s_instance; }
    Editor& GetEditor() { return m_editor; }
    RendererD3D9& GetRenderer() { return m_renderer; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool CreateAppWindow(HINSTANCE hInstance);

    static App* s_instance;
    HWND m_hwnd = nullptr;
    RendererD3D9 m_renderer;
    ImGuiLayer m_imgui;
    Editor m_editor;
};
