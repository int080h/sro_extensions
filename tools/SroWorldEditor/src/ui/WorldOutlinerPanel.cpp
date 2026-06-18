#include "ui/UiPanels.h"
#include "imgui.h"
#include <cstdio>
#include <filesystem>

static void DrawEntityNode(EditorContext& ctx, const SelectionId& id, const char* label, bool leaf = true) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (leaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    bool selected = ctx.selection && *ctx.selection == id;
    if (selected) flags |= ImGuiTreeNodeFlags_Selected;
    bool open = ImGui::TreeNodeEx(label, flags);
    if (ImGui::IsItemClicked()) ctx.Select(id);
    if (!leaf && open) ImGui::TreePop();
}

void Ui::DrawWorldOutlinerPanel(EditorContext& ctx) {
    if (!ImGui::Begin("World Outliner", &ctx.panels.worldOutliner)) { ImGui::End(); return; }

    static char filter[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filter", "Search...", filter, sizeof(filter));

    if (ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ctx.sroPlacements && ImGui::TreeNodeEx("Loaded SRO Placements", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& placement : *ctx.sroPlacements) {
                std::string label;
                if (!placement.BsrPath.empty()) {
                    label = std::filesystem::path(placement.BsrPath).filename().string();
                } else {
                    label = "Obj " + std::to_string(placement.Object.ObjID);
                }
                label += "  UID " + std::to_string(placement.Object.UID);
                if (filter[0] && label.find(filter) == std::string::npos) continue;

                const int regionId = EditorContext::EncodeRegionId(placement.LoadedRx, placement.LoadedRy);
                DrawEntityNode(ctx, {EntityKind::MapPlacement, regionId, std::to_string(placement.Object.UID)}, label.c_str());
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Regions", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& region : ctx.world.regions) {
                char label[64];
                snprintf(label, sizeof(label), "Region %d", region.id);
                SelectionId rid{EntityKind::Region, region.id, std::to_string(region.id)};
                bool regionOpen = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_OpenOnArrow);
                if (ImGui::IsItemClicked()) ctx.Select(rid);
                if (regionOpen) {
                    if (ImGui::TreeNodeEx("Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
                        for (const auto& obj : region.objects) {
                            if (filter[0] && obj.name.find(filter) == std::string::npos) continue;
                            DrawEntityNode(ctx, {EntityKind::WorldObject, region.id, obj.id}, obj.name.c_str());
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("NPCs")) {
                        for (const auto& n : region.npcs)
                            DrawEntityNode(ctx, {EntityKind::Npc, region.id, n.id}, n.codeName.c_str());
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Monster Spawns")) {
                        for (const auto& s : region.spawns)
                            DrawEntityNode(ctx, {EntityKind::SpawnPoint, region.id, s.id}, s.monsterCodeName.c_str());
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Teleports")) {
                        for (const auto& t : region.teleports)
                            DrawEntityNode(ctx, {EntityKind::TeleportPoint, region.id, t.id}, t.id.c_str());
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Zones")) {
                        for (const auto& z : region.zones)
                            DrawEntityNode(ctx, {EntityKind::Zone, region.id, z.id}, z.zoneType.c_str());
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Dungeons")) ImGui::TreePop();
        if (ImGui::TreeNode("Fortress Areas")) ImGui::TreePop();
        if (ImGui::TreeNode("Event Zones")) ImGui::TreePop();
        ImGui::TreePop();
    }
    ImGui::End();
}
