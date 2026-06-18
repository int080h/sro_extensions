#include "ui/UiCommon.h"
#include "formats/DofFormat.h"
#include "parsers/DofParser.h"
#include "core/Logger.h"
#include "imgui.h"
#include <cstring>

static bool BeginDungeonSection(const char* label) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);
    ImGui::PopStyleVar();
    return open;
}

void Ui::DrawDungeonNavPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Dungeon Nav", &ctx.panels.dungeonNavPanel)) { ImGui::End(); return; }

    auto& st = ctx.navmeshEditor;

    if (BeginDungeonSection("Open Dungeon (.dof / JMXVDOF)")) {
        ImGui::Indent(8.0f);
        ImGui::SetNextItemWidth(420);
        ImGui::InputText("Path", st.dofPath, sizeof(st.dofPath));
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            std::wstring wpath(st.dofPath, st.dofPath + std::strlen(st.dofPath));
            auto dof = std::make_shared<sro::formats::DofDungeon>();
            auto res = sro::DofParser::Read(wpath, *dof);
            if (res.ok) {
                ctx.loadedDof = dof;
                ctx.loadedDofPath = st.dofPath;
                ctx.selectedDofBlockIdx = -1;
                Logger::Instance().Info("Dungeon loaded: " + std::to_string(dof->Blocks.size()) +
                    " blocks, " + std::to_string(dof->Voxels.size()) + " voxels");
            } else {
                ctx.loadedDof.reset();
                Logger::Instance().Warning("Dungeon load failed: " + res.error);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            ctx.loadedDof.reset();
            ctx.loadedDofPath.clear();
            ctx.selectedDofBlockIdx = -1;
        }
        if (ctx.loadedDof) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Loaded: %s", ctx.loadedDofPath.c_str());
        }
        ImGui::Unindent(8.0f);
    }

    if (!ctx.loadedDof) {
        ImGui::TextDisabled("No dungeon loaded. Enter a .dof path and click Load.");
        ImGui::TextDisabled("Dungeons use JMXVDOF (RTNavMeshDungeon): 3D block grid + voxel");
        ImGui::TextDisabled("lookup + off-mesh links -- distinct from terrain JMXVNVM.");
        ImGui::End();
        return;
    }

    const auto& d = *ctx.loadedDof;

    if (BeginDungeonSection("Dungeon Statistics")) {
        ImGui::Indent(8.0f);
        ImGui::Text("Signature: %s", d.Signature.c_str());
        ImGui::Text("Name: %s", d.Name.c_str());
        ImGui::Text("RegionID: %u", d.RegionID);
        ImGui::Separator();
        ImGui::Text("Blocks: %d", (int)d.Blocks.size());
        ImGui::Text("Voxels: %d  (grid %u x %u x %u)", (int)d.Voxels.size(), d.GridWidth, d.GridHeight, d.GridLength);
        ImGui::Text("Links: %d", (int)d.Links.size());
        ImGui::Text("Groups: %d", (int)d.Groups.size());
        ImGui::Text("Rooms: %d   Floors: %d", (int)d.Labels.Rooms.size(), (int)d.Labels.Floors.size());
        int totalObjects = 0, colObjects = 0, waterObjects = 0;
        for (const auto& b : d.Blocks) {
            totalObjects += (int)b.Objects.size();
            for (const auto& o : b.Objects) {
                if (o.Flag & 2) ++colObjects;
                if (o.Flag & 4) ++waterObjects;
            }
        }
        ImGui::Separator();
        ImGui::Text("Objects: %d  (ColObj=%d, WaterObj=%d)", totalObjects, colObjects, waterObjects);
        ImGui::Unindent(8.0f);
    }

    if (BeginDungeonSection("Viewport")) {
        ImGui::Indent(8.0f);
        ImGui::TextDisabled("Layer toggles live in Nav Layers panel.");
        if (ImGui::Button("Open Nav Layers")) ctx.panels.navLayersPanel = true;
        ImGui::Unindent(8.0f);
    }

    if (BeginDungeonSection("Blocks")) {
        ImGui::Indent(8.0f);
        ImGui::BeginChild("DofBlockList", ImVec2(280, -1), true);
        for (int i = 0; i < (int)d.Blocks.size(); ++i) {
            const auto& b = d.Blocks[i];
            char label[160];
            snprintf(label, sizeof(label), "Block[%d] %s  R%u/F%u", i,
                     b.Name.empty() ? "(unnamed)" : b.Name.c_str(), b.RoomIndex, b.FloorIndex);
            bool sel = (ctx.selectedDofBlockIdx == i);
            if (ImGui::Selectable(label, sel)) ctx.selectedDofBlockIdx = i;
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("DofBlockProps", ImVec2(0, -1), true);
        if (ctx.selectedDofBlockIdx >= 0 && ctx.selectedDofBlockIdx < (int)d.Blocks.size()) {
            const auto& b = d.Blocks[ctx.selectedDofBlockIdx];
            ImGui::Text("Name: %s", b.Name.c_str());
            ImGui::Text("Path: %s", b.Path.c_str());
            ImGui::Text("Position: (%.1f, %.1f, %.1f)", b.Position.x, b.Position.y, b.Position.z);
            ImGui::Text("Yaw: %.3f", b.Yaw);
            ImGui::Text("Room: %u   Floor: %u", b.RoomIndex, b.FloorIndex);
            ImGui::Separator();
            ImGui::Text("Connected blocks: %d", (int)b.ConnectedBlockIndices.size());
            ImGui::Text("Visible blocks: %d", (int)b.VisibleBlockIndices.size());
            ImGui::Text("Objects: %d   Lights: %d", (int)b.Objects.size(), (int)b.Lights.size());
            if (!b.Objects.empty() && ImGui::TreeNode("Objects")) {
                for (int oi = 0; oi < (int)b.Objects.size(); ++oi) {
                    const auto& o = b.Objects[oi];
                    const char* kind = (o.Flag & 4) ? "Water" : ((o.Flag & 2) ? "Col" : "None");
                    ImGui::Text("  [%d] %s  flag=%s  (%.1f,%.1f,%.1f)", oi,
                                o.Path.c_str(), kind, o.Position.x, o.Position.y, o.Position.z);
                }
                ImGui::TreePop();
            }
        } else {
            ImGui::TextDisabled("Select a block to inspect.");
        }
        ImGui::EndChild();
        ImGui::Unindent(8.0f);
    }

    ImGui::End();
}
