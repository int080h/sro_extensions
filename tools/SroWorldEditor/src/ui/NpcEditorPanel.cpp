#include "ui/UiCommon.h"
#include "core/Logger.h"
#include "index/TextDataCatalog.h"
#include "imgui.h"
#include <cstdio>

void Ui::DrawNpcEditorPanel(EditorContext& ctx) {
    if (!ImGui::Begin("NPC Editor", &ctx.panels.npcEditor)) { ImGui::End(); return; }

    Region* region = ctx.world.FindRegion(ctx.activeRegionId);
    if (!region) {
        ImGui::TextDisabled("No active region.");
        ImGui::End();
        return;
    }

    ImGui::Text("Region %d (%s)", region->id, region->name.c_str());
    ImGui::Text("NPC count: %d", (int)region->npcs.size());
    ImGui::Separator();

    static char codeNameBuf[128] = "NPC_CH_SMITH";
    if (!ctx.npcEditorState.newCodeName.empty()) {
        std::snprintf(codeNameBuf, sizeof(codeNameBuf), "%s", ctx.npcEditorState.newCodeName.c_str());
    }
    if (ImGui::InputText("CodeName", codeNameBuf, sizeof(codeNameBuf)))
        ctx.npcEditorState.newCodeName = codeNameBuf;

    if (ctx.sroTextData) {
        ImGui::TextDisabled("Pick from catalog:");
        ImGui::BeginChild("NpcCatalog", ImVec2(0, 120), true);
        std::vector<const sro::GameObjectRef*> npcs;
        ctx.sroTextData->Search("NPC_", npcs, 300);
        for (const auto* ref : npcs) {
            if (!ref->IsNpc()) continue;
            if (ImGui::Selectable(ref->codeName.c_str(), ctx.npcEditorState.newCodeName == ref->codeName)) {
                ctx.npcEditorState.newCodeName = ref->codeName;
            }
        }
        ImGui::EndChild();
    }

    if (ImGui::Button("Add NPC at Camera")) {
        NPC npc;
        npc.id = "npc_" + std::to_string(region->npcs.size() + 1) + "_" + std::to_string(region->id);
        npc.codeName = ctx.npcEditorState.newCodeName.empty() ? "NPC_CH_SMITH" : ctx.npcEditorState.newCodeName;
        npc.regionId = region->id;
        npc.transform.position = ctx.project.cameraPosition;
        region->npcs.push_back(npc);
        ctx.MarkModified();
        ctx.Select({EntityKind::Npc, region->id, npc.id});
        Logger::Instance().Info("Added NPC " + npc.codeName);
    }
    ImGui::SameLine();
    if (ImGui::Button("Use NPC Placement Tool")) {
        ctx.activeTool = EditorToolType::NpcPlacement;
    }

    ImGui::Separator();
    ImGui::Text("NPCs in region:");
    ImGui::BeginChild("NpcList");
    int deleteIdx = -1;
    for (int i = 0; i < static_cast<int>(region->npcs.size()); ++i) {
        const auto& npc = region->npcs[i];
        const bool selected = ctx.selection && ctx.selection->kind == EntityKind::Npc
            && ctx.selection->id == npc.id && ctx.selection->regionId == region->id;
        ImGui::PushID(i);
        if (ImGui::Selectable(npc.codeName.c_str(), selected)) {
            ctx.Select({EntityKind::Npc, region->id, npc.id});
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Inspect")) {
            ctx.InspectGameObject(npc.codeName);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Delete"))
            deleteIdx = i;
        ImGui::PopID();
    }
    if (deleteIdx >= 0) {
        region->npcs.erase(region->npcs.begin() + deleteIdx);
        ctx.MarkModified();
        ctx.ClearSelection();
    }
    ImGui::EndChild();

    ImGui::End();
}
