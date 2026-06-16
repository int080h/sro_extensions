#pragma once
#include "EditorContext.h"
#include "EditorViewport.h"
#include "ImGuiLayer.h"
#include "RendererD3D9.h"
#include "core/EditorSession.h"
#include <windows.h>

class Editor {
public:
    bool Initialize(HWND hwnd, RendererD3D9& renderer, ImGuiLayer& imgui);
    void Shutdown();
    void OnResize(UINT width, UINT height);
    void OnDeviceLost();
    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void Update(float dt);
    void Render(RendererD3D9& renderer, ImGuiLayer& imgui);

    EditorContext& Context() { return m_ctx; }
    EditorViewport& Viewport() { return m_viewport; }

    void NewProject();
    bool OpenProject();
    bool SaveProject();
    bool SaveProjectAs();
    void ImportClientData();
    void ResetLayout();
    void RunValidation();
    void ClearClientCache();
    void SetViewportFocused(bool focused) { m_viewportFocused = focused; }
    void RenderViewportScene(float w, float h);

private:
    void SaveSession();
    void DrawSessionRestoreModal();
    bool RestoreSession(const EditorSession& session);

    EditorContext m_ctx;
    EditorViewport m_viewport;
    EditorSession m_savedSession;
    HWND m_hwnd = nullptr;
    bool m_keys[256]{};
    bool m_layoutInitialized = false;
    bool m_viewportFocused = false;
    bool m_showSessionRestoreModal = false;
    bool m_sessionRestoreHandled = false;
    float m_sessionSaveTimer = 0.0f;
    LARGE_INTEGER m_freq{};
    LARGE_INTEGER m_lastTick{};
    int m_frameCount = 0;
    float m_fpsAccum = 0;
};
