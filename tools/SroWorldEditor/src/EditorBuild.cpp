
#include "EditorPublic.h"
#include "ui/UiCommon.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include "core/EditorSession.h"
#include "cache/ClientDataCache.h"
#include "project/ProjectSerializer.h"
#include "imgui.h"
#include <commdlg.h>
#include <shlobj.h>
#include <filesystem>

bool Editor::Initialize(HWND hwnd, RendererD3D9& renderer, ImGuiLayer& /*imgui*/) {
    m_hwnd = hwnd;
    m_ctx.world = World::CreateSampleWorld();
    m_ctx.project.projectName = "MySROWorld";
    m_ctx.project.loadedRegions = {25000, 25001, 25002};
    m_ctx.RefreshValidation();
    m_viewport.Initialize(renderer.Device());
    m_viewport.GetCamera().Position = m_ctx.project.cameraPosition;
    m_viewport.GetCamera().Yaw = m_ctx.project.cameraYaw;
    m_viewport.GetCamera().Pitch = m_ctx.project.cameraPitch;
    QueryPerformanceFrequency(&m_freq);
    QueryPerformanceCounter(&m_lastTick);
    if (EditorSession::Load(m_savedSession) && m_savedSession.valid && !m_savedSession.clientPath.empty()
        && FileExistsA(m_savedSession.clientPath)) {
        m_showSessionRestoreModal = true;
    }
    Logger::Instance().Info("Editor initialized with sample world data.");
    return true;
}

void Editor::Shutdown() {
    SaveSession();
    m_viewport.Shutdown();
}

void Editor::OnResize(UINT width, UINT height) {
    m_viewport.OnResize((float)width, (float)height);
}

void Editor::OnDeviceLost() {
    m_viewport.OnDeviceLost();
    if (m_ctx.sroRenderManager) {
        m_ctx.sroRenderManager->OnDeviceLost();
    }
}

void Editor::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CLOSE) {
        SaveSession();
    }
    if (msg == WM_KEYDOWN || msg == WM_KEYUP) {
        bool down = (msg == WM_KEYDOWN);
        m_keys[wParam & 0xFF] = down;
        if (down && (GetKeyState(VK_CONTROL) & 0x8000)) {
            if (wParam == 'S') SaveProject();
            if (wParam == 'O') OpenProject();
            if (wParam == 'N') NewProject();
            if (wParam == 'Z') m_ctx.commandHistory.Undo();
            if (wParam == 'Y') m_ctx.commandHistory.Redo();
        } else if (down && wParam == 'M') {
            m_ctx.panels.worldMap = !m_ctx.panels.worldMap;
        }
    }
}

void Editor::Update(float dt) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    m_fpsAccum += dt;
    m_frameCount++;
    if (m_fpsAccum >= 0.5f) {
        m_ctx.fps = m_frameCount / m_fpsAccum;
        m_frameCount = 0;
        m_fpsAccum = 0;
    }

    bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right) && m_viewportFocused;
    float dx = ImGui::GetIO().MouseDelta.x;
    float dy = ImGui::GetIO().MouseDelta.y;
    m_viewport.Update(m_ctx, dt, m_viewportFocused, rmb, dx, dy, m_keys);

    if (m_ctx.sroClientLoaded) {
        m_sessionSaveTimer += dt;
        if (m_sessionSaveTimer >= 2.0f) {
            m_sessionSaveTimer = 0.0f;
            SaveSession();
        }
    }
}

void Editor::RenderViewportScene(float w, float h) {
    m_viewport.RenderToTexture(m_ctx, w, h);
}

