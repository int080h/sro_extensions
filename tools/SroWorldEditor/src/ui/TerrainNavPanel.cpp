#include "ui/UiCommon.h"
#include "ui/TileMapCanvas.h"
#include "ui/HeightMapCanvas.h"
#include "ui/NavLayerUi.h"
#include "EditorPublic.h"
#include "sro/nav/ObjectNavSpatial.h"
#include "runtime/RegionManager.h"
#include "formats/NavMeshFormat.h"
#include "rendering/RenderManager.h"
#include "rendering/NvmRenderer.h"
#include "nav/NavMeshGenerator.h"
#include "nav/NavMeshEditorService.h"
#include "nav/NavMeshSession.h"
#include "io/PathHelpers.h"
#include "core/Logger.h"
#include "core/FileSystem.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <string>

static sro::formats::NavMesh* GetActiveNavMesh(EditorContext& ctx) {
    if (!ctx.sroRegionManager) return nullptr;
    return ctx.sroRegionManager->GetCenterNavMesh();
}

static void MarkNavmeshDirty(EditorContext& ctx) {
    if (!ctx.sroRegionManager) return;
    int rx = ctx.sroRegionManager->GetCurrentRx();
    int ry = ctx.sroRegionManager->GetCurrentRy();
    ctx.sroRegionManager->MarkNavmeshDirty(rx, ry);
    ctx.MarkModified();
}

static void RebuildGpuBuffers(EditorContext& ctx) {
    if (!ctx.sroRegionManager || !ctx.sroRenderManager) return;
    int rx = ctx.sroRegionManager->GetCurrentRx();
    int ry = ctx.sroRegionManager->GetCurrentRy();
    auto* nav = ctx.sroRegionManager->GetNavMesh(rx, ry);
    if (nav && ctx.sroRenderManager->GetNvmRenderer()) {
        ctx.sroRenderManager->GetNvmRenderer()->RebuildNavmeshBuffers(nav, rx, ry);
    }
}

static bool BeginNavmeshSection(const char* label) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);
    ImGui::PopStyleVar();
    return open;
}

static void CountTiles(const sro::formats::NavMesh& nav, int& walkable, int& blocked, int& oob) {
    walkable = blocked = oob = 0;
    for (const auto& tile : nav.TileMap) {
        if (Ui::IsTileOob(tile.Flags)) ++oob;
        else if (Ui::IsTileBlocked(tile.Flags)) ++blocked;
        else ++walkable;
    }
}

static void DrawOverviewTab(EditorContext& ctx, sro::formats::NavMesh* nav) {
    if (!nav) {
        ImGui::TextDisabled("No navmesh loaded for current region.");
        return;
    }

    int walkable = 0, blocked = 0, oob = 0;
    CountTiles(*nav, walkable, blocked, oob);
    ImGui::Text("Tiles: walkable %d  blocked %d  oob %d  |  Cells %d  Objects %d",
        walkable, blocked, oob, (int)nav->Cells.size(), (int)nav->Objects.size());
    ImGui::TextDisabled("Terrain blocked = tile Flag bit 0. Object BMS passability is on outline edges — use Blocking view or Combined passability map.");
    if (ctx.navmeshEditor.blockingViewActive || ctx.navmeshEditor.tileColorMode == 4) {
        ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.f), "Green=terrain corridor");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.5f, 1.f), "Teal=walkable platform");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "Orange=building BMS");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "Red=terrain block");
    }
    ImGui::TextDisabled("3D: Orange=building volume  Green=walkable platform  Red=nav edges");
    if (nav->TilesAre4Byte)
        ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "4-byte tile variant (Flags default 0)");

    ImGui::Separator();
    Ui::DrawTileMapCanvas(ctx, *nav, false);

    if (BeginNavmeshSection("NavMesh Statistics")) {
        ImGui::Indent(8.0f);
        ImGui::Text("Signature: %s", nav->Signature.c_str());
        ImGui::Separator();
        ImGui::Text("Objects:        %d", (int)nav->Objects.size());
        ImGui::Text("Cells:          %d / %d (open)", (int)nav->Cells.size(), nav->OpenCellCount);
        ImGui::Text("Total Cells:    %d", nav->TotalCellCount);
        ImGui::Text("Global Edges:   %d", (int)nav->GlobalEdges.size());
        ImGui::Text("Internal Edges: %d", (int)nav->InternalEdges.size());
        ImGui::Text("Tile Map:       96 x 96 = %d tiles", 96 * 96);
        ImGui::Text("Height Map:     %d values", (int)nav->HeightMap.size());
        ImGui::Text("Plane Types:    %d", (int)nav->PlaneTypeMap.size());
        ImGui::Text("Plane Heights:  %d", (int)nav->PlaneHeightMap.size());
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Pathfind Test")) {
        ImGui::Indent(8.0f);
        auto& st = ctx.navmeshEditor;
        ImGui::InputInt("Start Cell", &st.pathfindStartCell);
        ImGui::InputInt("End Cell", &st.pathfindEndCell);
        if (ImGui::Button("Run Pathfind")) {
            st.pathfindResult = sro::nav::NavMeshEditorService::FindPathBFS(*nav, st.pathfindStartCell, st.pathfindEndCell);
            if (st.pathfindResult.empty())
                Logger::Instance().Warning("NavMesh: No path between cells");
            else
                Logger::Instance().Info("NavMesh: Path with " + std::to_string(st.pathfindResult.size()) + " cells");
        }
        if (!st.pathfindResult.empty()) {
            std::string pathStr;
            for (size_t i = 0; i < st.pathfindResult.size(); ++i) {
                if (i) pathStr += " -> ";
                pathStr += std::to_string(st.pathfindResult[i]);
            }
            ImGui::TextWrapped("%s", pathStr.c_str());
        }
        ImGui::Unindent(8.0f);
    }
}

