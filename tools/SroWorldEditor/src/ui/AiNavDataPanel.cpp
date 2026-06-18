#include "ui/UiCommon.h"
#include "nav/NavMeshSession.h"
#include "formats/AiNavDataFormat.h"
#include "core/Logger.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <queue>
#include <set>
#include <string>
#include <vector>

static bool BeginAiNavSection(const char* label) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);
    ImGui::PopStyleVar();
    return open;
}

static uint16_t RegionIdFromCoords(int rx, int ry) {
    return static_cast<uint16_t>(((rx & 0xFF) << 8) | (ry & 0xFF));
}

static bool IsDungeonRegionId(uint16_t regionId) {
    return (regionId & 0x8000) != 0;
}

static void DecodeRegionId(uint16_t regionId, int& rx, int& ry) {
    rx = (regionId >> 8) & 0xFF;
    ry = regionId & 0xFF;
}

static void TryAutoLoadAiNav(EditorContext& ctx, sro::nav::NavMeshSession& session, uint16_t regionId) {
    if (session.GetAiNavData(regionId)) return;
    if (!session.Catalog().HasAiNav(regionId)) return;
    if (session.LoadAiNavData(regionId)) {
        Logger::Instance().Info("Auto-loaded ainavdata for region " + std::to_string(regionId));
        ctx.aiNavDataEditor.selectedRegionId = regionId;
    }
}

static std::vector<int> FindBlockPathBFS(const sro::formats::AiNavRefDungeon& dungeon, int startBlock, int endBlock) {
    if (startBlock < 0 || startBlock >= (int)dungeon.Blocks.size()) return {};
    if (endBlock < 0 || endBlock >= (int)dungeon.Blocks.size()) return {};
    if (startBlock == endBlock) return {startBlock};

    std::vector<std::vector<int>> adjacency(dungeon.Blocks.size());
    for (int bi = 0; bi < (int)dungeon.Blocks.size(); ++bi) {
        for (const auto& link : dungeon.Blocks[bi].Links) {
            int target = static_cast<int>(link.LinkedObjID);
            if (target >= 0 && target < (int)dungeon.Blocks.size()) {
                adjacency[bi].push_back(target);
                adjacency[target].push_back(bi);
            }
        }
    }

    std::vector<int> parent(dungeon.Blocks.size(), -1);
    std::vector<bool> visited(dungeon.Blocks.size(), false);
    std::queue<int> q;
    q.push(startBlock);
    visited[startBlock] = true;

    while (!q.empty()) {
        int current = q.front();
        q.pop();
        if (current == endBlock) break;
        for (int neighbor : adjacency[current]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = current;
                q.push(neighbor);
            }
        }
    }

    if (!visited[endBlock]) return {};

    std::vector<int> path;
    for (int cur = endBlock; cur != -1; cur = parent[cur])
        path.push_back(cur);
    std::reverse(path.begin(), path.end());
    return path;
}

static void SyncRefDungeonCounts(sro::formats::AiNavRefDungeon& ref) {
    ref.BlockCount = static_cast<uint32_t>(ref.Blocks.size());
    for (auto& block : ref.Blocks) {
        block.CellCount = static_cast<uint32_t>(block.CellLookupTable.size());
        block.EdgeCount = static_cast<uint32_t>(block.Links.size());
    }
}

