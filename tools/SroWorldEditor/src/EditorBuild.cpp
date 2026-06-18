
#include "EditorPublic.h"
#include "core/ClientLoadProgress.h"
#include "ui/UiCommon.h"
#include "ui/NavLayersPanel.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include "core/EditorAction.h"
#include "core/EditorSession.h"
#include "ImGuiLayer.h"
#include "cache/ClientDataCache.h"
#include "project/ProjectSerializer.h"
#include "runtime/RegionManager.h"
#include "imgui.h"
#include <commdlg.h>
#include <shlobj.h>
#include <algorithm>
#include <filesystem>
#include <set>

namespace {

void ClearSampleWorldNpcs(World& world) {
    for (auto& region : world.regions)
        region.npcs.clear();
}

bool PickFolder(HWND owner, const wchar_t* title, std::wstring& outPath) {
    BROWSEINFOW bi{};
    bi.hwndOwner = owner;
    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return false;
    wchar_t path[MAX_PATH];
    const BOOL ok = SHGetPathFromIDListW(pidl, path);
    CoTaskMemFree(pidl);
    if (!ok) return false;
    outPath = path;
    return true;
}

int16_t NextPlacementUid(const std::vector<PlacementVM>& placements, int rx, int ry) {
    std::set<int> used;
    int maxUid = 0;
    for (const auto& p : placements) {
        if (p.LoadedRx != rx || p.LoadedRy != ry) continue;
        const int uid = static_cast<int>(p.Object.UID);
        used.insert(uid);
        if (uid > maxUid) maxUid = uid;
    }

    if (maxUid > 0 && maxUid < 32767)
        return static_cast<int16_t>(maxUid + 1);

    for (int uid = 1; uid <= 32767; ++uid) {
        if (!used.count(uid))
            return static_cast<int16_t>(uid);
    }
    return 0;
}

} // namespace

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
    if (EditorSession::Load(m_savedSession)) {
        EditorSession::ApplyUiToContext(m_ctx, m_savedSession);
        if (m_savedSession.valid && !m_savedSession.clientPath.empty()
            && FileExistsA(m_savedSession.clientPath)) {
            m_showSessionRestoreModal = true;
        }
    }
    Logger::Instance().Info("Editor initialized with sample world data.");
    return true;
}

