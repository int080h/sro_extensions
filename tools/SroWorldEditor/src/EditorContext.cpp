#include "EditorContext.h"
#include "validation/Validator.h"
#include "assets/AssetResolver.h"
#include "core/FileSystem.h"
#include "index/TextDataCatalog.h"
#include <filesystem>
#include <set>

void EditorContext::Select(const SelectionId& id) {
    selection = id;
    selectedObjectName = SelectionDisplayName();
    if (objectViewer.followSelection)
        SyncObjectViewerFromSelection();
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

    if (sroPlacements) {
        auto collisionMsgs = Validator::ValidateCollisions(*sroPlacements,
            EncodeRegionId(sroRegionRx, sroRegionRy));
        validationMessages.insert(validationMessages.end(), collisionMsgs.begin(), collisionMsgs.end());
    }

    if (!sroPlacements) return;

    std::set<std::string> placementKeys;
    for (const auto& p : *sroPlacements) {
        const int regionId = EncodeRegionId(p.LoadedRx, p.LoadedRy);
        const std::string uidText = std::to_string(p.Object.UID);
        const std::string key = std::to_string(p.LoadedRx) + ":" + std::to_string(p.LoadedRy) + ":" + uidText;

        if (!placementKeys.insert(key).second) {
            validationMessages.push_back({
                ValidationSeverity::Error,
                "Duplicate placement UID in loaded SRO region: " + uidText,
                regionId,
                uidText
            });
        }

        if (p.Object.ObjID == 0) {
            validationMessages.push_back({
                ValidationSeverity::Error,
                "Placement has ObjID 0.",
                regionId,
                uidText
            });
        }

        if (p.Object.RegionID != 0 && static_cast<int>(p.Object.RegionID) != regionId) {
            validationMessages.push_back({
                ValidationSeverity::Warning,
                "Placement RegionID does not match loaded region.",
                regionId,
                uidText
            });
        }

        std::string bsrPath = p.BsrPath;
        if (bsrPath.empty() && sroAssets)
            bsrPath = sroAssets->ResolveObjectBsr(p.Object.ObjID);

        if (bsrPath.empty()) {
            validationMessages.push_back({
                ValidationSeverity::Error,
                "Placement model path could not be resolved.",
                regionId,
                uidText
            });
        } else if (sroAssets) {
            const std::wstring assetPath = sroAssets->ResolveAssetPath(bsrPath);
            if (!assetPath.empty() && !FileExists(assetPath)) {
                validationMessages.push_back({
                    ValidationSeverity::Error,
                    "Placement model file is missing: " + bsrPath,
                    regionId,
                    uidText
                });
            }
        }
    }
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

void EditorContext::InspectGameObject(const std::string& codeName) {
    objectViewer.source = ObjectInspectSource::GameObject;
    objectViewer.inspectedCodeName = codeName;
    objectViewer.inspectedBsrPath.clear();
    objectViewer.inspectedObjId = 0;
    objectViewer.selectedAnimPath.clear();
    objectViewer.animTimeMs = 0.f;
    objectViewer.ResetPreviewCamera();
    if (sroTextData) {
        if (const auto* ref = sroTextData->FindByCodeName(codeName)) {
            objectViewer.inspectedBsrPath = sroTextData->ResolvePrimaryModelPath(*ref);
        }
    }
    panels.objectViewer = true;
}

void EditorContext::InspectBsrPath(const std::string& bsrPath) {
    objectViewer.source = ObjectInspectSource::BsrPath;
    objectViewer.inspectedBsrPath = bsrPath;
    objectViewer.inspectedCodeName.clear();
    objectViewer.inspectedObjId = 0;
    objectViewer.selectedAnimPath.clear();
    objectViewer.animTimeMs = 0.f;
    objectViewer.ResetPreviewCamera();
    panels.objectViewer = true;
}

void EditorContext::InspectObjId(uint32_t objId) {
    objectViewer.source = ObjectInspectSource::ObjId;
    objectViewer.inspectedObjId = objId;
    objectViewer.inspectedCodeName.clear();
    objectViewer.inspectedBsrPath.clear();
    objectViewer.selectedAnimPath.clear();
    objectViewer.animTimeMs = 0.f;
    objectViewer.ResetPreviewCamera();
    if (sroAssets) {
        objectViewer.inspectedBsrPath = sroAssets->ResolveObjectBsr(objId);
    }
    panels.objectViewer = true;
}

void EditorContext::SyncObjectViewerFromSelection() {
    if (!selection) return;
    switch (selection->kind) {
    case EntityKind::MapPlacement: {
        if (auto* p = const_cast<EditorContext*>(this)->FindPlacementBySelection()) {
            if (!p->BsrPath.empty()) {
                InspectBsrPath(p->BsrPath);
            } else if (p->Object.ObjID != 0) {
                InspectObjId(p->Object.ObjID);
            }
        }
        break;
    }
    case EntityKind::Npc:
        if (auto* n = world.FindNpc(selection->id, selection->regionId)) {
            InspectGameObject(n->codeName);
        }
        break;
    case EntityKind::WorldObject:
        if (auto* o = world.FindObject(selection->id, selection->regionId)) {
            if (!o->modelPath.empty()) InspectBsrPath(o->modelPath);
        }
        break;
    default:
        break;
    }
}
