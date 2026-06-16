#pragma once
#include <cstdint>
#include <string>

enum class EditorToolType {
    Select, Move, Rotate, Scale, Terrain,
    ObjectPlacement, SpawnPlacement, NpcPlacement, TeleportPlacement,
    ZonePaint, CollisionPaint, Measure
};

enum class ViewportMode {
    Lit, Wireframe, Collision, ZoneOverlay, SpawnOverlay, RegionDebug
};

enum class EntityKind {
    WorldObject, MapPlacement, Npc, SpawnPoint, TeleportPoint, Zone, Region
};

struct SelectionId {
    EntityKind kind = EntityKind::WorldObject;
    int regionId = 0;
    std::string id;
    bool operator==(const SelectionId& other) const {
        return kind == other.kind && regionId == other.regionId && id == other.id;
    }
};

struct PanelVisibility {
    bool viewport = true;
    bool worldOutliner = true;
    bool properties = true;
    bool assetBrowser = true;
    bool regionManager = false;
    bool worldMap = false;
    bool console = true;
    bool validation = true;
    bool performance = true;
    bool spawnEditor = false;
    bool npcEditor = false;
    bool teleportEditor = false;
    bool zoneEditor = false;
    bool collisionViewer = false;
};

struct ViewportSettings {
    bool showGrid = true;
    bool showRegionBounds = true;
    bool showCollision = false;
    bool showSpawns = true;
    bool showNpcs = true;
    bool showTeleports = true;
    bool showEventDecors = false;
    bool snapToGrid = false;
    bool snapToGround = true;
    bool localTransform = false;
    float cameraSpeed = 250.0f;
    ViewportMode viewMode = ViewportMode::Lit;
};
