#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "nav/NavMeshSession.h"
#include "nav/NavMeshCatalog.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <string>

static const char* FileKindLabel(sro::nav::NavMeshFileKind kind) {
    switch (kind) {
    case sro::nav::NavMeshFileKind::TerrainNvm: return "Terrain NVM";
    case sro::nav::NavMeshFileKind::AiNavData: return "AI Nav Data";
    case sro::nav::NavMeshFileKind::MapInfo: return "MapInfo";
    case sro::nav::NavMeshFileKind::ObjectIfo: return "object.ifo";
    case sro::nav::NavMeshFileKind::ObjectStringIfo: return "objectstring.ifo";
    case sro::nav::NavMeshFileKind::ObjExtIfo: return "objext.ifo";
    case sro::nav::NavMeshFileKind::Tile2DIfo: return "tile2d.ifo";
    default: return "Other";
    }
}

static bool PassesFilter(const sro::nav::NavMeshFileEntry& entry, int filterKind, const char* search) {
    if (filterKind == 1 && entry.kind != sro::nav::NavMeshFileKind::TerrainNvm) return false;
    if (filterKind == 2 && entry.kind != sro::nav::NavMeshFileKind::AiNavData) return false;
    if (filterKind == 3 && entry.kind != sro::nav::NavMeshFileKind::MapInfo &&
        entry.kind != sro::nav::NavMeshFileKind::ObjectIfo &&
        entry.kind != sro::nav::NavMeshFileKind::ObjectStringIfo &&
        entry.kind != sro::nav::NavMeshFileKind::ObjExtIfo &&
        entry.kind != sro::nav::NavMeshFileKind::Tile2DIfo) return false;
    if (search && search[0] != '\0') {
        std::string name = ToNarrow(entry.fileName);
        if (!strstr(name.c_str(), search)) return false;
    }
    return true;
}