void Editor::Render(RendererD3D9& renderer, ImGuiLayer& imgui) {
    const float clear[4] = {0.08f, 0.09f, 0.11f, 1.0f};
    renderer.BeginFrame(clear);

    imgui.NewFrame();
    DrawSessionRestoreModal();
    Ui::DrawMainMenuBar(m_ctx, *this);
    Ui::DrawToolbar(m_ctx, *this);

    ImGuiWindowFlags dockFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float menuH = ImGui::GetFrameHeight();
    float toolbarH = 36.0f;
    float statusH = 22.0f;
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + menuH + toolbarH));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, vp->WorkSize.y - menuH - toolbarH - statusH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("DockSpaceHost", nullptr, dockFlags);
    ImGui::PopStyleVar(2);

    ImGuiID dockId = ImGui::GetID("MainDockSpace");
    imgui.SetupDocking(dockId);
    if (!m_layoutInitialized) {
        Ui::SetupDefaultDockLayout((unsigned int)dockId);
        m_layoutInitialized = true;
    }

    if (m_ctx.panels.viewport) Ui::DrawViewportPanel(m_ctx, *this);
    if (m_ctx.panels.worldOutliner) Ui::DrawWorldOutlinerPanel(m_ctx);
    if (m_ctx.panels.properties) Ui::DrawPropertiesPanel(m_ctx);
    if (m_ctx.panels.assetBrowser) Ui::DrawAssetBrowserPanel(m_ctx);
    if (m_ctx.panels.regionManager) Ui::DrawRegionManagerPanel(m_ctx, *this);
    if (m_ctx.panels.worldMap) Ui::DrawWorldMapPanel(m_ctx, *this);
    if (m_ctx.panels.console) Ui::DrawConsolePanel(m_ctx);
    if (m_ctx.panels.validation) Ui::DrawValidationPanel(m_ctx);
    if (m_ctx.panels.performance) Ui::DrawPerformancePanel(m_ctx);

    ImGui::End();
    Ui::DrawStatusBar(m_ctx);

    ImGui::EndFrame();

    imgui.Render();
    renderer.EndFrame();
}

void Editor::NewProject() {
    m_ctx.project = Project{};
    m_ctx.project.projectName = "Untitled";
    m_ctx.world = World::CreateSampleWorld();
    m_ctx.ClearSelection();
    m_ctx.project.modified = false;
    Logger::Instance().Info("New project created.");
}

bool Editor::OpenProject() {
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = "SRO Project (*.sroproj)\0*.sroproj\0JSON (*.json)\0*.json\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    if (!GetOpenFileNameA(&ofn)) return false;
    Project loaded;
    if (!ProjectSerializer::Load(path, loaded)) {
        Logger::Instance().Error("Failed to load project.");
        return false;
    }
    m_ctx.project = loaded;
    m_viewport.GetCamera().Position = loaded.cameraPosition;
    m_viewport.GetCamera().Yaw = loaded.cameraYaw;
    m_viewport.GetCamera().Pitch = loaded.cameraPitch;
    m_ctx.viewport.cameraSpeed = loaded.settings.cameraSpeed;
    m_ctx.viewport.showGrid = loaded.settings.showGrid;
    m_ctx.viewport.showRegionBounds = loaded.settings.showRegionBounds;
    m_ctx.sroRegionRx = loaded.sroRegionRx;
    m_ctx.sroRegionRy = loaded.sroRegionRy;
    if (!loaded.clientPath.empty()) {
        m_ctx.sroClientLoaded = m_viewport.TryLoadSroClient(
            ToWide(loaded.clientPath), loaded.sroRegionRx, loaded.sroRegionRy,
            &loaded.cameraPosition, loaded.cameraYaw, loaded.cameraPitch);
        if (m_ctx.sroClientLoaded) {
            m_viewport.GetCamera().Position = loaded.cameraPosition;
            m_viewport.GetCamera().Yaw = loaded.cameraYaw;
            m_viewport.GetCamera().Pitch = loaded.cameraPitch;
        }
    }
    Logger::Instance().Info("Opened project: " + loaded.projectName);
    return true;
}

bool Editor::SaveProject() {
    if (m_ctx.project.filePath.empty()) return SaveProjectAs();
    m_ctx.project.cameraPosition = m_viewport.GetCamera().Position;
    m_ctx.project.cameraYaw = m_viewport.GetCamera().Yaw;
    m_ctx.project.cameraPitch = m_viewport.GetCamera().Pitch;
    m_ctx.project.settings.cameraSpeed = m_ctx.viewport.cameraSpeed;
    m_ctx.project.settings.showGrid = m_ctx.viewport.showGrid;
    m_ctx.project.settings.showRegionBounds = m_ctx.viewport.showRegionBounds;
    m_ctx.project.sroRegionRx = m_ctx.sroRegionRx;
    m_ctx.project.sroRegionRy = m_ctx.sroRegionRy;
    if (ProjectSerializer::Save(m_ctx.project, m_ctx.project.filePath)) {
        m_ctx.project.modified = false;
        Logger::Instance().Info("Project saved.");
        return true;
    }
    Logger::Instance().Error("Failed to save project.");
    return false;
}

