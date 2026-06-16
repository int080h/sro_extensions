#include "core/EntityPropertyApplier.h"
#include "core/EditorAction.h"
#include "PlacementVM.h"
#include "world/WorldObject.h"
#include "world/Region.h"
#include "rendering/RenderManager.h"
#include "runtime/RegionManager.h"
#include <cmath>

namespace {

bool Vec3Near(const Vector3& a, const Vector3& b, float eps = 0.001f) {
    return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps && std::fabs(a.z - b.z) < eps;
}

} // namespace

bool EntityPropertyApplier::ApplyPlacement(EditorContext& ctx, const EntityPropertySheet& sheet, const SelectionId& sel) {
    if (sel.kind != EntityKind::MapPlacement || !ctx.sroRegionManager) return false;

    int rx = 0, ry = 0;
    EditorContext::DecodeRegionId(sel.regionId, rx, ry);
    int16_t uid = static_cast<int16_t>(std::stoi(sel.id));
    PlacementVM* vm = ctx.FindPlacement(uid, rx, ry);
    if (!vm) return false;

    Vector3 oldPos(vm->Object.PosX, vm->Object.PosY, vm->Object.PosZ);
    float oldYaw = vm->Object.Yaw;

    Vector3 newPos = sheet.editPosition.value_or(oldPos);
    float newYaw = sheet.editYaw.value_or(oldYaw);

    bool transformChanged = !Vec3Near(oldPos, newPos) || std::fabs(oldYaw - newYaw) > 0.0001f;
    if (transformChanged) {
        vm->Object.PosX = oldPos.x;
        vm->Object.PosY = oldPos.y;
        vm->Object.PosZ = oldPos.z;
        vm->Object.Yaw = oldYaw;

        ctx.sroRegionManager->PushAction(std::make_unique<ModifyObjectAction>(
            uid, vm->LoadedRx, vm->LoadedRy, oldPos, oldYaw, newPos, newYaw));
    }

    if (ctx.sroRenderManager && sheet.editHidden) {
        bool hidden = *sheet.editHidden;
        bool wasHidden = ctx.sroRenderManager->HiddenObjectUIDs.count(uid) > 0;
        if (hidden) {
            ctx.sroRenderManager->HiddenObjectUIDs.insert(uid);
        } else {
            ctx.sroRenderManager->HiddenObjectUIDs.erase(uid);
        }
        if (hidden != wasHidden) transformChanged = true;
    }

    return transformChanged;
}

bool EntityPropertyApplier::ApplyProjectEntity(EditorContext& ctx, const EntityPropertySheet& sheet, const SelectionId& sel) {
    bool changed = false;

    switch (sel.kind) {
    case EntityKind::WorldObject:
        if (auto* obj = ctx.world.FindObject(sel.id, sel.regionId)) {
            if (sheet.editName) { obj->name = *sheet.editName; changed = true; }
            if (sheet.editModelPath) { obj->modelPath = *sheet.editModelPath; changed = true; }
            if (sheet.editPosition) { obj->transform.position = *sheet.editPosition; changed = true; }
            if (sheet.editRotation) { obj->transform.rotation = *sheet.editRotation; changed = true; }
            if (sheet.editScale) { obj->transform.scale = *sheet.editScale; changed = true; }
            if (sheet.editCollision) { obj->collisionEnabled = *sheet.editCollision; changed = true; }
            if (sheet.editVisible) { obj->visible = *sheet.editVisible; changed = true; }
            if (sheet.editLocked) { obj->locked = *sheet.editLocked; changed = true; }
            if (sheet.editNotes) { obj->notes = *sheet.editNotes; changed = true; }
        }
        break;
    case EntityKind::Npc:
        if (auto* npc = ctx.world.FindNpc(sel.id, sel.regionId)) {
            if (sheet.editPosition) { npc->transform.position = *sheet.editPosition; changed = true; }
            if (sheet.editRotation) { npc->transform.rotation = *sheet.editRotation; changed = true; }
            if (sheet.editScale) { npc->transform.scale = *sheet.editScale; changed = true; }
            for (const auto& f : sheet.metadata.fields) {
                if (!f.editable) continue;
                if (f.label == "Shop Link") { npc->shopLink = f.intValue; changed = true; }
                else if (f.label == "Quest Link") { npc->questLink = f.intValue; changed = true; }
                else if (f.label == "Dialog Link") { npc->dialogLink = f.intValue; changed = true; }
                else if (f.label == "Teleport Link") { npc->teleportLink = f.intValue; changed = true; }
            }
        }
        break;
    case EntityKind::SpawnPoint:
        if (auto* sp = ctx.world.FindSpawn(sel.id, sel.regionId)) {
            if (sheet.editPosition) { sp->position = *sheet.editPosition; changed = true; }
            for (const auto& f : sheet.geometry.fields) {
                if (f.editable && f.label == "Radius") { sp->radius = f.floatValue; changed = true; }
            }
            for (const auto& f : sheet.metadata.fields) {
                if (!f.editable) continue;
                if (f.label == "Count") { sp->count = f.intValue; changed = true; }
                else if (f.label == "Respawn Time") { sp->respawnTime = f.floatValue; changed = true; }
                else if (f.label == "Aggressive") { sp->aggressive = f.boolValue; changed = true; }
                else if (f.label == "Tactics ID") { sp->tacticsId = f.intValue; changed = true; }
                else if (f.label == "Hive ID") { sp->hiveId = f.intValue; changed = true; }
                else if (f.label == "Nest ID") { sp->nestId = f.intValue; changed = true; }
            }
        }
        break;
    case EntityKind::TeleportPoint:
        if (auto* tp = ctx.world.FindTeleport(sel.id, sel.regionId)) {
            if (sheet.editPosition) { tp->sourcePosition = *sheet.editPosition; changed = true; }
            if (sheet.editDestPosition) { tp->destinationPosition = *sheet.editDestPosition; changed = true; }
            for (const auto& f : sheet.metadata.fields) {
                if (!f.editable) continue;
                if (f.label == "Dest Region") { tp->destinationRegionId = f.intValue; changed = true; }
                else if (f.label == "Required Level") { tp->requiredLevel = f.intValue; changed = true; }
                else if (f.label == "Required Gold") { tp->requiredGold = f.intValue; changed = true; }
                else if (f.label == "Enabled") { tp->enabled = f.boolValue; changed = true; }
            }
        }
        break;
    case EntityKind::Zone:
        if (auto* z = ctx.world.FindZone(sel.id, sel.regionId)) {
            for (const auto& f : sheet.metadata.fields) {
                if (!f.editable) continue;
                if (f.label == "Priority") { z->priority = f.intValue; changed = true; }
                else if (f.label == "PvP") { z->pvpEnabled = f.boolValue; changed = true; }
                else if (f.label == "Job Only") { z->jobOnly = f.boolValue; changed = true; }
                else if (f.label == "Safe Zone") { z->safeZone = f.boolValue; changed = true; }
                else if (f.label == "Event Zone") { z->eventZone = f.boolValue; changed = true; }
            }
        }
        break;
    default:
        break;
    }

    if (changed) ctx.MarkModified();
    return changed;
}
