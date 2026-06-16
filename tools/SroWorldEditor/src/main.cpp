#include "App.h"
#include "core/Logger.h"
#include "imgui_impl_dx9.h"
#include <chrono>

App* App::s_instance = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    if (!s_instance) return DefWindowProc(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && s_instance->m_renderer.Device()) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            s_instance->m_editor.OnDeviceLost();
            s_instance->m_renderer.HandleResize(LOWORD(lParam), HIWORD(lParam));
            s_instance->m_renderer.Reset();
            ImGui_ImplDX9_CreateDeviceObjects();
            s_instance->m_editor.OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        s_instance->m_editor.ProcessMessage(msg, wParam, lParam);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool App::CreateAppWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc{sizeof(wc), CS_CLASSDC, WndProc, 0, 0, hInstance, nullptr, nullptr, nullptr, nullptr, L"SroWorldEditorClass", nullptr};
    RegisterClassExW(&wc);
    m_hwnd = CreateWindowW(wc.lpszClassName, L"Silkroad Online Game World Editor",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900, nullptr, nullptr, hInstance, nullptr);
    return m_hwnd != nullptr;
}

int App::Run(HINSTANCE hInstance) {
    s_instance = this;
    Logger::Instance().InstallCrashHandlers();
    Logger::Instance().Info("Silkroad Online Game World Editor starting...");

    if (!CreateAppWindow(hInstance)) return 1;
    if (!m_renderer.Initialize(m_hwnd)) return 1;
    if (!m_imgui.Initialize(m_hwnd, m_renderer.Device())) return 1;
    if (!m_editor.Initialize(m_hwnd, m_renderer, m_imgui)) return 1;

    ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(m_hwnd);

    MSG msg{};
    auto last = std::chrono::steady_clock::now();
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        m_editor.Update(dt);
        m_editor.Render(m_renderer, m_imgui);
    }

    m_editor.Shutdown();
    m_imgui.Shutdown();
    m_renderer.Shutdown();
    DestroyWindow(m_hwnd);
    UnregisterClassW(L"SroWorldEditorClass", hInstance);
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    App app;
    return app.Run(hInstance);
}
