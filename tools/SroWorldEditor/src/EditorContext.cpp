#include "EditorContext.h"
#include "validation/Validator.h"
#include <filesystem>

void EditorContext::Select(const SelectionId& id) {
    selection = id;
    selectedObjectName = SelectionDisplayName();
}

void EditorContext::ClearSelection() {
    selection.reset();
    selectedObjectName.clear();
}

void EditorContext::MarkModified() {
    project.modified = true;
    if (auto* r = world.FindRegion(activeRegionId)) {
        r->modified = true;
    }
}

void EditorContext::RefreshValidation() {
    validationMessages = Validator::ValidateWorld(world);
    auto sample = Validator::RunSampleValidation();
    validationMessages.insert(validationMessages.end(), sample.begin(), sample.end());
}

PlacementVM* EditorContext::FindPlacement(int16_t uid, int rx, int ry) {
    if (!sroPlacements) return nullptr;
    for (auto& p : *sroPlacements) {
        if (p.Object.UID == uid && p.LoadedRx == rx && p.LoadedRy == ry)
            return &p;
    }
    return nullptr;
}

PlacementVM* EditorContext::FindPlacementBySelection() {
    if (!selection || selection->kind != EntityKind::MapPlacement) return nullptr;
    int rx = 0, ry = 0;
    DecodeRegionId(selection->regionId, rx, ry);
    int16_t uid = static_cast<int16_t>(std::stoi(selection->id));
    return FindPlacement(uid, rx, ry);
}

std::string EditorContext::SelectionDisplayName() const {
    if (!selection) return "";
    switch (selection->kind) {
    case EntityKind::MapPlacement: {
        if (auto* p = const_cast<EditorContext*>(this)->FindPlacementBySelection()) {
            if (!p->BsrPath.empty())
                return std::filesystem::path(p->BsrPath).filename().string();
            return "Obj " + std::to_string(p->Object.ObjID);
        }
        break;
    }
    case EntityKind::WorldObject:
        if (auto* o = const_cast<World&>(world).FindObject(selection->id, selection->regionId)) return o->name;
        break;
    case EntityKind::Npc:
        if (auto* n = const_cast<World&>(world).FindNpc(selection->id, selection->regionId)) return n->codeName;
        break;
    case EntityKind::SpawnPoint:
        if (auto* s = const_cast<World&>(world).FindSpawn(selection->id, selection->regionId)) return s->monsterCodeName;
        break;
    case EntityKind::TeleportPoint:
        if (auto* t = const_cast<World&>(world).FindTeleport(selection->id, selection->regionId)) return t->id;
        break;
    case EntityKind::Zone:
        if (auto* z = const_cast<World&>(world).FindZone(selection->id, selection->regionId)) return z->zoneType;
        break;
    case EntityKind::Region:
        if (auto* r = world.FindRegion(selection->regionId)) return r->name;
        break;
    }
    return selection->id;
}
