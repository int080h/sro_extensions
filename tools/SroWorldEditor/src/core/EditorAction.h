#pragma once
#include <vector>
#include <string>
#include <memory>
#include "Core/Math.h"
#include "formats/MapFormats.h"
#include "formats/NavMeshFormat.h"
#include "Core/EditorTypes.h"

class RegionManager;

class EditorAction {
public:
    virtual ~EditorAction() = default;
    virtual void Undo(RegionManager* regionManager) = 0;
    virtual void Redo(RegionManager* regionManager) = 0;
    virtual std::string GetName() const = 0;
};

class ModifyObjectAction : public EditorAction {
private:
    int16_t m_uid;
    int m_rx;
    int m_ry;
    Vector3 m_oldPos;
    float m_oldYaw;
    Vector3 m_newPos;
    float m_newYaw;

public:
    ModifyObjectAction(int16_t uid, int rx, int ry, const Vector3& oldPos, float oldYaw, const Vector3& newPos, float newYaw)
        : m_uid(uid), m_rx(rx), m_ry(ry), m_oldPos(oldPos), m_oldYaw(oldYaw), m_newPos(newPos), m_newYaw(newYaw) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return "Modify Object Transform"; }
};

class SpawnObjectAction : public EditorAction {
private:
    sro::formats::MapObject m_obj;
    std::string m_bsrPath;
    int m_blockZ;
    int m_blockX;
    int m_lod;

public:
    SpawnObjectAction(const sro::formats::MapObject& obj, const std::string& bsrPath, int bz, int bx, int lod)
        : m_obj(obj), m_bsrPath(bsrPath), m_blockZ(bz), m_blockX(bx), m_lod(lod) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return "Spawn Object"; }
};

class DeleteObjectAction : public EditorAction {
private:
    sro::formats::MapObject m_obj;
    std::string m_bsrPath;
    int m_blockZ;
    int m_blockX;
    int m_lod;
    int m_index;

public:
    DeleteObjectAction(const sro::formats::MapObject& obj, const std::string& bsrPath, int bz, int bx, int lod, int index)
        : m_obj(obj), m_bsrPath(bsrPath), m_blockZ(bz), m_blockX(bx), m_lod(lod), m_index(index) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return "Delete Object"; }
};

class ModifyTerrainHeightAction : public EditorAction {
private:
    std::vector<HeightDelta> m_deltas;
    int m_rx;
    int m_ry;

public:
    ModifyTerrainHeightAction(int rx, int ry, const std::vector<HeightDelta>& deltas)
        : m_rx(rx), m_ry(ry), m_deltas(deltas) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return "Modify Terrain Heights"; }
};

class PaintTerrainTextureAction : public EditorAction {
private:
    std::vector<TextureDelta> m_deltas;
    int m_rx;
    int m_ry;

public:
    PaintTerrainTextureAction(int rx, int ry, const std::vector<TextureDelta>& deltas)
        : m_rx(rx), m_ry(ry), m_deltas(deltas) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return "Paint Terrain Texture"; }
};

class PaintNavmeshFlagsAction : public EditorAction {
private:
    std::vector<NavmeshFlagDelta> m_deltas;
    int m_rx;
    int m_ry;

public:
    PaintNavmeshFlagsAction(int rx, int ry, const std::vector<NavmeshFlagDelta>& deltas)
        : m_rx(rx), m_ry(ry), m_deltas(deltas) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return "Paint Navmesh Flags"; }
};

class ModifyWaterAction : public EditorAction {
private:
    int m_blockIdx;
    int8_t m_oldWaterType;
    uint8_t m_oldWaveType;
    float m_oldWaterHeight;
    int8_t m_newWaterType;
    uint8_t m_newWaveType;
    float m_newWaterHeight;
    int m_rx;
    int m_ry;

public:
    ModifyWaterAction(int rx, int ry, int blockIdx, int8_t oldType, uint8_t oldWave, float oldHeight,
                      int8_t newType, uint8_t newWave, float newHeight)
        : m_rx(rx), m_ry(ry), m_blockIdx(blockIdx),
          m_oldWaterType(oldType), m_oldWaveType(oldWave), m_oldWaterHeight(oldHeight),
          m_newWaterType(newType), m_newWaveType(newWave), m_newWaterHeight(newHeight) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return "Modify Water Properties"; }
};

class ModifyNavmeshEdgeAction : public EditorAction {
private:
    int m_rx;
    int m_ry;
    bool m_isGlobal;
    bool m_isDelete;
    int m_edgeIdx;
    sro::formats::NavGlobalEdge m_oldGlobalEdge;
    sro::formats::NavGlobalEdge m_newGlobalEdge;
    sro::formats::NavInternalEdge m_oldInternalEdge;
    sro::formats::NavInternalEdge m_newInternalEdge;

public:
    ModifyNavmeshEdgeAction(int rx, int ry, int edgeIdx, const sro::formats::NavGlobalEdge& oldEdge, const sro::formats::NavGlobalEdge& newEdge, bool isDelete)
        : m_rx(rx), m_ry(ry), m_isGlobal(true), m_isDelete(isDelete), m_edgeIdx(edgeIdx),
          m_oldGlobalEdge(oldEdge), m_newGlobalEdge(newEdge) {}

    ModifyNavmeshEdgeAction(int rx, int ry, int edgeIdx, const sro::formats::NavInternalEdge& oldEdge, const sro::formats::NavInternalEdge& newEdge, bool isDelete)
        : m_rx(rx), m_ry(ry), m_isGlobal(false), m_isDelete(isDelete), m_edgeIdx(edgeIdx),
          m_oldInternalEdge(oldEdge), m_newInternalEdge(newEdge) {}

    void Undo(RegionManager* regionManager) override;
    void Redo(RegionManager* regionManager) override;
    std::string GetName() const override { return m_isDelete ? "Delete Navmesh Edge" : "Modify Navmesh Edge"; }
};
