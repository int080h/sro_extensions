#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "core/Logger.h"
#include "imgui.h"

void Ui::DrawRegionManagerPanel(EditorContext& ctx, Editor& editor) {
    if (!ImGui::Begin("Region Manager", &ctx.panels.regionManager)) { ImGui::End(); return; }

    if (ImGui::BeginTable("Regions", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("Region ID");
        ImGui::TableSetupColumn("Objects");
        ImGui::TableSetupColumn("NPCs");
        ImGui::TableSetupColumn("Spawns");
        ImGui::TableSetupColumn("Teleports");
        ImGui::TableSetupColumn("Zones");
        ImGui::TableSetupColumn("Collision");
        ImGui::TableSetupColumn("Errors");
        ImGui::TableSetupColumn("Modified");
        ImGui::TableHeadersRow();

        for (const auto& r : ctx.world.regions) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Selectable(std::to_string(r.id).c_str(), ctx.activeRegionId == r.id)) {
                ctx.activeRegionId = r.id;
                ctx.Select({EntityKind::Region, r.id, std::to_string(r.id)});
            }
            ImGui::TableNextColumn(); ImGui::Text("%d", (int)r.objects.size());
            ImGui::TableNextColumn(); ImGui::Text("%d", (int)r.npcs.size());
            ImGui::TableNextColumn(); ImGui::Text("%d", (int)r.spawns.size());
            ImGui::TableNextColumn(); ImGui::Text("%d", (int)r.teleports.size());
            ImGui::TableNextColumn(); ImGui::Text("%d", (int)r.zones.size());
            ImGui::TableNextColumn(); ImGui::Text(r.hasCollision ? "Yes" : "No");
            ImGui::TableNextColumn(); ImGui::Text("-");
            ImGui::TableNextColumn(); ImGui::Text(r.modified ? "*" : "");
        }
        ImGui::EndTable();
    }

    if (ImGui::Button("Open Region")) {
        if (ctx.sroClientLoaded && editor.Viewport().GetSroSession()) {
            int rx = ctx.sroRegionRx;
            int ry = ctx.sroRegionRy;
            editor.Viewport().SetSroRegion(rx, ry);
            Logger::Instance().Info("Opened SRO region " + std::to_string(rx) + "," + std::to_string(ry));
        } else if (auto* r = ctx.world.FindRegion(ctx.activeRegionId)) {
            Logger::Instance().Info("Opened editor region " + std::to_string(r->id) + " (import client data for SRO view)");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Validate Region")) editor.RunValidation();
    ImGui::End();
}
