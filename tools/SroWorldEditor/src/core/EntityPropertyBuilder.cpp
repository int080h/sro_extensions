#include "core/EntityPropertyBuilder.h"
#include "PlacementVM.h"
#include "world/WorldObject.h"
#include "world/Region.h"
#include "rendering/MeshRenderer.h"
#include "rendering/RenderManager.h"
#include "index/TextDataCatalog.h"
#include "sro/world/PlacementFilter.h"
#include <cstdio>
#include <filesystem>

namespace {

PropertyField ReadOnlyText(const std::string& label, const std::string& value) {
    PropertyField f;
    f.label = label;
    f.type = PropertyFieldType::ReadOnlyText;
    f.textValue = value;
    return f;
}

PropertyField EditableText(const std::string& label, const std::string& value) {
    PropertyField f;
    f.label = label;
    f.type = PropertyFieldType::Text;
    f.editable = true;
    f.textValue = value;
    return f;
}

PropertyField ReadOnlyFloat(const std::string& label, float value, const char* fmt = "%.2f") {
    char buf[64];
    std::snprintf(buf, sizeof(buf), fmt, value);
    return ReadOnlyText(label, buf);
}

PropertyField EditableFloat3(const std::string& label, const Vector3& v) {
    PropertyField f;
    f.label = label;
    f.type = PropertyFieldType::Float3;
    f.editable = true;
    f.float3Value = v;
    return f;
}

PropertyField EditableYaw(const std::string& label, float yaw) {
    PropertyField f;
    f.label = label;
    f.type = PropertyFieldType::YawRadians;
    f.editable = true;
    f.floatValue = yaw;
    return f;
}

PropertyField EditableBool(const std::string& label, bool value) {
    PropertyField f;
    f.label = label;
    f.type = PropertyFieldType::Bool;
    f.editable = true;
    f.boolValue = value;
    return f;
}

PropertyField ReadOnlyInt(const std::string& label, int value) {
    return ReadOnlyText(label, std::to_string(value));
}

PropertyField EditableInt(const std::string& label, int value) {
    PropertyField f;
    f.label = label;
    f.type = PropertyFieldType::Int;
    f.editable = true;
    f.intValue = value;
    return f;
}

PropertyField EditableFloat(const std::string& label, float value) {
    PropertyField f;
    f.label = label;
    f.type = PropertyFieldType::Float;
    f.editable = true;
    f.floatValue = value;
    return f;
}

void AppendModelSections(EntityPropertySheet& sheet, MeshRenderer* mr, const std::string& bsrPath) {
    if (!mr || bsrPath.empty()) return;
    auto* model = mr->PreloadModel(bsrPath);
    if (!model) return;

    const Vector3 size = model->MaxBounds - model->MinBounds;
    char sizeBuf[128];
    std::snprintf(sizeBuf, sizeof(sizeBuf), "%.1f x %.1f x %.1f", size.x, size.y, size.z);
    sheet.geometry.fields.push_back(ReadOnlyText("AABB Min", std::to_string(model->MinBounds.x) + ", " +
        std::to_string(model->MinBounds.y) + ", " + std::to_string(model->MinBounds.z)));
    sheet.geometry.fields.push_back(ReadOnlyText("AABB Max", std::to_string(model->MaxBounds.x) + ", " +
        std::to_string(model->MaxBounds.y) + ", " + std::to_string(model->MaxBounds.z)));
    sheet.geometry.fields.push_back(ReadOnlyText("Size (W x H x D)", sizeBuf));
    sheet.geometry.fields.push_back(ReadOnlyInt("Submesh Count", model->SubMeshCount));
    sheet.geometry.fields.push_back(ReadOnlyInt("Vertex Count", model->TotalVertices));
    sheet.geometry.fields.push_back(ReadOnlyInt("Index Count", model->TotalIndices));

    for (const auto& sm : model->SubMeshes) {
        MaterialEntry entry;
        entry.materialName = sm.MaterialName;
        entry.vertexCount = sm.VertexCount;
        entry.indexCount = sm.IndexCount;
        if (!sm.TexturePath.empty()) {
            entry.texturePath = std::string(sm.TexturePath.begin(), sm.TexturePath.end());
        }
        sheet.material.entries.push_back(std::move(entry));
    }
}

EntityPropertySheet BuildMapPlacement(const PlacementVM& vm, MeshRenderer* mr, RenderManager* rm) {
    EntityPropertySheet sheet;
    sheet.kind = EntityKind::MapPlacement;
    sheet.requiresApply = true;

    std::string title = vm.BsrPath;
    if (!title.empty()) {
        title = std::filesystem::path(title).filename().string();
    } else {
        title = "Obj " + std::to_string(vm.Object.ObjID);
    }
    sheet.title = title;

    AppendModelSections(sheet, mr, vm.BsrPath);

    sheet.transform.fields.push_back(EditableFloat3("Position", Vector3(vm.Object.PosX, vm.Object.PosY, vm.Object.PosZ)));
    sheet.transform.fields.push_back(EditableYaw("Yaw", vm.Object.Yaw));

    sheet.editPosition = Vector3(vm.Object.PosX, vm.Object.PosY, vm.Object.PosZ);
    sheet.editYaw = vm.Object.Yaw;

    bool hidden = rm && rm->HiddenObjectUIDs.count(vm.Object.UID) > 0;
    sheet.editHidden = hidden;
    sheet.metadata.fields.push_back(EditableBool("Hidden", hidden));

    sheet.metadata.fields.push_back(ReadOnlyInt("ObjID", static_cast<int>(vm.Object.ObjID)));
    sheet.metadata.fields.push_back(ReadOnlyInt("UID", vm.Object.UID));
    sheet.metadata.fields.push_back(ReadOnlyInt("IsStatic", vm.Object.IsStatic));
    sheet.metadata.fields.push_back(ReadOnlyInt("IsBig", vm.Object.IsBig));
    sheet.metadata.fields.push_back(ReadOnlyInt("IsStruct", vm.Object.IsStruct));
    sheet.metadata.fields.push_back(ReadOnlyInt("Short0", vm.Object.Short0));
    sheet.metadata.fields.push_back(ReadOnlyInt("RegionID", vm.Object.RegionID));
    sheet.metadata.fields.push_back(ReadOnlyInt("Block Z", vm.BlockZ));
    sheet.metadata.fields.push_back(ReadOnlyInt("Block X", vm.BlockX));
    sheet.metadata.fields.push_back(ReadOnlyInt("LOD", vm.Lod));
    sheet.metadata.fields.push_back(ReadOnlyInt("Loaded Rx", vm.LoadedRx));
    sheet.metadata.fields.push_back(ReadOnlyInt("Loaded Ry", vm.LoadedRy));
    sheet.metadata.fields.push_back(ReadOnlyText("BSR Path", vm.BsrPath));
    sheet.metadata.fields.push_back(ReadOnlyText("Event Decor",
        sro::IsEventOrSeasonalDecor(vm.BsrPath) ? "Yes" : "No"));

    return sheet;
}

EntityPropertySheet BuildWorldObject(const WorldObject& obj, MeshRenderer* mr) {
    EntityPropertySheet sheet;
    sheet.kind = EntityKind::WorldObject;
    sheet.title = obj.name.empty() ? obj.id : obj.name;

    AppendModelSections(sheet, mr, obj.modelPath);

    sheet.transform.fields.push_back(EditableFloat3("Position", obj.transform.position));
    sheet.transform.fields.push_back(EditableFloat3("Rotation", obj.transform.rotation));
    sheet.transform.fields.push_back(EditableFloat3("Scale", obj.transform.scale));

    sheet.editPosition = obj.transform.position;
    sheet.editRotation = obj.transform.rotation;
    sheet.editScale = obj.transform.scale;
    sheet.editName = obj.name;
    sheet.editModelPath = obj.modelPath;
    sheet.editCollision = obj.collisionEnabled;
    sheet.editVisible = obj.visible;
    sheet.editLocked = obj.locked;
    sheet.editNotes = obj.notes;

    sheet.metadata.fields.push_back(ReadOnlyText("ID", obj.id));
    sheet.metadata.fields.push_back(EditableText("Name", obj.name));
    sheet.metadata.fields.push_back(EditableText("Model Path", obj.modelPath));
    sheet.metadata.fields.push_back(ReadOnlyInt("Region ID", obj.regionId));
    sheet.metadata.fields.push_back(EditableBool("Collision", obj.collisionEnabled));
    sheet.metadata.fields.push_back(EditableBool("Visible", obj.visible));
    sheet.metadata.fields.push_back(EditableBool("Locked", obj.locked));
    sheet.metadata.fields.push_back(EditableText("Notes", obj.notes));

    return sheet;
}

EntityPropertySheet BuildNpc(const NPC& npc, MeshRenderer* mr, const sro::TextDataCatalog* catalog) {
    EntityPropertySheet sheet;
    sheet.kind = EntityKind::Npc;
    sheet.title = npc.codeName.empty() ? npc.id : npc.codeName;

    sheet.transform.fields.push_back(EditableFloat3("Position", npc.transform.position));
    sheet.transform.fields.push_back(EditableFloat3("Rotation", npc.transform.rotation));
    sheet.transform.fields.push_back(EditableFloat3("Scale", npc.transform.scale));

    sheet.editPosition = npc.transform.position;
    sheet.editRotation = npc.transform.rotation;
    sheet.editScale = npc.transform.scale;

    sheet.metadata.fields.push_back(ReadOnlyText("ID", npc.id));
    sheet.metadata.fields.push_back(ReadOnlyText("CodeName", npc.codeName));
    sheet.metadata.fields.push_back(ReadOnlyInt("Region ID", npc.regionId));
    sheet.metadata.fields.push_back(EditableInt("Shop Link", npc.shopLink));
    sheet.metadata.fields.push_back(EditableInt("Quest Link", npc.questLink));
    sheet.metadata.fields.push_back(EditableInt("Dialog Link", npc.dialogLink));
    sheet.metadata.fields.push_back(EditableInt("Teleport Link", npc.teleportLink));

    if (catalog) {
        if (const auto* ref = catalog->FindByCodeName(npc.codeName)) {
            const std::string modelPath = catalog->ResolvePrimaryModelPath(*ref);
            if (!modelPath.empty()) {
                sheet.metadata.fields.push_back(ReadOnlyText("Model Path", modelPath));
                AppendModelSections(sheet, mr, modelPath);
                if (mr) {
                    if (const auto* bsr = mr->GetBsrResource(modelPath)) {
                        sheet.metadata.fields.push_back(ReadOnlyInt("Animation Count",
                            static_cast<int>(bsr->AnimationPaths.size())));
                    }
                }
            }
        }
    }

    return sheet;
}

EntityPropertySheet BuildSpawn(const SpawnPoint& sp) {
    EntityPropertySheet sheet;
    sheet.kind = EntityKind::SpawnPoint;
    sheet.title = sp.monsterCodeName.empty() ? sp.id : sp.monsterCodeName;

    sheet.geometry.fields.push_back(EditableFloat("Radius", sp.radius));
    sheet.transform.fields.push_back(EditableFloat3("Position", sp.position));
    sheet.editPosition = sp.position;

    sheet.metadata.fields.push_back(ReadOnlyText("ID", sp.id));
    sheet.metadata.fields.push_back(ReadOnlyText("Monster", sp.monsterCodeName));
    sheet.metadata.fields.push_back(ReadOnlyInt("Region ID", sp.regionId));
    sheet.metadata.fields.push_back(EditableInt("Count", sp.count));
    sheet.metadata.fields.push_back(EditableFloat("Respawn Time", sp.respawnTime));
    sheet.metadata.fields.push_back(EditableBool("Aggressive", sp.aggressive));
    sheet.metadata.fields.push_back(EditableInt("Tactics ID", sp.tacticsId));
    sheet.metadata.fields.push_back(EditableInt("Hive ID", sp.hiveId));
    sheet.metadata.fields.push_back(EditableInt("Nest ID", sp.nestId));

    return sheet;
}

EntityPropertySheet BuildTeleport(const TeleportPoint& tp) {
    EntityPropertySheet sheet;
    sheet.kind = EntityKind::TeleportPoint;
    sheet.title = tp.id;

    sheet.transform.fields.push_back(EditableFloat3("Source Position", tp.sourcePosition));
    sheet.transform.fields.push_back(EditableFloat3("Dest Position", tp.destinationPosition));
    sheet.editPosition = tp.sourcePosition;
    sheet.editDestPosition = tp.destinationPosition;

    sheet.metadata.fields.push_back(ReadOnlyText("ID", tp.id));
    sheet.metadata.fields.push_back(ReadOnlyInt("Source Region", tp.sourceRegionId));
    sheet.metadata.fields.push_back(EditableInt("Dest Region", tp.destinationRegionId));
    sheet.metadata.fields.push_back(EditableInt("Required Level", tp.requiredLevel));
    sheet.metadata.fields.push_back(EditableInt("Required Gold", tp.requiredGold));
    sheet.metadata.fields.push_back(EditableBool("Enabled", tp.enabled));

    return sheet;
}

EntityPropertySheet BuildZone(const Zone& z) {
    EntityPropertySheet sheet;
    sheet.kind = EntityKind::Zone;
    sheet.title = z.zoneType.empty() ? z.id : z.zoneType;

    const Vector3 size = z.boundsMax - z.boundsMin;
    char sizeBuf[128];
    std::snprintf(sizeBuf, sizeof(sizeBuf), "%.1f x %.1f x %.1f", size.x, size.y, size.z);
    sheet.geometry.fields.push_back(ReadOnlyText("Bounds Min",
        std::to_string(z.boundsMin.x) + ", " + std::to_string(z.boundsMin.y) + ", " + std::to_string(z.boundsMin.z)));
    sheet.geometry.fields.push_back(ReadOnlyText("Bounds Max",
        std::to_string(z.boundsMax.x) + ", " + std::to_string(z.boundsMax.y) + ", " + std::to_string(z.boundsMax.z)));
    sheet.geometry.fields.push_back(ReadOnlyText("Size (W x H x D)", sizeBuf));

    sheet.metadata.fields.push_back(ReadOnlyText("ID", z.id));
    sheet.metadata.fields.push_back(ReadOnlyText("Type", z.zoneType));
    sheet.metadata.fields.push_back(ReadOnlyInt("Region ID", z.regionId));
    sheet.metadata.fields.push_back(EditableInt("Priority", z.priority));
    sheet.metadata.fields.push_back(EditableBool("PvP", z.pvpEnabled));
    sheet.metadata.fields.push_back(EditableBool("Job Only", z.jobOnly));
    sheet.metadata.fields.push_back(EditableBool("Safe Zone", z.safeZone));
    sheet.metadata.fields.push_back(EditableBool("Event Zone", z.eventZone));

    return sheet;
}

EntityPropertySheet BuildRegion(const Region& r) {
    EntityPropertySheet sheet;
    sheet.kind = EntityKind::Region;
    sheet.title = r.name.empty() ? ("Region " + std::to_string(r.id)) : r.name;

    sheet.geometry.fields.push_back(ReadOnlyInt("Object Count", static_cast<int>(r.objects.size())));
    sheet.geometry.fields.push_back(ReadOnlyInt("NPC Count", static_cast<int>(r.npcs.size())));
    sheet.geometry.fields.push_back(ReadOnlyInt("Spawn Count", static_cast<int>(r.spawns.size())));

    sheet.metadata.fields.push_back(ReadOnlyInt("Region ID", r.id));
    sheet.metadata.fields.push_back(ReadOnlyText("Name", r.name));
    sheet.metadata.fields.push_back(ReadOnlyText("Modified", r.modified ? "Yes" : "No"));
    sheet.metadata.fields.push_back(ReadOnlyText("Locked", r.locked ? "Yes" : "No"));
    sheet.metadata.fields.push_back(ReadOnlyText("Has Collision", r.hasCollision ? "Yes" : "No"));
    sheet.metadata.fields.push_back(ReadOnlyInt("Teleports", static_cast<int>(r.teleports.size())));
    sheet.metadata.fields.push_back(ReadOnlyInt("Zones", static_cast<int>(r.zones.size())));

    return sheet;
}

} // namespace