void Ui::DrawNavMeshBrowserPanel(EditorContext& ctx, Editor& editor) {
    if (!ImGui::Begin("NavMesh Browser", &ctx.panels.navMeshBrowser)) { ImGui::End(); return; }

    auto& st = ctx.navMeshBrowser;
    auto* session = ctx.navMeshSession;

    if (!session) {
        ImGui::TextDisabled("NavMesh session not available. Import client data first.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Rescan Folder")) {
        session->Scan();
        Logger::Instance().Info("NavMesh catalog rescanned.");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%d files", (int)session->Catalog().AllFiles().size());

    ImGui::SetNextItemWidth(160);
    const char* filters[] = {"All", "Terrain (.nvm)", "AI Nav (.dat)", "Index / IFO"};
    ImGui::Combo("Filter", &st.filterKind, filters, IM_ARRAYSIZE(filters));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("Search", st.searchFilter, sizeof(st.searchFilter));

    ImGui::Separator();

    if (ImGui::BeginTable("NavMeshFiles", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
            ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
        ImGui::TableSetupColumn("File");
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Region");
        ImGui::TableSetupColumn("Size");
        ImGui::TableHeadersRow();

        const auto& files = session->Catalog().AllFiles();
        for (int i = 0; i < (int)files.size(); ++i) {
            const auto& fe = files[i];
            if (!PassesFilter(fe, st.filterKind, st.searchFilter)) continue;

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            std::string name = ToNarrow(fe.fileName);
            bool selected = (st.selectedIndex == i);
            if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                st.selectedIndex = i;

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Open")) {
                    if (fe.kind == sro::nav::NavMeshFileKind::TerrainNvm) {
                        session->LoadTerrainNavMesh(fe.rx, fe.ry);
                        editor.Viewport().SetSroRegion(fe.rx, fe.ry);
                        ctx.panels.terrainNavPanel = true;
                        Logger::Instance().Info("Opened terrain navmesh " + std::to_string(fe.rx) + "," + std::to_string(fe.ry));
                    } else if (fe.kind == sro::nav::NavMeshFileKind::AiNavData) {
                        ctx.aiNavDataEditor.selectedRegionId = fe.regionId;
                        session->LoadAiNavData(fe.regionId);
                        ctx.panels.aiNavDataPanel = true;
                        Logger::Instance().Info("Opened AI nav data region " + std::to_string(fe.regionId));
                    } else {
                        Logger::Instance().Info("Selected: " + name);
                    }
                }
                if (fe.kind == sro::nav::NavMeshFileKind::TerrainNvm && ImGui::MenuItem("Delete")) {
                    if (session->DeleteTerrainNavMesh(fe.rx, fe.ry)) {
                        session->Scan();
                        Logger::Instance().Info("Deleted terrain navmesh " + std::to_string(fe.rx) + "," + std::to_string(fe.ry));
                    }
                }
                if (fe.kind == sro::nav::NavMeshFileKind::AiNavData && ImGui::MenuItem("Delete")) {
                    if (session->DeleteAiNavData(fe.regionId)) {
                        session->Scan();
                        Logger::Instance().Info("Deleted AI nav data region " + std::to_string(fe.regionId));
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(FileKindLabel(fe.kind));
            ImGui::TableNextColumn();
            if (fe.kind == sro::nav::NavMeshFileKind::TerrainNvm)
                ImGui::Text("(%d, %d)", fe.rx, fe.ry);
            else if (fe.kind == sro::nav::NavMeshFileKind::AiNavData)
                ImGui::Text("%u", fe.regionId);
            else
                ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (fe.fileSize >= 1024 * 1024)
                ImGui::Text("%.1f MB", fe.fileSize / (1024.f * 1024.f));
            else if (fe.fileSize >= 1024)
                ImGui::Text("%.1f KB", fe.fileSize / 1024.f);
            else
                ImGui::Text("%llu B", (unsigned long long)fe.fileSize);
        }
        ImGui::EndTable();
    }

    if (ImGui::Button("Create Terrain NVM...")) {
        st.showCreateTerrainPopup = true;
        st.createRx = ctx.sroRegionRx;
        st.createRy = ctx.sroRegionRy;
    }
    ImGui::SameLine();
    if (ImGui::Button("Create AI Nav Data...")) {
        st.showCreateAiNavPopup = true;
        st.createRegionId = static_cast<int>(ctx.aiNavDataEditor.selectedRegionId);
        if (st.createRegionId == 0)
            st.createRegionId = EditorContext::EncodeRegionId(ctx.sroRegionRx, ctx.sroRegionRy);
    }

    if (st.showCreateTerrainPopup)
        ImGui::OpenPopup("Create Terrain NVM");
    if (ImGui::BeginPopupModal("Create Terrain NVM", &st.showCreateTerrainPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputInt("Region X", &st.createRx);
        ImGui::InputInt("Region Y", &st.createRy);
        if (ImGui::Button("Create")) {
            if (session->CreateTerrainNavMesh(st.createRx, st.createRy)) {
                session->Scan();
                Logger::Instance().Info("Created terrain NVM " + std::to_string(st.createRx) + "," + std::to_string(st.createRy));
            }
            st.showCreateTerrainPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            st.showCreateTerrainPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (st.showCreateAiNavPopup)
        ImGui::OpenPopup("Create AI Nav Data");
    if (ImGui::BeginPopupModal("Create AI Nav Data", &st.showCreateAiNavPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputInt("Region ID", &st.createRegionId);
        if (ImGui::Button("Create")) {
            auto regionId = static_cast<uint16_t>(st.createRegionId);
            if (session->CreateAiNavData(regionId)) {
                session->Scan();
                ctx.aiNavDataEditor.selectedRegionId = regionId;
                Logger::Instance().Info("Created ainavdata_" + std::to_string(regionId) + ".dat");
            }
            st.showCreateAiNavPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            st.showCreateAiNavPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
