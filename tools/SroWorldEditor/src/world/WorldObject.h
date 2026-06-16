#pragma once
#include "core/MathTypes.h"
#include <string>

struct Transform {
    Vector3 position{0, 0, 0};
    Vector3 rotation{0, 0, 0};
    Vector3 scale{1, 1, 1};
};

struct WorldObject {
    std::string id;
    std::string name;
    std::string modelPath;
    int regionId = 0;
    Transform transform;
    bool collisionEnabled = true;
    bool visible = true;
    bool locked = false;
    std::string notes;
};

struct SpawnPoint {
    std::string id;
    std::string monsterCodeName;
    int regionId = 0;
    Vector3 position;
    float radius = 10.0f;
    int count = 1;
    float respawnTime = 60.0f;
    bool aggressive = true;
    int tacticsId = 0;
    int hiveId = 0;
    int nestId = 0;
    float championRatio = 0.0f;
    float partyMobRatio = 0.0f;
};

struct NPC {
    std::string id;
    std::string codeName;
    int regionId = 0;
    Transform transform;
    int shopLink = 0;
    int questLink = 0;
    int dialogLink = 0;
    int teleportLink = 0;
};

struct TeleportPoint {
    std::string id;
    Vector3 sourcePosition;
    int sourceRegionId = 0;
    int destinationRegionId = 0;
    Vector3 destinationPosition;
    int requiredLevel = 0;
    int requiredGold = 0;
    std::string requiredItem;
    int requiredQuest = 0;
    bool enabled = true;
};

struct Zone {
    std::string id;
    std::string zoneType;
    int regionId = 0;
    Vector3 boundsMin;
    Vector3 boundsMax;
    int priority = 0;
    bool pvpEnabled = false;
    bool jobOnly = false;
    bool safeZone = false;
    bool eventZone = false;
};
