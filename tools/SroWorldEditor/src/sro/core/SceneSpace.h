#pragma once
#include "Constants.h"
#include "RegionCoord.h"
#include "RegionId.h"
#include "core/MathTypes.h"
#include <cmath>

namespace sro {

struct WorldPos {
    float x = 0; // East-West (SRO world)
    float y = 0; // South-North (SRO world)
};

struct LocalPos {
    int rx = 0;
    int ry = 0;
    float localX = 0;
    float localZ = 0;

    int sectorX() const { return static_cast<int>(localX / TILE_SIZE); }
    int sectorY() const { return static_cast<int>(localZ / TILE_SIZE); }
    RegionId regionId() const { return RegionId::FromRxRy(rx, ry); }
};

struct SceneSpace {
    static LocalPos SceneToLocal(float sceneX, float sceneZ, int centerRx, int centerRy) {
        LocalPos lp;
        lp.rx = centerRx + static_cast<int>(std::floor(sceneZ / REGION_SIZE));
        lp.ry = centerRy + static_cast<int>(std::floor(sceneX / REGION_SIZE));
        lp.localX = sceneX - (lp.ry - centerRy) * REGION_SIZE;
        lp.localZ = sceneZ - (lp.rx - centerRx) * REGION_SIZE;
        return lp;
    }

    static WorldPos LocalToWorld(const LocalPos& lp) {
        WorldPos wp;
        wp.x = (lp.ry - CENTER_RY) * SRO_REGION_SIZE + lp.localX / 10.0f;
        wp.y = (lp.rx - CENTER_RX) * SRO_REGION_SIZE + lp.localZ / 10.0f;
        return wp;
    }

    static WorldPos SceneToWorld(float sceneX, float sceneZ, int centerRx, int centerRy) {
        return LocalToWorld(SceneToLocal(sceneX, sceneZ, centerRx, centerRy));
    }

    static float SceneOffsetX(int ry, int centerRy) {
        return (ry - centerRy) * REGION_SIZE;
    }

    static float SceneOffsetZ(int rx, int centerRx) {
        return (rx - centerRx) * REGION_SIZE;
    }

    static float ObjectOffsetX(int objRy, int centerRy) {
        return (objRy - centerRy) * REGION_SIZE;
    }

    static float ObjectOffsetZ(int objRx, int centerRx) {
        return (objRx - centerRx) * REGION_SIZE;
    }

    static Vector3 ObjectWorldPosition(const Vector3& localPos, int objRx, int objRy, int centerRx, int centerRy) {
        float oX = ObjectOffsetX(objRy, centerRy);
        float oZ = ObjectOffsetZ(objRx, centerRx);
        return Vector3(localPos.x + oX, localPos.y, localPos.z + oZ);
    }

    static Vector3 ObjectWorldPositionFromRegionId(const Vector3& localPos, uint16_t regionId, int centerRx, int centerRy) {
        RegionId rid(regionId);
        return ObjectWorldPosition(localPos, rid.Rx(), rid.Ry(), centerRx, centerRy);
    }
};

} // namespace sro
