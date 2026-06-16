#pragma once
#include "world/WorldObject.h"
#include <vector>
#include <optional>
#include <algorithm>

struct Region {
    int id = 0;
    std::string name;
    bool modified = false;
    bool locked = false;
    bool hasCollision = false;
    std::vector<WorldObject> objects;
    std::vector<NPC> npcs;
    std::vector<SpawnPoint> spawns;
    std::vector<TeleportPoint> teleports;
    std::vector<Zone> zones;
};

class World {
public:
    static World CreateSampleWorld();

    Region* FindRegion(int regionId);
    const Region* FindRegion(int regionId) const;
    WorldObject* FindObject(const std::string& id, int regionId);
    const WorldObject* FindObject(const std::string& id, int regionId) const;
    NPC* FindNpc(const std::string& id, int regionId);
    const NPC* FindNpc(const std::string& id, int regionId) const;
    SpawnPoint* FindSpawn(const std::string& id, int regionId);
    const SpawnPoint* FindSpawn(const std::string& id, int regionId) const;
    TeleportPoint* FindTeleport(const std::string& id, int regionId);
    const TeleportPoint* FindTeleport(const std::string& id, int regionId) const;
    Zone* FindZone(const std::string& id, int regionId);
    const Zone* FindZone(const std::string& id, int regionId) const;

    std::vector<Region> regions;
    std::vector<Region> dungeons;
    std::vector<Region> fortressAreas;
    std::vector<Region> eventZones;
};
