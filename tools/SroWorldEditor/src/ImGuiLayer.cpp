#include "ImGuiLayer.h"
#include "core/EditorSession.h"
#include "core/FileSystem.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

bool ImGuiLayer::HasSavedLayout() {
    const std::wstring path = EditorSession::IniFilePath();
    return !path.empty() && FileExists(path);
}

bool ImGuiLayer::Initialize(HWND hwnd, LPDIRECT3DDEVICE9 device) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    m_iniPath = ToNarrow(EditorSession::IniFilePath());
    io.IniFilename = m_iniPath.empty() ? nullptr : m_iniPath.c_str();
    io.Fonts->AddFontDefault();
    ApplyDarkTheme();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);
    return true;
}

void ImGuiLayer::Shutdown() {
    if (ImGui::GetCurrentContext() && !m_iniPath.empty())
        ImGui::SaveIniSettingsToDisk(m_iniPath.c_str());
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::NewFrame() {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::Render() {
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::ApplyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8, 6);
    style.WindowPadding = ImVec2(10, 10);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.95f, 1.0f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
    c[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.13f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
    c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.13f, 0.98f);
    c[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);
    c[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.20f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.28f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.28f, 0.33f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.13f, 0.16f, 1.0f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.29f, 0.35f, 1.0f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.34f, 0.42f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.24f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.29f, 0.35f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.34f, 0.42f, 1.0f);
    c[ImGuiCol_Tab] = ImVec4(0.12f, 0.13f, 0.16f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.24f, 0.28f, 1.0f);
    c[ImGuiCol_TabActive] = ImVec4(0.18f, 0.20f, 0.24f, 1.0f);
    c[ImGuiCol_CheckMark] = ImVec4(0.55f, 0.75f, 1.0f, 1.0f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.55f, 0.85f, 1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.65f, 0.95f, 1.0f);
}

void ImGuiLayer::SetupDocking(ImGuiID dockspaceId) {
    ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
}

bool ImGuiLayer::WantCaptureKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiLayer::WantCaptureMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}