bool Editor::SaveProjectAs() {
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = "SRO Project (*.sroproj)\0*.sroproj\0JSON (*.json)\0*.json\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameA(&ofn)) return false;
    m_ctx.project.filePath = path;
    return SaveProject();
}

void Editor::ImportClientData() {
    BROWSEINFOW bi{};
    bi.hwndOwner = m_hwnd;
    bi.lpszTitle = L"Select Silkroad Online Client Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;
    wchar_t path[MAX_PATH];
    SHGetPathFromIDListW(pidl, path);
    CoTaskMemFree(pidl);

    m_showSessionRestoreModal = false;
    m_sessionRestoreHandled = true;

    m_ctx.project.clientPath = ToNarrow(path);
    Logger::Instance().Info("Importing client data from: " + m_ctx.project.clientPath);
    m_ctx.sroClientLoaded = m_viewport.TryLoadSroClient(path, m_ctx.sroRegionRx, m_ctx.sroRegionRy);
    if (m_ctx.sroClientLoaded) {
        SaveSession();
        Logger::Instance().Info("Client data loaded successfully.");
    } else {
        Logger::Instance().Error("Failed to load client data. Check that the folder is a valid SRO client.");
    }
    m_ctx.MarkModified();
}

void Editor::ResetLayout() {
    m_layoutInitialized = false;
    ImGui::LoadIniSettingsFromDisk("sroworldeditor.ini");
    Logger::Instance().Info("Layout reset — restart or re-dock panels.");
}

void Editor::RunValidation() {
    m_ctx.RefreshValidation();
    Logger::Instance().Info("Validation complete: " + std::to_string(m_ctx.validationMessages.size()) + " messages.");
}

void Editor::SaveSession() {
    EditorSession session;
    EditorSession::CaptureFrom(m_ctx, m_viewport, session);
    EditorSession::Save(session);
}

bool Editor::RestoreSession(const EditorSession& session) {
    m_ctx.project.clientPath = session.clientPath;
    m_ctx.sroRegionRx = session.sroRegionRx;
    m_ctx.sroRegionRy = session.sroRegionRy;
    m_ctx.viewport.cameraSpeed = session.cameraSpeed;
    m_ctx.sroClientLoaded = m_viewport.TryLoadSroClient(
        ToWide(session.clientPath), session.sroRegionRx, session.sroRegionRy,
        &session.cameraPosition, session.cameraYaw, session.cameraPitch);
    if (m_ctx.sroClientLoaded) {
        m_viewport.GetCamera().Position = session.cameraPosition;
        m_viewport.GetCamera().Yaw = session.cameraYaw;
        m_viewport.GetCamera().Pitch = session.cameraPitch;
        SaveSession();
        Logger::Instance().Info("Restored last session: " + session.clientPath);
    }
    return m_ctx.sroClientLoaded;
}

void Editor::DrawSessionRestoreModal() {
    if (!m_showSessionRestoreModal || m_sessionRestoreHandled) return;

    ImGui::OpenPopup("Restore Session");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Restore Session", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string clientName = m_savedSession.clientPath;
        if (!clientName.empty()) {
            clientName = std::filesystem::path(clientName).filename().string();
        }
        ImGui::TextWrapped("Restore your last client session?");
        ImGui::Separator();
        ImGui::Text("Client: %s", clientName.c_str());
        ImGui::Text("Region: %d, %d", m_savedSession.sroRegionRx, m_savedSession.sroRegionRy);
        ImGui::Text("Camera: %.0f, %.0f, %.0f", m_savedSession.cameraPosition.x,
            m_savedSession.cameraPosition.y, m_savedSession.cameraPosition.z);
        ImGui::Spacing();

        if (ImGui::Button("Restore", ImVec2(120, 0))) {
            RestoreSession(m_savedSession);
            m_showSessionRestoreModal = false;
            m_sessionRestoreHandled = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Start Fresh", ImVec2(120, 0))) {
            m_showSessionRestoreModal = false;
            m_sessionRestoreHandled = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Editor::ClearClientCache() {
    sro::ClientDataCache::Instance().ClearCurrentClient();
}