static void DrawObjectsTab(EditorContext& ctx, sro::formats::NavMesh* nav) {
    if (!nav) {
        ImGui::TextDisabled("No navmesh loaded.");
        return;
    }

    auto& st = ctx.navmeshEditor;

    ImGui::InputText("Search", st.objectSearch, sizeof(st.objectSearch));
    ImGui::SameLine();
    if (ImGui::Button("Add Object")) {
        sro::formats::NavObject newObj;
        newObj.AssetID = 0;
        newObj.PosX = 960.0f;
        newObj.PosY = 0.0f;
        newObj.PosZ = 960.0f;
        newObj.Type = -1;
        newObj.Yaw = 0.0f;
        newObj.LocalUID = static_cast<int16_t>(nav->Objects.size());
        newObj.Short0 = 0;
        newObj.IsBig = 0;
        newObj.IsStruct = 0;
        newObj.RegionID = static_cast<uint16_t>(EditorContext::EncodeRegionId(
            ctx.sroRegionManager->GetCurrentRx(), ctx.sroRegionManager->GetCurrentRy()));
        nav->Objects.push_back(newObj);
        st.selectedObjectIdx = (int)nav->Objects.size() - 1;
        MarkNavmeshDirty(ctx);
        RebuildGpuBuffers(ctx);
        Logger::Instance().Info("NavMesh: Added new object at index " + std::to_string(st.selectedObjectIdx));
    }

    ImGui::Separator();

    ImGui::BeginChild("NavObjList", ImVec2(280, -1), true);
    for (int i = 0; i < (int)nav->Objects.size(); ++i) {
        const auto& obj = nav->Objects[i];
        if (st.objectSearch[0] != '\0') {
            std::string label = std::to_string(obj.AssetID) + " UID:" + std::to_string(obj.LocalUID);
            if (!strstr(label.c_str(), st.objectSearch)) continue;
        }
        char label[128];
        snprintf(label, sizeof(label), "Obj[%d] AssetID=%d UID=%d%s", i, obj.AssetID, obj.LocalUID,
                 obj.Type == -1 ? " [S]" : (obj.Type == 0 ? " [Skin]" : ""));
        bool selected = (st.selectedObjectIdx == i);
        if (ImGui::Selectable(label, selected)) {
            st.selectedObjectIdx = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("NavObjProps", ImVec2(0, -1), true);
    if (st.selectedObjectIdx >= 0 && st.selectedObjectIdx < (int)nav->Objects.size()) {
        auto& obj = nav->Objects[st.selectedObjectIdx];

        ImGui::PushItemWidth(180);
        ImGui::InputInt("AssetID", &obj.AssetID);
        ImGui::InputFloat("Pos X", &obj.PosX);
        ImGui::InputFloat("Pos Y", &obj.PosY);
        ImGui::InputFloat("Pos Z", &obj.PosZ);
        ImGui::InputScalar("Type", ImGuiDataType_S16, &obj.Type);
        ImGui::InputFloat("Yaw", &obj.Yaw);
        ImGui::InputScalar("LocalUID", ImGuiDataType_S16, &obj.LocalUID);
        ImGui::InputScalar("Short0", ImGuiDataType_S16, &obj.Short0);
        bool isBig = obj.IsBig != 0;
        if (ImGui::Checkbox("Is Big", &isBig)) obj.IsBig = static_cast<uint8_t>(isBig ? 1 : 0);
        bool isStruct = obj.IsStruct != 0;
        if (ImGui::Checkbox("Is Struct", &isStruct)) obj.IsStruct = static_cast<uint8_t>(isStruct ? 1 : 0);
        int regionId = obj.RegionID;
        if (ImGui::InputInt("Region ID", &regionId)) obj.RegionID = static_cast<uint16_t>(regionId);
        ImGui::Text("WorldUID: 0x%08X", obj.WorldUID());

        if (ctx.sroPlacements) {
            const PlacementVM* linked = nullptr;
            for (const auto& p : *ctx.sroPlacements) {
                if (p.Object.UID == obj.LocalUID
                    && p.LoadedRx == ctx.sroRegionManager->GetCurrentRx()
                    && p.LoadedRy == ctx.sroRegionManager->GetCurrentRy()) {
                    linked = &p;
                    break;
                }
            }
            if (linked) {
                if (linked->Collision.NavMesh) {
                    const auto kind = sro::nav::ClassifyPlacementNav(*linked->Collision.NavMesh);
                    const auto edges = sro::nav::CountBmsEdges(*linked->Collision.NavMesh);
                    const int cellCount = (int)linked->Collision.NavMesh->Cells.size();
                    ImGui::TextColored(
                        kind == sro::nav::BmsObjectNavKind::WalkablePlatform
                            ? ImVec4(0.4f, 1.f, 0.5f, 1.f) : ImVec4(1.f, 0.6f, 0.3f, 1.f),
                        "BMS nav: %s — %d cells, %d open / %d blocked outline edges",
                        sro::nav::BmsObjectNavKindLabel(kind), cellCount,
                        edges.outlineOpenTerrain, edges.outlineBlocked);
                } else if (!linked->Collision.CollisionMeshPath.empty()) {
                    ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "BMS path: %s (not loaded)",
                        linked->Collision.CollisionMeshPath.c_str());
                } else if (!linked->BsrPath.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.f), "No BMS NavMeshOffset on model");
                }
            } else {
                ImGui::TextDisabled("No map placement matched by LocalUID in this region");
            }
        }

        ImGui::PopItemWidth();

        MarkNavmeshDirty(ctx);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Object Edges (%d):", (int)obj.Edges.size());

        if (ImGui::Button("Add Edge")) {
            sro::formats::NavObjectEdge newEdge;
            newEdge.LinkedObjID = 0;
            newEdge.LinkedObjEdgeID = 0;
            newEdge.EdgeID = static_cast<int16_t>(obj.Edges.size());
            obj.Edges.push_back(newEdge);
            MarkNavmeshDirty(ctx);
        }

        ImGui::BeginChild("ObjEdgeList", ImVec2(-1, 0), false);
        for (int e = 0; e < (int)obj.Edges.size(); ++e) {
            ImGui::PushID(e);
            auto& edge = obj.Edges[e];
            ImGui::Separator();
            ImGui::Text("Edge #%d", e);
            ImGui::PushItemWidth(120);
            ImGui::InputScalar("Linked Obj ID", ImGuiDataType_S16, &edge.LinkedObjID);
            ImGui::InputScalar("Linked Obj Edge", ImGuiDataType_S16, &edge.LinkedObjEdgeID);
            ImGui::InputScalar("Edge ID", ImGuiDataType_S16, &edge.EdgeID);
            ImGui::PopItemWidth();
            if (ImGui::SmallButton("Delete Edge")) {
                obj.Edges.erase(obj.Edges.begin() + e);
                MarkNavmeshDirty(ctx);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete This Object")) {
            Logger::Instance().Info("NavMesh: Deleted object at index " + std::to_string(st.selectedObjectIdx));
            nav->Objects.erase(nav->Objects.begin() + st.selectedObjectIdx);
            st.selectedObjectIdx = -1;
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::TextDisabled("Select an object to edit its properties.");
    }
    ImGui::EndChild();
}

static void DrawCellsTab(EditorContext& ctx, sro::formats::NavMesh* nav) {
    if (!nav) {
        ImGui::TextDisabled("No navmesh loaded.");
        return;
    }

    auto& st = ctx.navmeshEditor;

    ImGui::Text("Cells: %d", (int)nav->Cells.size());
    ImGui::SameLine();
    if (ImGui::Button("Add Cell")) {
        sro::formats::NavCell newCell;
        newCell.MinX = 0.0f;
        newCell.MinY = 0.0f;
        newCell.MaxX = 100.0f;
        newCell.MaxY = 100.0f;
        nav->Cells.push_back(newCell);
        nav->TotalCellCount = (uint32_t)nav->Cells.size();
        st.selectedCellIdx = (int)nav->Cells.size() - 1;
        MarkNavmeshDirty(ctx);
        RebuildGpuBuffers(ctx);
        Logger::Instance().Info("NavMesh: Added new cell at index " + std::to_string(st.selectedCellIdx));
    }

    ImGui::Separator();

    ImGui::BeginChild("NavCellList", ImVec2(260, -1), true);
    for (int i = 0; i < (int)nav->Cells.size(); ++i) {
        const auto& cell = nav->Cells[i];
        char label[128];
        snprintf(label, sizeof(label), "Cell[%d] (%.0f,%.0f)-(%.0f,%.0f) objs:%d",
                 i, cell.MinX, cell.MinY, cell.MaxX, cell.MaxY, (int)cell.ObjIndices.size());
        bool selected = (st.selectedCellIdx == i);
        if (ImGui::Selectable(label, selected)) {
            st.selectedCellIdx = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("NavCellProps", ImVec2(0, -1), true);
    if (st.selectedCellIdx >= 0 && st.selectedCellIdx < (int)nav->Cells.size()) {
        auto& cell = nav->Cells[st.selectedCellIdx];

        ImGui::PushItemWidth(160);
        bool boundsChanged = false;
        boundsChanged |= ImGui::InputFloat("Min X", &cell.MinX);
        boundsChanged |= ImGui::InputFloat("Min Y", &cell.MinY);
        boundsChanged |= ImGui::InputFloat("Max X", &cell.MaxX);
        boundsChanged |= ImGui::InputFloat("Max Y", &cell.MaxY);
        if (boundsChanged) {
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Object Indices (%d):", (int)cell.ObjIndices.size());

        if (ImGui::Button("Add Object Index")) {
            cell.ObjIndices.push_back(0);
            MarkNavmeshDirty(ctx);
        }

        ImGui::BeginChild("CellObjIdxList", ImVec2(-1, 0), false);
        for (int o = 0; o < (int)cell.ObjIndices.size(); ++o) {
            ImGui::PushID(o);
            int val = cell.ObjIndices[o];
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputInt("##objidx", &val)) {
                cell.ObjIndices[o] = static_cast<uint16_t>(val);
                MarkNavmeshDirty(ctx);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                cell.ObjIndices.erase(cell.ObjIndices.begin() + o);
                MarkNavmeshDirty(ctx);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete This Cell")) {
            Logger::Instance().Info("NavMesh: Deleted cell at index " + std::to_string(st.selectedCellIdx));
            nav->Cells.erase(nav->Cells.begin() + st.selectedCellIdx);
            nav->TotalCellCount = (uint32_t)nav->Cells.size();
            st.selectedCellIdx = -1;
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::TextDisabled("Select a cell to edit its properties.");
    }
    ImGui::EndChild();
}

static void DrawGlobalEdgesTab(EditorContext& ctx, sro::formats::NavMesh* nav) {
    if (!nav) {
        ImGui::TextDisabled("No navmesh loaded.");
        return;
    }

    auto& st = ctx.navmeshEditor;

    ImGui::Text("Global Edges: %d", (int)nav->GlobalEdges.size());
    ImGui::SameLine();
    if (ImGui::Button("Add Global Edge")) {
        sro::formats::NavGlobalEdge newEdge;
        newEdge.MinX = 0.0f; newEdge.MinY = 0.0f;
        newEdge.MaxX = 100.0f; newEdge.MaxY = 100.0f;
        newEdge.Flags = 0;
        newEdge.D0 = 0; newEdge.D1 = 0;
        newEdge.C0 = 0; newEdge.C1 = 0;
        newEdge.R0 = 0; newEdge.R1 = 0;
        nav->GlobalEdges.push_back(newEdge);
        nav->GlobalEdgeCount = (uint32_t)nav->GlobalEdges.size();
        st.selectedGlobalEdgeIdx = (int)nav->GlobalEdges.size() - 1;
        MarkNavmeshDirty(ctx);
        RebuildGpuBuffers(ctx);
        Logger::Instance().Info("NavMesh: Added new global edge at index " + std::to_string(st.selectedGlobalEdgeIdx));
    }

    ImGui::Separator();

    ImGui::BeginChild("NavGEdgeList", ImVec2(280, -1), true);
    for (int i = 0; i < (int)nav->GlobalEdges.size(); ++i) {
        const auto& edge = nav->GlobalEdges[i];
        char label[128];
        snprintf(label, sizeof(label), "GEdge[%d] (%.0f,%.0f)-(%.0f,%.0f) F:%d",
                 i, edge.MinX, edge.MinY, edge.MaxX, edge.MaxY, edge.Flags);
        bool selected = (st.selectedGlobalEdgeIdx == i);
        if (ImGui::Selectable(label, selected)) {
            st.selectedGlobalEdgeIdx = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("NavGEdgeProps", ImVec2(0, -1), true);
    if (st.selectedGlobalEdgeIdx >= 0 && st.selectedGlobalEdgeIdx < (int)nav->GlobalEdges.size()) {
        auto& edge = nav->GlobalEdges[st.selectedGlobalEdgeIdx];

        ImGui::PushItemWidth(160);
        bool changed = false;
        changed |= ImGui::InputFloat("Min X", &edge.MinX);
        changed |= ImGui::InputFloat("Min Y", &edge.MinY);
        changed |= ImGui::InputFloat("Max X", &edge.MaxX);
        changed |= ImGui::InputFloat("Max Y", &edge.MaxY);
        int flags = edge.Flags;
        if (ImGui::InputInt("Flags", &flags)) { edge.Flags = static_cast<uint8_t>(flags); changed = true; }
        int d0 = edge.D0; if (ImGui::InputInt("D0", &d0)) { edge.D0 = static_cast<int8_t>(d0); changed = true; }
        int d1 = edge.D1; if (ImGui::InputInt("D1", &d1)) { edge.D1 = static_cast<int8_t>(d1); changed = true; }
        ImGui::InputScalar("C0", ImGuiDataType_S16, &edge.C0);
        ImGui::InputScalar("C1", ImGuiDataType_S16, &edge.C1);
        ImGui::InputScalar("R0", ImGuiDataType_S16, &edge.R0);
        ImGui::InputScalar("R1", ImGuiDataType_S16, &edge.R1);
        ImGui::PopItemWidth();

        if (changed) {
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete This Global Edge")) {
            Logger::Instance().Info("NavMesh: Deleted global edge at index " + std::to_string(st.selectedGlobalEdgeIdx));
            nav->GlobalEdges.erase(nav->GlobalEdges.begin() + st.selectedGlobalEdgeIdx);
            nav->GlobalEdgeCount = (uint32_t)nav->GlobalEdges.size();
            st.selectedGlobalEdgeIdx = -1;
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::TextDisabled("Select a global edge to edit.");
    }
    ImGui::EndChild();
}

static void DrawInternalEdgesTab(EditorContext& ctx, sro::formats::NavMesh* nav) {
    if (!nav) {
        ImGui::TextDisabled("No navmesh loaded.");
        return;
    }

    auto& st = ctx.navmeshEditor;

    ImGui::Text("Internal Edges: %d", (int)nav->InternalEdges.size());
    ImGui::SameLine();
    if (ImGui::Button("Add Internal Edge")) {
        sro::formats::NavInternalEdge newEdge;
        newEdge.MinX = 0.0f; newEdge.MinY = 0.0f;
        newEdge.MaxX = 100.0f; newEdge.MaxY = 100.0f;
        newEdge.Flags = 0;
        newEdge.D0 = 0; newEdge.D1 = 0;
        newEdge.C0 = 0; newEdge.C1 = 0;
        nav->InternalEdges.push_back(newEdge);
        nav->InternalEdgeCount = (uint32_t)nav->InternalEdges.size();
        st.selectedInternalEdgeIdx = (int)nav->InternalEdges.size() - 1;
        MarkNavmeshDirty(ctx);
        RebuildGpuBuffers(ctx);
        Logger::Instance().Info("NavMesh: Added new internal edge at index " + std::to_string(st.selectedInternalEdgeIdx));
    }

    ImGui::Separator();

    ImGui::BeginChild("NavIEdgeList", ImVec2(280, -1), true);
    for (int i = 0; i < (int)nav->InternalEdges.size(); ++i) {
        const auto& edge = nav->InternalEdges[i];
        char label[128];
        snprintf(label, sizeof(label), "IEdge[%d] (%.0f,%.0f)-(%.0f,%.0f) F:%d",
                 i, edge.MinX, edge.MinY, edge.MaxX, edge.MaxY, edge.Flags);
        bool selected = (st.selectedInternalEdgeIdx == i);
        if (ImGui::Selectable(label, selected)) {
            st.selectedInternalEdgeIdx = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("NavIEdgeProps", ImVec2(0, -1), true);
    if (st.selectedInternalEdgeIdx >= 0 && st.selectedInternalEdgeIdx < (int)nav->InternalEdges.size()) {
        auto& edge = nav->InternalEdges[st.selectedInternalEdgeIdx];

        ImGui::PushItemWidth(160);
        bool changed = false;
        changed |= ImGui::InputFloat("Min X", &edge.MinX);
        changed |= ImGui::InputFloat("Min Y", &edge.MinY);
        changed |= ImGui::InputFloat("Max X", &edge.MaxX);
        changed |= ImGui::InputFloat("Max Y", &edge.MaxY);
        int flags = edge.Flags;
        if (ImGui::InputInt("Flags", &flags)) { edge.Flags = static_cast<uint8_t>(flags); changed = true; }
        int d0 = edge.D0; if (ImGui::InputInt("D0", &d0)) { edge.D0 = static_cast<int8_t>(d0); changed = true; }
        int d1 = edge.D1; if (ImGui::InputInt("D1", &d1)) { edge.D1 = static_cast<int8_t>(d1); changed = true; }
        ImGui::InputScalar("C0", ImGuiDataType_S16, &edge.C0);
        ImGui::InputScalar("C1", ImGuiDataType_S16, &edge.C1);
        ImGui::PopItemWidth();

        if (changed) {
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete This Internal Edge")) {
            Logger::Instance().Info("NavMesh: Deleted internal edge at index " + std::to_string(st.selectedInternalEdgeIdx));
            nav->InternalEdges.erase(nav->InternalEdges.begin() + st.selectedInternalEdgeIdx);
            nav->InternalEdgeCount = (uint32_t)nav->InternalEdges.size();
            st.selectedInternalEdgeIdx = -1;
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::TextDisabled("Select an internal edge to edit.");
    }
    ImGui::EndChild();
}

static void DrawTilesTab(EditorContext& ctx, sro::formats::NavMesh* nav) {
    if (!nav) {
        ImGui::TextDisabled("No navmesh loaded.");
        return;
    }

    auto& st = ctx.navmeshEditor;

    if (BeginNavmeshSection("Tile Paint Tool")) {
        ImGui::Indent(8.0f);
        const char* modes[] = {"Set Blocked", "Set Walkable"};
        ImGui::Combo("Paint Mode", &st.paintMode, modes, 2);
        ImGui::SliderFloat("Brush Size", &st.brushSize, 1.0f, 10.0f);
        ImGui::TextDisabled("Paint on grid below (LMB drag / RMB toggle). Collision Paint in viewport also works.");
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Batch Operations")) {
        ImGui::Indent(8.0f);
        if (ImGui::Button("Set All Walkable")) {
            for (auto& tile : nav->TileMap) {
                if (tile.Flags != 0xFFFF) tile.Flags &= 0xFFFE;
            }
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
            Logger::Instance().Info("NavMesh: Set all tiles walkable");
        }
        ImGui::SameLine();
        if (ImGui::Button("Set All Blocked")) {
            for (auto& tile : nav->TileMap) {
                if (tile.Flags != 0xFFFF) tile.Flags |= 0x0001;
            }
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
            Logger::Instance().Info("NavMesh: Set all tiles blocked");
        }
        ImGui::Spacing();
        ImGui::TextDisabled("The TileMap is the game's authored blocking grid (20u tiles).");
        ImGui::TextDisabled("CellQuads (pathfinding nodes) are generated from it. Object collision");
        ImGui::TextDisabled("is the per-object RTNavMeshObj -- toggle it in the Collision Editor.");
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Tile Inspector")) {
        ImGui::Indent(8.0f);
        ImGui::Text("Click a tile in the grid below or enter index manually:");
        ImGui::PushItemWidth(120);
        ImGui::InputInt("Tile Index", &st.selectedTileIdx);
        ImGui::PopItemWidth();

        if (st.selectedTileIdx >= 0 && st.selectedTileIdx < (int)nav->TileMap.size()) {
            auto& tile = nav->TileMap[st.selectedTileIdx];
            int tx = st.selectedTileIdx % 96;
            int tz = st.selectedTileIdx / 96;
            ImGui::Text("Position: (%d, %d)", tx, tz);

            ImGui::PushItemWidth(120);
            int cellId = tile.CellID;
            if (ImGui::InputInt("Cell ID", &cellId)) {
                tile.CellID = static_cast<int32_t>(cellId);
                MarkNavmeshDirty(ctx);
            }
            int flags = tile.Flags;
            if (ImGui::InputInt("Flags", &flags)) {
                tile.Flags = static_cast<uint16_t>(flags);
                MarkNavmeshDirty(ctx);
                RebuildGpuBuffers(ctx);
            }
            bool isBlocked = (tile.Flags == 0xFFFF) || (tile.Flags & 0x0001);
            if (ImGui::Checkbox("Blocked", &isBlocked)) {
                if (isBlocked) tile.Flags |= 0x0001;
                else tile.Flags &= 0xFFFE;
                MarkNavmeshDirty(ctx);
                RebuildGpuBuffers(ctx);
            }
            bool isOob = (tile.Flags == 0xFFFF);
            ImGui::Checkbox("Out of Bounds", &isOob);
            int texId = tile.TextureID;
            if (ImGui::InputInt("Texture ID", &texId)) {
                tile.TextureID = static_cast<uint16_t>(texId);
                MarkNavmeshDirty(ctx);
            }
            ImGui::PopItemWidth();
        }
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Tile Grid (96x96)")) {
        ImGui::Indent(8.0f);
        Ui::DrawTileMapCanvas(ctx, *nav, true);
        ImGui::Unindent(8.0f);
    }
}

static void DrawHeightmapTab(EditorContext& ctx, sro::formats::NavMesh* nav) {
    if (!nav) {
        ImGui::TextDisabled("No navmesh loaded.");
        return;
    }

    static int editIdx = 0;
    static float editValue = 0.0f;

    if (BeginNavmeshSection("Heightmap Info")) {
        ImGui::Indent(8.0f);
        ImGui::Text("Grid Size: 97 x 97 = %d values", 97 * 97);
        ImGui::Text("Stored Values: %d", (int)nav->HeightMap.size());
        if (!nav->HeightMap.empty()) {
            float minH = nav->HeightMap[0], maxH = nav->HeightMap[0], sum = 0;
            for (float h : nav->HeightMap) {
                if (h < minH) minH = h;
                if (h > maxH) maxH = h;
                sum += h;
            }
            ImGui::Text("Min: %.2f", minH);
            ImGui::Text("Max: %.2f", maxH);
            ImGui::Text("Avg: %.2f", sum / nav->HeightMap.size());
        }
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Heightmap Heatmap (97x97)")) {
        ImGui::Indent(8.0f);
        Ui::DrawHeightMapCanvas(*nav, &editIdx);
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Edit Heightmap Vertex")) {
        ImGui::Indent(8.0f);
        ImGui::PushItemWidth(120);
        ImGui::InputInt("Index (0..96*97-1)", &editIdx);
        if (editIdx >= 0 && editIdx < (int)nav->HeightMap.size()) {
            editValue = nav->HeightMap[editIdx];
            if (ImGui::InputFloat("Height", &editValue)) {
                nav->HeightMap[editIdx] = editValue;
                MarkNavmeshDirty(ctx);
                RebuildGpuBuffers(ctx);
            }
        } else {
            ImGui::TextDisabled("Index out of range");
        }
        ImGui::PopItemWidth();
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Batch Height Operations")) {
        ImGui::Indent(8.0f);
        static float offsetVal = 0.0f;
        ImGui::PushItemWidth(120);
        ImGui::InputFloat("Offset All", &offsetVal);
        ImGui::PopItemWidth();
        if (ImGui::Button("Apply Offset")) {
            for (auto& h : nav->HeightMap) h += offsetVal;
            MarkNavmeshDirty(ctx);
            RebuildGpuBuffers(ctx);
            Logger::Instance().Info("NavMesh: Applied height offset " + std::to_string(offsetVal));
        }
        ImGui::Unindent(8.0f);
    }

    if (BeginNavmeshSection("Plane Data")) {
        ImGui::Indent(8.0f);
        ImGui::Text("Plane Types (36):");
        for (int i = 0; i < (int)nav->PlaneTypeMap.size(); ++i) {
            ImGui::PushID(i + 1000);
            int val = nav->PlaneTypeMap[i];
            ImGui::SetNextItemWidth(60);
            if (ImGui::InputInt("##pt", &val)) {
                nav->PlaneTypeMap[i] = static_cast<uint8_t>(val);
                MarkNavmeshDirty(ctx);
            }
            if ((i + 1) % 6 != 0) ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::Spacing();
        ImGui::Text("Plane Heights (36):");
        for (int i = 0; i < (int)nav->PlaneHeightMap.size(); ++i) {
            ImGui::PushID(i + 2000);
            ImGui::SetNextItemWidth(80);
            if (ImGui::InputFloat("##ph", &nav->PlaneHeightMap[i])) {
                MarkNavmeshDirty(ctx);
            }
            if ((i + 1) % 6 != 0) ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::Unindent(8.0f);
    }
}

static void DrawNavToolbar(EditorContext& ctx, sro::formats::NavMesh* nav, int rx, int ry) {
    static sro::nav::NavMeshSession dummySession;
    static sro::nav::NavMeshEditorService validateService(dummySession);
    static std::string lastValidation;

    if (ImGui::Button("Blocking view")) {
        Ui::ApplyBlockingViewPreset(ctx);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Combined passability map + object nav volume (orange building, green platform) + red edges.");
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (ctx.sroRegionManager && ctx.sroRegionManager->ReloadNavMeshFromDisk(rx, ry))
            Logger::Instance().Info("Reloaded nv_" + std::to_string(rx) + std::to_string(ry) + ".nvm from disk");
        else
            Logger::Instance().Warning("Failed to reload navmesh from disk");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        if (ctx.sroRegionManager && ctx.sroRegionManager->SaveNavMeshToDisk(rx, ry))
            Logger::Instance().Info("Saved nv_" + std::to_string(rx) + std::to_string(ry) + ".nvm");
        else
            Logger::Instance().Warning("Failed to save navmesh");
    }
    ImGui::SameLine();
    if (ImGui::Button("Regenerate CellQuads") && nav) {
        sro::nav::NavMeshGenerator::RegenerateCells(*nav);
        MarkNavmeshDirty(ctx);
        RebuildGpuBuffers(ctx);
        Logger::Instance().Info("Regenerated CellQuads from TileMap");
    }
    ImGui::SameLine();
    if (ImGui::Button("Validate") && nav) {
        auto result = validateService.ValidateNavMesh(*nav);
        lastValidation.clear();
        if (result.ok) lastValidation = "OK";
        for (const auto& issue : result.issues)
            lastValidation += (lastValidation.empty() ? "" : "\n") + issue.message + (issue.isError ? " [error]" : " [warn]");
        if (!result.ok) Logger::Instance().Warning("NavMesh validation failed");
        else Logger::Instance().Info("NavMesh validation passed");
    }
    if (!lastValidation.empty())
        ImGui::TextWrapped("%s", lastValidation.c_str());
}

void Ui::DrawTerrainNavPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Terrain Nav", &ctx.panels.terrainNavPanel)) { ImGui::End(); return; }
    auto* nav = GetActiveNavMesh(ctx);

    int rx = 0, ry = 0;
    if (ctx.sroRegionManager) {
        rx = ctx.sroRegionManager->GetCurrentRx();
        ry = ctx.sroRegionManager->GetCurrentRy();
        char fileLabel[32];
        snprintf(fileLabel, sizeof(fileLabel), "nv_%02x%02x.nvm", rx, ry);
        ImGui::Text("Region (%d,%d)  %s", rx, ry, fileLabel);
        ImGui::SameLine();
        if (nav) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Loaded");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No NavMesh");
        }
    } else {
        ImGui::TextDisabled("No region manager (client not loaded)");
    }

    if (ctx.sroRegionManager)
        DrawNavToolbar(ctx, nav, rx, ry);

    ImGui::Separator();

    const char* tabNames[] = {
        "Analyze", "Terrain Data", "Cell Graph", "NVM Objects"
    };
    for (int i = 0; i < IM_ARRAYSIZE(tabNames); ++i) {
        if (i > 0) ImGui::SameLine();
        bool active = (ctx.navmeshEditor.activeTab == static_cast<NavMeshEditorTab>(i));
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(tabNames[i])) {
            ctx.navmeshEditor.activeTab = static_cast<NavMeshEditorTab>(i);
        }
        if (active) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Open Nav Layers")) ctx.panels.navLayersPanel = true;

    ImGui::Separator();

    switch (ctx.navmeshEditor.activeTab) {
    case NavMeshEditorTab::Analyze:
        DrawOverviewTab(ctx, nav);
        break;
    case NavMeshEditorTab::TerrainData: {
        if (ImGui::BeginTabBar("TerrainDataSub")) {
            if (ImGui::BeginTabItem("Tiles")) { DrawTilesTab(ctx, nav); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("HeightMap")) { DrawHeightmapTab(ctx, nav); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        break;
    }
    case NavMeshEditorTab::CellGraph: {
        if (ImGui::BeginTabBar("CellGraphSub")) {
            if (ImGui::BeginTabItem("CellQuads")) { DrawCellsTab(ctx, nav); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Internal Edges")) { DrawInternalEdgesTab(ctx, nav); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Global Edges")) { DrawGlobalEdgesTab(ctx, nav); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        break;
    }
    case NavMeshEditorTab::NvmObjects:
        DrawObjectsTab(ctx, nav);
        break;
    default: break;
    }

    ImGui::End();
}
