#include "world/Region.h"

World World::CreateSampleWorld() {
    World world;
    for (int rid = 25000; rid <= 25002; ++rid) {
        Region r;
        r.id = rid;
        r.name = "Region " + std::to_string(rid);
        r.hasCollision = true;
        r.modified = (rid == 25001);

        WorldObject gate;
        gate.id = "obj_gate_" + std::to_string(rid);
        gate.name = "JanganGate01";
        gate.modelPath = "res/bldg/jangan_gate.bsr";
        gate.regionId = rid;
        gate.transform.position = Vector3(960.0f, 0.0f, 960.0f);
        r.objects.push_back(gate);

        WorldObject tree;
        tree.id = "obj_tree_" + std::to_string(rid);
        tree.name = "TreeOak01";
        tree.modelPath = "res/nature/tree_oak.bsr";
        tree.regionId = rid;
        tree.transform.position = Vector3(500.0f, 0.0f, 700.0f);
        r.objects.push_back(tree);

        NPC npc;
        npc.id = "npc_" + std::to_string(rid);
        npc.codeName = "NPC_CH_SMITH";
        npc.regionId = rid;
        npc.transform.position = Vector3(800.0f, 0.0f, 800.0f);
        npc.shopLink = 1001;
        r.npcs.push_back(npc);

        SpawnPoint sp;
        sp.id = "spawn_" + std::to_string(rid);
        sp.monsterCodeName = "MOB_CH_MANGNYANG";
        sp.regionId = rid;
        sp.position = Vector3(1200.0f, 0.0f, 1200.0f);
        sp.count = 3;
        r.spawns.push_back(sp);

        TeleportPoint tp;
        tp.id = "tp_" + std::to_string(rid);
        tp.sourceRegionId = rid;
        tp.sourcePosition = Vector3(100.0f, 0.0f, 100.0f);
        tp.destinationRegionId = rid + 1;
        tp.destinationPosition = Vector3(1800.0f, 0.0f, 1800.0f);
        tp.requiredLevel = 1;
        r.teleports.push_back(tp);

        Zone zone;
        zone.id = "zone_" + std::to_string(rid);
        zone.zoneType = "SafeZone";
        zone.regionId = rid;
        zone.boundsMin = Vector3(0, 0, 0);
        zone.boundsMax = Vector3(500, 100, 500);
        zone.safeZone = true;
        r.zones.push_back(zone);

        world.regions.push_back(std::move(r));
    }
    return world;
}

Region* World::FindRegion(int regionId) {
    for (auto& r : regions) if (r.id == regionId) return &r;
    return nullptr;
}

const Region* World::FindRegion(int regionId) const {
    for (const auto& r : regions) if (r.id == regionId) return &r;
    return nullptr;
}

WorldObject* World::FindObject(const std::string& id, int regionId) {
    if (auto* r = FindRegion(regionId)) {
        for (auto& o : r->objects) if (o.id == id) return &o;
    }
    return nullptr;
}

NPC* World::FindNpc(const std::string& id, int regionId) {
    if (auto* r = FindRegion(regionId)) {
        for (auto& n : r->npcs) if (n.id == id) return &n;
    }
    return nullptr;
}

SpawnPoint* World::FindSpawn(const std::string& id, int regionId) {
    if (auto* r = FindRegion(regionId)) {
        for (auto& s : r->spawns) if (s.id == id) return &s;
    }
    return nullptr;
}

TeleportPoint* World::FindTeleport(const std::string& id, int regionId) {
    if (auto* r = FindRegion(regionId)) {
        for (auto& t : r->teleports) if (t.id == id) return &t;
    }
    return nullptr;
}

Zone* World::FindZone(const std::string& id, int regionId) {
    if (auto* r = FindRegion(regionId)) {
        for (auto& z : r->zones) if (z.id == id) return &z;
    }
    return nullptr;
}

const WorldObject* World::FindObject(const std::string& id, int regionId) const {
    if (const auto* r = FindRegion(regionId)) {
        for (const auto& o : r->objects) if (o.id == id) return &o;
    }
    return nullptr;
}

const NPC* World::FindNpc(const std::string& id, int regionId) const {
    if (const auto* r = FindRegion(regionId)) {
        for (const auto& n : r->npcs) if (n.id == id) return &n;
    }
    return nullptr;
}

const SpawnPoint* World::FindSpawn(const std::string& id, int regionId) const {
    if (const auto* r = FindRegion(regionId)) {
        for (const auto& s : r->spawns) if (s.id == id) return &s;
    }
    return nullptr;
}

const TeleportPoint* World::FindTeleport(const std::string& id, int regionId) const {
    if (const auto* r = FindRegion(regionId)) {
        for (const auto& t : r->teleports) if (t.id == id) return &t;
    }
    return nullptr;
}

const Zone* World::FindZone(const std::string& id, int regionId) const {
    if (const auto* r = FindRegion(regionId)) {
        for (const auto& z : r->zones) if (z.id == id) return &z;
    }
    return nullptr;
}