void Editor::Shutdown() {
    SaveSession();
    if (ImGui::GetCurrentContext() && ImGui::GetIO().IniFilename)
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
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
        const bool uiCapturesKeyboard = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard;
        if (down && (GetKeyState(VK_CONTROL) & 0x8000)) {
            if (wParam == 'S') SaveProject();
            if (wParam == 'O') OpenProject();
            if (wParam == 'N') NewProject();
            if (!uiCapturesKeyboard) {
                if (wParam == 'Z') Undo();
                if (wParam == 'Y') Redo();
                if (wParam == 'D') DuplicateSelection();
            }
        } else if (down && wParam == 'M') {
            if (!ImGui::GetCurrentContext() || !ImGui::GetIO().WantCaptureKeyboard)
                m_ctx.panels.worldMap = !m_ctx.panels.worldMap;
        } else if (down && wParam == VK_DELETE && !uiCapturesKeyboard) {
            DeleteSelection();
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

    if (m_viewport.IsClientLoadActive()) {
        ClientLoadProgress progress;
        const bool more = m_viewport.TickClientLoad(progress);
        if (progress.failed) {
            m_clientLoadFailed = true;
            m_clientLoadError = progress.error.empty() ? "Client load failed" : progress.error;
            m_ctx.sroClientLoaded = false;
            Logger::Instance().Error(m_clientLoadError);
        } else if (!more && progress.done) {
            m_ctx.sroClientLoaded = true;
            ClearSampleWorldNpcs(m_ctx.world);
            SaveSession();
            Logger::Instance().Info("Client data loaded successfully.");
            m_ctx.MarkModified();
        }
        (void)more;
    }

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
    DrawClientLoadModal();
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
        if (m_forceDefaultLayout || !ImGuiLayer::HasSavedLayout())
            Ui::SetupDefaultDockLayout((unsigned int)dockId);
        m_forceDefaultLayout = false;
        m_layoutInitialized = true;
    }

    if (m_ctx.panels.viewport) Ui::DrawViewportPanel(m_ctx, *this);
    if (m_ctx.panels.worldOutliner) Ui::DrawWorldOutlinerPanel(m_ctx);
    if (m_ctx.panels.properties) Ui::DrawPropertiesPanel(m_ctx);
    if (m_ctx.panels.assetBrowser) Ui::DrawAssetBrowserPanel(m_ctx);
    if (m_ctx.panels.objectViewer) Ui::DrawObjectViewerPanel(m_ctx, *this);
    if (m_ctx.panels.npcEditor) Ui::DrawNpcEditorPanel(m_ctx);
    if (m_ctx.panels.regionManager) Ui::DrawRegionManagerPanel(m_ctx, *this);
    if (m_ctx.panels.worldMap) Ui::DrawWorldMapPanel(m_ctx, *this);
    if (m_ctx.panels.console) Ui::DrawConsolePanel(m_ctx);
    if (m_ctx.panels.validation) Ui::DrawValidationPanel(m_ctx);
    if (m_ctx.panels.performance) Ui::DrawPerformancePanel(m_ctx);
    if (m_ctx.panels.terrainNavPanel) Ui::DrawTerrainNavPanel(m_ctx);
    if (m_ctx.panels.navLayersPanel) Ui::DrawNavLayersPanel(m_ctx);
    if (m_ctx.panels.aiNavDataPanel) Ui::DrawAiNavDataPanel(m_ctx);
    if (m_ctx.panels.dungeonNavPanel) Ui::DrawDungeonNavPanel(m_ctx);
    if (m_ctx.panels.navMeshBrowser) Ui::DrawNavMeshBrowserPanel(m_ctx, *this);
    if (m_ctx.panels.collisionEditor) Ui::DrawCollisionEditorPanel(m_ctx);

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
        StartClientLoad(ToWide(loaded.clientPath), loaded.sroRegionRx, loaded.sroRegionRy,
            &loaded.cameraPosition, loaded.cameraYaw, loaded.cameraPitch);
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

void Editor::StartClientLoad(const std::wstring& path, int rx, int ry,
    const Vector3* restorePosition, float restoreYaw, float restorePitch) {
    m_showSessionRestoreModal = false;
    m_sessionRestoreHandled = true;
    m_clientLoadFailed = false;
    m_clientLoadError.clear();
    m_ctx.project.clientPath = ToNarrow(path);
    m_ctx.sroRegionRx = rx;
    m_ctx.sroRegionRy = ry;
    m_ctx.sroClientLoaded = false;

    EditorViewport::ClientLoadRequest req;
    req.clientPath = path;
    req.rx = rx;
    req.ry = ry;
    if (restorePosition) {
        req.useRestoreCamera = true;
        req.restorePosition = *restorePosition;
        req.restoreYaw = restoreYaw;
        req.restorePitch = restorePitch;
    }
    m_viewport.BeginClientLoad(req);
}

void Editor::ImportClientData() {
    std::wstring path;
    if (!PickFolder(m_hwnd, L"Select Silkroad Online Client Folder", path)) return;

    m_showSessionRestoreModal = false;
    m_sessionRestoreHandled = true;

    m_ctx.project.clientPath = ToNarrow(path);
    Logger::Instance().Info("Importing client data from: " + m_ctx.project.clientPath);
    StartClientLoad(path, m_ctx.sroRegionRx, m_ctx.sroRegionRy);
}

void Editor::ResetLayout() {
    const std::wstring iniPath = EditorSession::IniFilePath();
    if (!iniPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(iniPath, ec);
    }
    m_forceDefaultLayout = true;
    m_layoutInitialized = false;
    Logger::Instance().Info("Layout reset — applying default dock layout.");
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
    m_ctx.viewport.cameraSpeed = session.cameraSpeed;
    StartClientLoad(ToWide(session.clientPath), session.sroRegionRx, session.sroRegionRy,
        &session.cameraPosition, session.cameraYaw, session.cameraPitch);
    Logger::Instance().Info("Restoring session: " + session.clientPath);
    return true;
}

void Editor::DrawClientLoadModal() {
    if (m_viewport.IsClientLoadActive()) {
        const ClientLoadProgress& p = m_viewport.GetClientLoadProgress();
        ImGui::OpenPopup("Loading Client Data");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::BeginPopupModal("Loading Client Data", nullptr, flags)) {
            ImGui::Text("Loading client data...");
            ImGui::ProgressBar(p.fraction, ImVec2(380, 0));
            ImGui::TextWrapped("%s", p.stage.c_str());
            if (p.totalSteps > 1 && p.step > 0)
                ImGui::TextDisabled("%d / %d", p.step, p.totalSteps);
            ImGui::EndPopup();
        }
        return;
    }

    if (!m_clientLoadFailed) return;

    ImGui::OpenPopup("Client Load Failed");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Client Load Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", m_clientLoadError.c_str());
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            m_clientLoadFailed = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
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

bool Editor::CanUndo() const {
    if (m_ctx.sroRegionManager && m_ctx.sroRegionManager->CanUndo())
        return true;
    return m_ctx.commandHistory.CanUndo();
}

bool Editor::CanRedo() const {
    if (m_ctx.sroRegionManager && m_ctx.sroRegionManager->CanRedo())
        return true;
    return m_ctx.commandHistory.CanRedo();
}

void Editor::Undo() {
    if (m_ctx.sroRegionManager && m_ctx.sroRegionManager->CanUndo()) {
        m_ctx.sroRegionManager->Undo();
        m_ctx.MarkModified();
        m_ctx.selectedObjectName = m_ctx.SelectionDisplayName();
        Logger::Instance().Info("Undo SRO edit.");
        return;
    }
    m_ctx.commandHistory.Undo();
}

void Editor::Redo() {
    if (m_ctx.sroRegionManager && m_ctx.sroRegionManager->CanRedo()) {
        m_ctx.sroRegionManager->Redo();
        m_ctx.MarkModified();
        m_ctx.selectedObjectName = m_ctx.SelectionDisplayName();
        Logger::Instance().Info("Redo SRO edit.");
        return;
    }
    m_ctx.commandHistory.Redo();
}

bool Editor::DuplicateSelection() {
    if (!m_ctx.selection || m_ctx.selection->kind != EntityKind::MapPlacement || !m_ctx.sroRegionManager || !m_ctx.sroPlacements)
        return false;

    PlacementVM* source = m_ctx.FindPlacementBySelection();
    if (!source) return false;

    const int sourceRx = source->LoadedRx;
    const int sourceRy = source->LoadedRy;
    const int sourceBlockZ = source->BlockZ;
    const int sourceBlockX = source->BlockX;
    const int sourceLod = source->Lod;
    const int16_t sourceUid = source->Object.UID;
    const std::string sourceBsrPath = source->BsrPath;

    sro::formats::MapObject obj = source->Object;
    const int16_t newUid = NextPlacementUid(*m_ctx.sroPlacements, sourceRx, sourceRy);
    if (newUid == 0) {
        Logger::Instance().Error("Cannot duplicate placement: no free UID in loaded region.");
        return false;
    }

    obj.UID = newUid;
    obj.PosX += 20.0f;
    obj.PosZ += 20.0f;
    if (obj.RegionID == 0)
        obj.RegionID = static_cast<uint16_t>(EditorContext::EncodeRegionId(sourceRx, sourceRy));

    m_ctx.sroRegionManager->PushAction(std::make_unique<SpawnObjectAction>(
        obj, sourceBsrPath, sourceBlockZ, sourceBlockX, sourceLod));
    m_ctx.Select({EntityKind::MapPlacement,
        EditorContext::EncodeRegionId(sourceRx, sourceRy),
        std::to_string(obj.UID)});
    m_ctx.MarkModified();
    Logger::Instance().Info("Duplicated placement UID " + std::to_string(sourceUid)
        + " -> " + std::to_string(obj.UID));
    return true;
}

bool Editor::DeleteSelection() {
    if (!m_ctx.selection || m_ctx.selection->kind != EntityKind::MapPlacement || !m_ctx.sroRegionManager)
        return false;

    PlacementVM* source = m_ctx.FindPlacementBySelection();
    if (!source) return false;

    const int16_t deletedUid = source->Object.UID;
    m_ctx.sroRegionManager->PushAction(std::make_unique<DeleteObjectAction>(
        source->Object, source->BsrPath, source->BlockZ, source->BlockX, source->Lod, source->Index));
    m_ctx.ClearSelection();
    m_ctx.MarkModified();
    Logger::Instance().Info("Deleted placement UID " + std::to_string(deletedUid));
    return true;
}

bool Editor::SaveChangedClientFiles() {
    if (!m_ctx.sroClientLoaded || !m_ctx.sroRegionManager) {
        Logger::Instance().Warning("No SRO client data is loaded.");
        return false;
    }
    if (!m_ctx.sroRegionManager->HasDirtyClientFiles()) {
        Logger::Instance().Info("No changed SRO client files to save.");
        return true;
    }
    const size_t dirtyCount = m_ctx.sroRegionManager->DirtyFileCount();
    const bool ok = m_ctx.sroRegionManager->SaveDirtyClientFiles();
    if (ok) {
        m_ctx.project.modified = false;
        Logger::Instance().Info("Saved " + std::to_string(dirtyCount) + " changed SRO client file(s).");
    } else {
        Logger::Instance().Error("Failed to save one or more changed SRO client files.");
    }
    return ok;
}

bool Editor::ExportChangedClientFiles() {
    if (!m_ctx.sroClientLoaded || !m_ctx.sroRegionManager) {
        Logger::Instance().Warning("No SRO client data is loaded.");
        return false;
    }
    if (!m_ctx.sroRegionManager->HasDirtyClientFiles()) {
        Logger::Instance().Info("No changed SRO client files to export.");
        return true;
    }

    std::wstring outputPath;
    if (!m_ctx.project.outputPath.empty() && FileExistsA(m_ctx.project.outputPath)) {
        outputPath = ToWide(m_ctx.project.outputPath);
    } else if (!PickFolder(m_hwnd, L"Select output folder for changed SRO client files", outputPath)) {
        return false;
    }

    const bool ok = m_ctx.sroRegionManager->ExportDirtyClientFiles(outputPath);
    if (ok) {
        m_ctx.project.outputPath = ToNarrow(outputPath);
        Logger::Instance().Info("Exported changed SRO client files to: " + m_ctx.project.outputPath);
    } else {
        Logger::Instance().Error("Failed to export one or more changed SRO client files.");
    }
    return ok;
}
