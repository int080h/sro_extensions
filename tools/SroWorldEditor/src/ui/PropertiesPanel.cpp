#include "ui/UiCommon.h"
#include "core/EntityPropertyBuilder.h"
#include "core/EntityPropertyApplier.h"
#include "ui/PropertySectionUi.h"
#include "imgui.h"
#include <cstdint>

void Ui::DrawPropertiesPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Properties", &ctx.panels.properties)) { ImGui::End(); return; }

    if (!ctx.selection) {
        ImGui::TextDisabled("No selection");
        ImGui::End();
        return;
    }

    static EntityPropertyEditState editState;
    static EntityPropertySheet sheet;
    static SelectionId cachedSelection{};

    const auto& sel = *ctx.selection;

    if (!(cachedSelection == sel)) {
        if (editState.hasPendingApply && cachedSelection.kind == EntityKind::MapPlacement) {
            int rx = 0, ry = 0;
            EditorContext::DecodeRegionId(cachedSelection.regionId, rx, ry);
            int16_t uid = static_cast<int16_t>(std::stoi(cachedSelection.id));
            if (auto* prev = ctx.FindPlacement(uid, rx, ry)) {
                prev->Object.PosX = editState.initialPosition.x;
                prev->Object.PosY = editState.initialPosition.y;
                prev->Object.PosZ = editState.initialPosition.z;
                prev->Object.Yaw = editState.initialYaw;
            }
        }
        sheet = EntityPropertyBuilder::Build(ctx, sel);
        cachedSelection = sel;
        editState.lastSelection = sel;
        editState.hasPendingApply = false;
        if (sheet.editPosition) editState.initialPosition = *sheet.editPosition;
        if (sheet.editYaw) editState.initialYaw = *sheet.editYaw;
    }

    bool changed = DrawEntityPropertySheet(sheet, editState, sel);

    if (sel.kind == EntityKind::MapPlacement && changed) {
        if (auto* vm = ctx.FindPlacementBySelection()) {
            if (sheet.editPosition) {
                vm->Object.PosX = sheet.editPosition->x;
                vm->Object.PosY = sheet.editPosition->y;
                vm->Object.PosZ = sheet.editPosition->z;
            }
            if (sheet.editYaw)
                vm->Object.Yaw = *sheet.editYaw;
        }
    } else if (changed && !sheet.requiresApply) {
        EntityPropertyApplier::ApplyProjectEntity(ctx, sheet, sel);
    }

    if (sheet.requiresApply) {
        ImGui::Spacing();
        if (ImGui::Button("Apply", ImVec2(90, 0))) {
            EntityPropertyApplier::ApplyPlacement(ctx, sheet, sel);
            editState.hasPendingApply = false;
            if (sheet.editPosition) editState.initialPosition = *sheet.editPosition;
            if (sheet.editYaw) editState.initialYaw = *sheet.editYaw;
        }
        ImGui::SameLine();
        if (editState.hasPendingApply)
            ImGui::TextDisabled("Unsaved changes");
    }

    if (ctx.sroClientLoaded && (sel.kind == EntityKind::MapPlacement || sel.kind == EntityKind::Npc
            || sel.kind == EntityKind::WorldObject)) {
        ImGui::Spacing();
        if (ImGui::Button("Inspect in Object Viewer")) {
            ctx.panels.objectViewer = true;
            ctx.SyncObjectViewerFromSelection();
        }
    }

    if (sel.kind == EntityKind::Region) {
        if (auto* r = ctx.world.FindRegion(sel.regionId))
            ctx.activeRegionId = r->id;
    }

    ImGui::End();
}