static void DrawRefDungeonBlocks(EditorContext& ctx, sro::formats::AiNavData& data, sro::nav::NavMeshSession& session) {
    auto& st = ctx.aiNavDataEditor;
    auto& ref = data.RefDungeon;

    ImGui::Text("RefDungeon blocks: %d", (int)ref.Blocks.size());
    ImGui::SameLine();
    if (ImGui::Button("Add Block")) {
        sro::formats::AiNavRefBlock block;
        block.Index = static_cast<uint32_t>(ref.Blocks.size());
        ref.Blocks.push_back(block);
        ref.BlockLookupTable.push_back(static_cast<int16_t>(ref.Blocks.size() - 1));
        SyncRefDungeonCounts(ref);
        st.selectedBlockIdx = (int)ref.Blocks.size() - 1;
        st.selectedLinkIdx = -1;
        session.MarkAiNavDirty(st.selectedRegionId);
        ctx.MarkModified();
    }

    ImGui::BeginChild("AiNavBlockList", ImVec2(240, 200), true);
    for (int i = 0; i < (int)ref.Blocks.size(); ++i) {
        const auto& b = ref.Blocks[i];
        char label[96];
        snprintf(label, sizeof(label), "Block[%d] idx=%u cells=%u links=%u",
                 i, b.Index, b.CellCount, b.EdgeCount);
        bool selected = (st.selectedBlockIdx == i);
        if (ImGui::Selectable(label, selected)) {
            st.selectedBlockIdx = i;
            st.selectedLinkIdx = -1;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("AiNavBlockProps", ImVec2(0, 200), true);
    if (st.selectedBlockIdx >= 0 && st.selectedBlockIdx < (int)ref.Blocks.size()) {
        auto& block = ref.Blocks[st.selectedBlockIdx];
        ImGui::PushItemWidth(140);
        int idx = static_cast<int>(block.Index);
        if (ImGui::InputInt("Index", &idx)) {
            block.Index = static_cast<uint32_t>(idx);
            session.MarkAiNavDirty(st.selectedRegionId);
            ctx.MarkModified();
        }
        ImGui::PopItemWidth();

        ImGui::Separator();
        ImGui::Text("Cell Lookup (%d):", (int)block.CellLookupTable.size());
        if (ImGui::Button("Add Cell")) {
            block.CellLookupTable.push_back({});
            SyncRefDungeonCounts(ref);
            session.MarkAiNavDirty(st.selectedRegionId);
            ctx.MarkModified();
        }
        ImGui::BeginChild("AiNavCells", ImVec2(-1, 70), true);
        for (int ci = 0; ci < (int)block.CellLookupTable.size(); ++ci) {
            ImGui::PushID(ci);
            auto& cell = block.CellLookupTable[ci];
            int e0 = cell.RefEdgeIndex0;
            int e1 = cell.RefEdgeIndex1;
            ImGui::SetNextItemWidth(70);
            if (ImGui::InputInt("E0##c", &e0)) {
                cell.RefEdgeIndex0 = static_cast<int16_t>(e0);
                session.MarkAiNavDirty(st.selectedRegionId);
                ctx.MarkModified();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            if (ImGui::InputInt("E1##c", &e1)) {
                cell.RefEdgeIndex1 = static_cast<int16_t>(e1);
                session.MarkAiNavDirty(st.selectedRegionId);
                ctx.MarkModified();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                block.CellLookupTable.erase(block.CellLookupTable.begin() + ci);
                SyncRefDungeonCounts(ref);
                session.MarkAiNavDirty(st.selectedRegionId);
                ctx.MarkModified();
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Text("Links (%d):", (int)block.Links.size());
        if (ImGui::Button("Add Link")) {
            block.Links.push_back({});
            SyncRefDungeonCounts(ref);
            st.selectedLinkIdx = (int)block.Links.size() - 1;
            session.MarkAiNavDirty(st.selectedRegionId);
            ctx.MarkModified();
        }
        ImGui::BeginChild("AiNavLinks", ImVec2(-1, 70), true);
        for (int li = 0; li < (int)block.Links.size(); ++li) {
            ImGui::PushID(li + 1000);
            auto& link = block.Links[li];
            bool sel = (st.selectedLinkIdx == li);
            char llabel[80];
            snprintf(llabel, sizeof(llabel), "Link %d -> obj %u", li, link.LinkedObjID);
            if (ImGui::Selectable(llabel, sel)) st.selectedLinkIdx = li;

            if (st.selectedLinkIdx == li) {
                ImGui::Indent();
                int id = static_cast<int>(link.ID);
                int cellId = link.CellID;
                int objId = link.LinkedObjID;
                int edgeIdx = link.LinkedObjRefEdgeIndex;
                ImGui::SetNextItemWidth(100);
                if (ImGui::InputInt("ID", &id)) { link.ID = static_cast<uint32_t>(id); session.MarkAiNavDirty(st.selectedRegionId); ctx.MarkModified(); }
                ImGui::SetNextItemWidth(100);
                if (ImGui::InputInt("CellID", &cellId)) { link.CellID = static_cast<uint16_t>(cellId); session.MarkAiNavDirty(st.selectedRegionId); ctx.MarkModified(); }
                ImGui::SetNextItemWidth(100);
                if (ImGui::InputInt("LinkedObjID", &objId)) { link.LinkedObjID = static_cast<uint16_t>(objId); session.MarkAiNavDirty(st.selectedRegionId); ctx.MarkModified(); }
                ImGui::SetNextItemWidth(100);
                if (ImGui::InputInt("RefEdge", &edgeIdx)) { link.LinkedObjRefEdgeIndex = static_cast<uint16_t>(edgeIdx); session.MarkAiNavDirty(st.selectedRegionId); ctx.MarkModified(); }
                if (ImGui::SmallButton("Delete Link")) {
                    block.Links.erase(block.Links.begin() + li);
                    SyncRefDungeonCounts(ref);
                    st.selectedLinkIdx = -1;
                    session.MarkAiNavDirty(st.selectedRegionId);
                    ctx.MarkModified();
                    ImGui::Unindent();
                    ImGui::PopID();
                    break;
                }
                ImGui::Unindent();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete Block")) {
            ref.Blocks.erase(ref.Blocks.begin() + st.selectedBlockIdx);
            if (st.selectedBlockIdx < (int)ref.BlockLookupTable.size())
                ref.BlockLookupTable.erase(ref.BlockLookupTable.begin() + st.selectedBlockIdx);
            SyncRefDungeonCounts(ref);
            st.selectedBlockIdx = -1;
            st.selectedLinkIdx = -1;
            session.MarkAiNavDirty(st.selectedRegionId);
            ctx.MarkModified();
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::TextDisabled("Select a block to edit.");
    }
    ImGui::EndChild();
}

static void DrawSimpleDungeonBlocks(EditorContext& ctx, sro::formats::AiNavData& data, sro::nav::NavMeshSession& session) {
    auto& st = ctx.aiNavDataEditor;
    auto& simple = data.SimpleDungeon;

    ImGui::Text("Simple dungeon blocks: %d", (int)simple.Blocks.size());
    ImGui::SameLine();
    if (ImGui::Button("Add Simple Block")) {
        simple.Blocks.push_back({});
        simple.BlockCount = static_cast<uint32_t>(simple.Blocks.size());
        st.selectedSimpleBlockIdx = (int)simple.Blocks.size() - 1;
        st.selectedEdgeIdx = -1;
        session.MarkAiNavDirty(st.selectedRegionId);
        ctx.MarkModified();
    }

    ImGui::BeginChild("AiNavSimpleList", ImVec2(200, 180), true);
    for (int i = 0; i < (int)simple.Blocks.size(); ++i) {
        char label[64];
        snprintf(label, sizeof(label), "Simple[%d] edges=%u", i, simple.Blocks[i].EdgeCount);
        if (ImGui::Selectable(label, st.selectedSimpleBlockIdx == i))
            st.selectedSimpleBlockIdx = i;
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("AiNavSimpleProps", ImVec2(0, 180), true);
    if (st.selectedSimpleBlockIdx >= 0 && st.selectedSimpleBlockIdx < (int)simple.Blocks.size()) {
        auto& block = simple.Blocks[st.selectedSimpleBlockIdx];
        int edgeCount = static_cast<int>(block.EdgeCount);
        if (ImGui::InputInt("Edge Count", &edgeCount) && edgeCount >= 0) {
            block.EdgeCount = static_cast<uint32_t>(edgeCount);
            block.EdgeCenterX.resize(edgeCount, 0.f);
            block.EdgeCenterY.resize(edgeCount, 0.f);
            block.EdgeCenterZ.resize(edgeCount, 0.f);
            session.MarkAiNavDirty(st.selectedRegionId);
            ctx.MarkModified();
        }

        ImGui::BeginChild("AiNavEdgeCenters", ImVec2(-1, 0), false);
        for (int ei = 0; ei < (int)block.EdgeCenterX.size(); ++ei) {
            ImGui::PushID(ei);
            ImGui::Text("Edge %d", ei);
            ImGui::SetNextItemWidth(90);
            if (ImGui::InputFloat("X", &block.EdgeCenterX[ei])) { session.MarkAiNavDirty(st.selectedRegionId); ctx.MarkModified(); }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            if (ImGui::InputFloat("Y", &block.EdgeCenterY[ei])) { session.MarkAiNavDirty(st.selectedRegionId); ctx.MarkModified(); }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            if (ImGui::InputFloat("Z", &block.EdgeCenterZ[ei])) { session.MarkAiNavDirty(st.selectedRegionId); ctx.MarkModified(); }
            ImGui::PopID();
        }
        ImGui::EndChild();

        if (ImGui::Button("Delete Simple Block")) {
            simple.Blocks.erase(simple.Blocks.begin() + st.selectedSimpleBlockIdx);
            simple.BlockCount = static_cast<uint32_t>(simple.Blocks.size());
            st.selectedSimpleBlockIdx = -1;
            session.MarkAiNavDirty(st.selectedRegionId);
            ctx.MarkModified();
        }
    } else {
        ImGui::TextDisabled("Select a simple block to edit edge centers.");
    }
    ImGui::EndChild();
}

void Ui::DrawAiNavDataPanel(EditorContext& ctx) {
    if (!ImGui::Begin("AI Nav Data", &ctx.panels.aiNavDataPanel)) { ImGui::End(); return; }

    auto& st = ctx.aiNavDataEditor;
    auto* session = ctx.navMeshSession;

    if (!session) {
        ImGui::TextDisabled("NavMesh session not available. Import client data first.");
        ImGui::End();
        return;
    }

    const int viewRx = ctx.sroRegionRx;
    const int viewRy = ctx.sroRegionRy;
    const uint16_t viewRegionId = RegionIdFromCoords(viewRx, viewRy);
    const bool viewIsDungeon = IsDungeonRegionId(viewRegionId);
    const auto& aiFiles = session->Catalog().AiNavFiles();

    ImGui::Checkbox("Follow viewport region", &st.followViewportRegion);
    ImGui::SameLine();
    ImGui::TextDisabled("Viewport: (%d,%d)  ID %u%s", viewRx, viewRy, viewRegionId,
                        viewIsDungeon ? "  [dungeon]" : "  [terrain]");

    if (st.followViewportRegion) {
        if (viewRx != st.lastSyncedRx || viewRy != st.lastSyncedRy) {
            st.lastSyncedRx = viewRx;
            st.lastSyncedRy = viewRy;
            if (viewIsDungeon || session->Catalog().HasAiNav(viewRegionId)) {
                st.selectedRegionId = viewRegionId;
                st.selectedBlockIdx = -1;
                st.selectedLinkIdx = -1;
                for (int i = 0; i < (int)aiFiles.size(); ++i) {
                    if (aiFiles[i].regionId == viewRegionId) {
                        st.catalogPickerIdx = i;
                        break;
                    }
                }
                TryAutoLoadAiNav(ctx, *session, viewRegionId);
            }
        }
    } else if (st.selectedRegionId == 0 && !aiFiles.empty()) {
        st.selectedRegionId = aiFiles[0].regionId;
        st.catalogPickerIdx = 0;
        TryAutoLoadAiNav(ctx, *session, st.selectedRegionId);
    }

    if (!viewIsDungeon && st.followViewportRegion) {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.f, 1.f),
            "Terrain region uses nv_%02x%02x.nvm — open Terrain Nav for tile/cell editing.",
            viewRx, viewRy);
        ImGui::TextWrapped(
            "AINavData (.dat) is for dungeon instances only (region ID 32768+). "
            "The green/red overlay in the viewport is terrain nav from the .nvm file, not AINavData.");
        ImGui::Separator();
        ImGui::Text("Select a dungeon AINavData file:");
    }

    if (!aiFiles.empty()) {
        if (st.catalogPickerIdx < 0 || st.catalogPickerIdx >= (int)aiFiles.size())
            st.catalogPickerIdx = 0;

        std::vector<std::string> labels;
        labels.reserve(aiFiles.size());
        for (const auto& fe : aiFiles) {
            int rx = 0, ry = 0;
            DecodeRegionId(fe.regionId, rx, ry);
            labels.push_back("ainavdata_" + std::to_string(fe.regionId) +
                             "  (" + std::to_string(rx) + "," + std::to_string(ry) + ")");
        }
        std::vector<const char*> items;
        items.reserve(labels.size());
        for (const auto& s : labels) items.push_back(s.c_str());

        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("Dungeon files", &st.catalogPickerIdx, items.data(), (int)items.size())) {
            st.followViewportRegion = false;
            st.selectedRegionId = aiFiles[st.catalogPickerIdx].regionId;
            st.selectedBlockIdx = -1;
            st.selectedLinkIdx = -1;
            TryAutoLoadAiNav(ctx, *session, st.selectedRegionId);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("All ainavdata_*.dat files in Data/navmesh (%d dungeons)", (int)aiFiles.size());
    } else {
        ImGui::TextColored(ImVec4(1.f, 0.5f, 0.2f, 1.f), "No ainavdata_*.dat files found in Data/navmesh.");
    }

    ImGui::Separator();

    int regionId = st.selectedRegionId;
    ImGui::PushItemWidth(120);
    if (ImGui::InputInt("Region ID", &regionId)) {
        st.selectedRegionId = static_cast<uint16_t>(regionId);
        st.selectedBlockIdx = -1;
        st.selectedLinkIdx = -1;
        st.selectedSimpleBlockIdx = -1;
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Use Viewport ID")) {
        st.selectedRegionId = viewRegionId;
        st.followViewportRegion = true;
        st.lastSyncedRx = viewRx;
        st.lastSyncedRy = viewRy;
        TryAutoLoadAiNav(ctx, *session, st.selectedRegionId);
    }

    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (session->LoadAiNavData(st.selectedRegionId))
            Logger::Instance().Info("Loaded ainavdata for region " + std::to_string(st.selectedRegionId));
        else
            Logger::Instance().Warning("Failed to load ainavdata for region " + std::to_string(st.selectedRegionId) +
                " — file may not exist (terrain regions use .nvm, not .dat)");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        if (session->SaveAiNavData(st.selectedRegionId))
            Logger::Instance().Info("Saved ainavdata for region " + std::to_string(st.selectedRegionId));
        else
            Logger::Instance().Warning("Failed to save ainavdata for region " + std::to_string(st.selectedRegionId));
    }
    ImGui::SameLine();
    if (ImGui::Button("Create")) {
        if (session->CreateAiNavData(st.selectedRegionId))
            Logger::Instance().Info("Created ainavdata for region " + std::to_string(st.selectedRegionId));
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete File")) {
        if (session->DeleteAiNavData(st.selectedRegionId))
            Logger::Instance().Info("Deleted ainavdata for region " + std::to_string(st.selectedRegionId));
    }

    if (session->IsAiNavDirty(st.selectedRegionId)) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f), "*");
    }

    ImGui::Separator();

    auto* data = session->GetAiNavData(st.selectedRegionId);
    if (!data) {
        if (session->Catalog().HasAiNav(st.selectedRegionId)) {
            ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f),
                "ainavdata_%u.dat exists but is not loaded. Click Load.", st.selectedRegionId);
        } else if (IsDungeonRegionId(st.selectedRegionId)) {
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.3f, 1.f),
                "No ainavdata_%u.dat in client. Click Create to add one.", st.selectedRegionId);
        } else {
            ImGui::TextDisabled(
                "No ainavdata for region ID %u (terrain). Use Terrain Nav, or pick a dungeon from the list above.",
                st.selectedRegionId);
        }
        ImGui::End();
        return;
    }

    if (BeginAiNavSection("File Info")) {
        ImGui::Indent(8.0f);
        ImGui::Text("Version: %u", data->Version);
        ImGui::Text("SimpleDungeonDataOffset: %u", data->SimpleDungeonDataOffset);
        ImGui::Text("RefDungeon RegionID: %u  Blocks: %u", data->RefDungeon.RegionID, data->RefDungeon.BlockCount);
        ImGui::Text("SimpleDungeon RegionID: %u  Blocks: %u", data->SimpleDungeon.RegionID, data->SimpleDungeon.BlockCount);
        ImGui::Unindent(8.0f);
    }

    if (BeginAiNavSection("RefDungeon Blocks & Links")) {
        ImGui::Indent(8.0f);
        DrawRefDungeonBlocks(ctx, *data, *session);
        ImGui::Unindent(8.0f);
    }

    if (BeginAiNavSection("Simple Dungeon Edge Centers")) {
        ImGui::Indent(8.0f);
        DrawSimpleDungeonBlocks(ctx, *data, *session);
        ImGui::Unindent(8.0f);
    }

    if (BeginAiNavSection("Pathfinding Test (RefDungeon links)")) {
        ImGui::Indent(8.0f);
        ImGui::Text("BFS between blocks using RefDungeon link graph.");
        ImGui::PushItemWidth(120);
        ImGui::InputInt("Start Block", &st.pathfindStartBlock);
        ImGui::InputInt("End Block", &st.pathfindEndBlock);
        ImGui::PopItemWidth();
        if (ImGui::Button("Find Path")) {
            st.pathfindResult = FindBlockPathBFS(data->RefDungeon, st.pathfindStartBlock, st.pathfindEndBlock);
            if (st.pathfindResult.empty()) {
                Logger::Instance().Warning("AI Nav: No path between blocks " +
                    std::to_string(st.pathfindStartBlock) + " and " + std::to_string(st.pathfindEndBlock));
            } else {
                Logger::Instance().Info("AI Nav: Path found with " + std::to_string(st.pathfindResult.size()) + " blocks");
            }
        }
        if (!st.pathfindResult.empty()) {
            std::string pathStr;
            for (size_t i = 0; i < st.pathfindResult.size(); ++i) {
                if (i > 0) pathStr += " -> ";
                pathStr += std::to_string(st.pathfindResult[i]);
            }
            ImGui::TextWrapped("%s", pathStr.c_str());
        }
        ImGui::Unindent(8.0f);
    }

    ImGui::End();
}