EntityPropertySheet EntityPropertyBuilder::Build(const EditorContext& ctx, const SelectionId& sel) {
    EntityPropertySheet empty;
    empty.title = "Unknown";

    switch (sel.kind) {
    case EntityKind::MapPlacement: {
        int rx = 0, ry = 0;
        EditorContext::DecodeRegionId(sel.regionId, rx, ry);
        int16_t uid = static_cast<int16_t>(std::stoi(sel.id));
        if (auto* p = const_cast<EditorContext&>(ctx).FindPlacement(uid, rx, ry))
            return BuildMapPlacement(*p, ctx.sroMeshRenderer, ctx.sroRenderManager);
        break;
    }
    case EntityKind::WorldObject:
        if (auto* obj = const_cast<World&>(ctx.world).FindObject(sel.id, sel.regionId))
            return BuildWorldObject(*obj, ctx.sroMeshRenderer);
        break;
    case EntityKind::Npc:
        if (auto* npc = const_cast<World&>(ctx.world).FindNpc(sel.id, sel.regionId))
            return BuildNpc(*npc, ctx.sroMeshRenderer, ctx.sroTextData);
        break;
    case EntityKind::SpawnPoint:
        if (auto* sp = const_cast<World&>(ctx.world).FindSpawn(sel.id, sel.regionId))
            return BuildSpawn(*sp);
        break;
    case EntityKind::TeleportPoint:
        if (auto* tp = const_cast<World&>(ctx.world).FindTeleport(sel.id, sel.regionId))
            return BuildTeleport(*tp);
        break;
    case EntityKind::Zone:
        if (auto* z = const_cast<World&>(ctx.world).FindZone(sel.id, sel.regionId))
            return BuildZone(*z);
        break;
    case EntityKind::Region:
        if (auto* r = ctx.world.FindRegion(sel.regionId))
            return BuildRegion(*r);
        break;
    }
    return empty;
}
