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
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    bool DuplicateSelection();
    bool DeleteSelection();
    bool SaveChangedClientFiles();
    bool ExportChangedClientFiles();
    void SetViewportFocused(bool focused) { m_viewportFocused = focused; }
    void RenderViewportScene(float w, float h);
    bool IsClientLoading() const { return m_viewport.IsClientLoadActive(); }
    const ClientLoadProgress& GetClientLoadProgress() const { return m_viewport.GetClientLoadProgress(); }

private:
    void SaveSession();
    void DrawSessionRestoreModal();
    void DrawClientLoadModal();
    bool RestoreSession(const EditorSession& session);
    void StartClientLoad(const std::wstring& path, int rx, int ry,
        const Vector3* restorePosition = nullptr, float restoreYaw = -90.f, float restorePitch = -30.f);

    EditorContext m_ctx;
    EditorViewport m_viewport;
    EditorSession m_savedSession;
    HWND m_hwnd = nullptr;
    bool m_keys[256]{};
    bool m_layoutInitialized = false;
    bool m_forceDefaultLayout = false;
    bool m_viewportFocused = false;
    bool m_showSessionRestoreModal = false;
    bool m_sessionRestoreHandled = false;
    bool m_clientLoadFailed = false;
    std::string m_clientLoadError;
    float m_sessionSaveTimer = 0.0f;
    LARGE_INTEGER m_freq{};
    LARGE_INTEGER m_lastTick{};
    int m_frameCount = 0;
    float m_fpsAccum = 0;
};
