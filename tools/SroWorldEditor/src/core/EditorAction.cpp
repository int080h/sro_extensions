#include "EditorAction.h"
#include "runtime/RegionManager.h"
#include "Rendering/RenderManager.h"
#include "Rendering/TerrainRenderer.h"
#include "Rendering/NvmRenderer.h"
#include <algorithm>

// Extern functions declared in main.cpp for synchronizing global memory lists
extern void SyncPlacementGlobalVector(int16_t uid, int rx, int ry, const Vector3& pos, float yaw, bool isDelete, const sro::formats::MapObject* spawnObj = nullptr, const std::string& bsrPath = "");

// ============================================================================
// ModifyObjectAction Implementation
// ============================================================================
void ModifyObjectAction::Undo(RegionManager* regionManager) {
    auto* placements = regionManager->GetPlacements(m_rx, m_ry);
    if (placements) {
        bool found = false;
        for (int z = 0; z < 6; ++z) {
            for (int x = 0; x < 6; ++x) {
                for (int lod = 0; lod < 4; ++lod) {
                    for (auto& obj : placements->Blocks[z][x][lod]) {
                        if (obj.UID == m_uid) {
                            obj.PosX = m_oldPos.x;
                            obj.PosY = m_oldPos.y;
                            obj.PosZ = m_oldPos.z;
                            obj.Yaw = m_oldYaw;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                if (found) break;
            }
            if (found) break;
        }
    }

    auto* nvm = regionManager->GetNavMesh(m_rx, m_ry);
    if (nvm) {
        for (auto& obj : nvm->Objects) {
            if (obj.UID == m_uid) {
                obj.PosX = m_oldPos.x;
                obj.PosY = m_oldPos.y;
                obj.PosZ = m_oldPos.z;
                obj.Yaw = m_oldYaw;
                break;
            }
        }
    }

    SyncPlacementGlobalVector(m_uid, m_rx, m_ry, m_oldPos, m_oldYaw, false);
}

void ModifyObjectAction::Redo(RegionManager* regionManager) {
    auto* placements = regionManager->GetPlacements(m_rx, m_ry);
    if (placements) {
        bool found = false;
        for (int z = 0; z < 6; ++z) {
            for (int x = 0; x < 6; ++x) {
                for (int lod = 0; lod < 4; ++lod) {
                    for (auto& obj : placements->Blocks[z][x][lod]) {
                        if (obj.UID == m_uid) {
                            obj.PosX = m_newPos.x;
                            obj.PosY = m_newPos.y;
                            obj.PosZ = m_newPos.z;
                            obj.Yaw = m_newYaw;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                if (found) break;
            }
            if (found) break;
        }
    }

    auto* nvm = regionManager->GetNavMesh(m_rx, m_ry);
    if (nvm) {
        for (auto& obj : nvm->Objects) {
            if (obj.UID == m_uid) {
                obj.PosX = m_newPos.x;
                obj.PosY = m_newPos.y;
                obj.PosZ = m_newPos.z;
                obj.Yaw = m_newYaw;
                break;
            }
        }
    }

    SyncPlacementGlobalVector(m_uid, m_rx, m_ry, m_newPos, m_newYaw, false);
}

// ============================================================================
// SpawnObjectAction Implementation
// ============================================================================
void SpawnObjectAction::Undo(RegionManager* regionManager) {
    int rx = (m_obj.RegionID >> 8) & 0xFF;
    int ry = m_obj.RegionID & 0xFF;
    auto* placements = regionManager->GetPlacements(rx, ry);
    if (placements) {
        auto& list = placements->Blocks[m_blockZ][m_blockX][m_lod];
        list.erase(std::remove_if(list.begin(), list.end(), [&](const sro::formats::MapObject& obj) {
            return obj.UID == m_obj.UID;
        }), list.end());
    }

    auto* nvm = regionManager->GetNavMesh(rx, ry);
    if (nvm) {
        nvm->Objects.erase(std::remove_if(nvm->Objects.begin(), nvm->Objects.end(), [&](const sro::formats::NavObject& obj) {
            return obj.UID == m_obj.UID;
        }), nvm->Objects.end());
    }

    SyncPlacementGlobalVector(m_obj.UID, rx, ry, Vector3(0,0,0), 0.0f, true);
}

void SpawnObjectAction::Redo(RegionManager* regionManager) {
    int rx = (m_obj.RegionID >> 8) & 0xFF;
    int ry = m_obj.RegionID & 0xFF;
    auto* placements = regionManager->GetPlacements(rx, ry);
    if (placements) {
        placements->Blocks[m_blockZ][m_blockX][m_lod].push_back(m_obj);
    }

    auto* nvm = regionManager->GetNavMesh(rx, ry);
    if (nvm) {
        sro::formats::NavObject no;
        no.ResID = m_obj.ObjID;
        no.PosX = m_obj.PosX;
        no.PosY = m_obj.PosY;
        no.PosZ = m_obj.PosZ;
        no.Yaw = m_obj.Yaw;
        no.UID = m_obj.UID;
        no.IsStatic = m_obj.IsStatic;
        no.IsBig = m_obj.IsBig;
        no.IsStruct = m_obj.IsStruct;
        no.RegionID = m_obj.RegionID;
        no.UnkShort = m_obj.Short0;
        nvm->Objects.push_back(no);
    }

    SyncPlacementGlobalVector(m_obj.UID, rx, ry, Vector3(m_obj.PosX, m_obj.PosY, m_obj.PosZ), m_obj.Yaw, false, &m_obj, m_bsrPath);
}

// ============================================================================
// DeleteObjectAction Implementation
// ============================================================================
void DeleteObjectAction::Undo(RegionManager* regionManager) {
    // Restore deleted object back to block
    int rx = (m_obj.RegionID >> 8) & 0xFF;
    int ry = m_obj.RegionID & 0xFF;
    auto* placements = regionManager->GetPlacements(rx, ry);
    if (placements) {
        auto& list = placements->Blocks[m_blockZ][m_blockX][m_lod];
        if (m_index >= 0 && m_index <= list.size()) {
            list.insert(list.begin() + m_index, m_obj);
        } else {
            list.push_back(m_obj);
        }
    }

    auto* nvm = regionManager->GetNavMesh(rx, ry);
    if (nvm) {
        sro::formats::NavObject no;
        no.ResID = m_obj.ObjID;
        no.PosX = m_obj.PosX;
        no.PosY = m_obj.PosY;
        no.PosZ = m_obj.PosZ;
        no.Yaw = m_obj.Yaw;
        no.UID = m_obj.UID;
        no.IsStatic = m_obj.IsStatic;
        no.IsBig = m_obj.IsBig;
        no.IsStruct = m_obj.IsStruct;
        no.RegionID = m_obj.RegionID;
        no.UnkShort = m_obj.Short0;
        nvm->Objects.push_back(no);
    }

    SyncPlacementGlobalVector(m_obj.UID, rx, ry, Vector3(m_obj.PosX, m_obj.PosY, m_obj.PosZ), m_obj.Yaw, false, &m_obj, m_bsrPath);
}

void DeleteObjectAction::Redo(RegionManager* regionManager) {
    int rx = (m_obj.RegionID >> 8) & 0xFF;
    int ry = m_obj.RegionID & 0xFF;
    auto* placements = regionManager->GetPlacements(rx, ry);
    if (placements) {
        auto& list = placements->Blocks[m_blockZ][m_blockX][m_lod];
        list.erase(std::remove_if(list.begin(), list.end(), [&](const sro::formats::MapObject& obj) {
            return obj.UID == m_obj.UID;
        }), list.end());
    }

    auto* nvm = regionManager->GetNavMesh(rx, ry);
    if (nvm) {
        nvm->Objects.erase(std::remove_if(nvm->Objects.begin(), nvm->Objects.end(), [&](const sro::formats::NavObject& obj) {
            return obj.UID == m_obj.UID;
        }), nvm->Objects.end());
    }

    SyncPlacementGlobalVector(m_obj.UID, rx, ry, Vector3(0,0,0), 0.0f, true);
}

// ============================================================================
// ModifyTerrainHeightAction Implementation
// ============================================================================
void ModifyTerrainHeightAction::Undo(RegionManager* regionManager) {
    sro::formats::MapMesh* mesh = regionManager->GetRenderManager()->GetTerrainRenderer()->GetMapMesh(m_rx, m_ry);
    sro::formats::NavMesh* nvm = regionManager->GetNavMesh(m_rx, m_ry);

    std::vector<int> dirtyBlocks;

    for (const auto& delta : m_deltas) {
        if (mesh) {
            mesh->Blocks[delta.BlockIdx].Vertices[delta.VertexIdx].Height = delta.OldHeight;
            if (std::find(dirtyBlocks.begin(), dirtyBlocks.end(), delta.BlockIdx) == dirtyBlocks.end()) {
                dirtyBlocks.push_back(delta.BlockIdx);
            }
        }

        if (nvm) {
            int bx = delta.BlockIdx % 6;
            int bz = delta.BlockIdx / 6;
            int vx = delta.VertexIdx % 17;
            int vz = delta.VertexIdx / 17;
            int gx = bx * 16 + vx;
            int gz = bz * 16 + vz;
            int nvmIdx = gz * 97 + gx;
            if (nvmIdx >= 0 && nvmIdx < static_cast<int>(nvm->HeightMap.size())) {
                nvm->HeightMap[nvmIdx] = delta.OldHeight;
            }
        }
    }

    // Recalculate block heights
    if (mesh) {
        for (int blockIdx : dirtyBlocks) {
            auto& block = mesh->Blocks[blockIdx];
            float minH = 999999.0f;
            float maxH = -999999.0f;
            float sum = 0.0f;
            for (int i = 0; i < 289; ++i) {
                float h = block.Vertices[i].Height;
                if (h < minH) minH = h;
                if (h > maxH) maxH = h;
                sum += h;
            }
            block.HeightMin = minH;
            block.HeightMax = maxH;

            // Sync Plane Height
            if (nvm && blockIdx < static_cast<int>(nvm->PlaneHeightMap.size())) {
                nvm->PlaneHeightMap[blockIdx] = sum / 289.0f;
            }
        }
        regionManager->GetRenderManager()->GetTerrainRenderer()->RebuildTerrainBuffers(m_rx, m_ry);
    }

    if (nvm) {
        regionManager->GetRenderManager()->GetNvmRenderer()->RebuildNavmeshBuffers(nvm, m_rx, m_ry);
    }
}

void ModifyTerrainHeightAction::Redo(RegionManager* regionManager) {
    sro::formats::MapMesh* mesh = regionManager->GetRenderManager()->GetTerrainRenderer()->GetMapMesh(m_rx, m_ry);
    sro::formats::NavMesh* nvm = regionManager->GetNavMesh(m_rx, m_ry);

    std::vector<int> dirtyBlocks;

    for (const auto& delta : m_deltas) {
        if (mesh) {
            mesh->Blocks[delta.BlockIdx].Vertices[delta.VertexIdx].Height = delta.NewHeight;
            if (std::find(dirtyBlocks.begin(), dirtyBlocks.end(), delta.BlockIdx) == dirtyBlocks.end()) {
                dirtyBlocks.push_back(delta.BlockIdx);
            }
        }

        if (nvm) {
            int bx = delta.BlockIdx % 6;
            int bz = delta.BlockIdx / 6;
            int vx = delta.VertexIdx % 17;
            int vz = delta.VertexIdx / 17;
            int gx = bx * 16 + vx;
            int gz = bz * 16 + vz;
            int nvmIdx = gz * 97 + gx;
            if (nvmIdx >= 0 && nvmIdx < static_cast<int>(nvm->HeightMap.size())) {
                nvm->HeightMap[nvmIdx] = delta.NewHeight;
            }
        }
    }

    if (mesh) {
        for (int blockIdx : dirtyBlocks) {
            auto& block = mesh->Blocks[blockIdx];
            float minH = 999999.0f;
            float maxH = -999999.0f;
            float sum = 0.0f;
            for (int i = 0; i < 289; ++i) {
                float h = block.Vertices[i].Height;
                if (h < minH) minH = h;
                if (h > maxH) maxH = h;
                sum += h;
            }
            block.HeightMin = minH;
            block.HeightMax = maxH;

            if (nvm && blockIdx < static_cast<int>(nvm->PlaneHeightMap.size())) {
                nvm->PlaneHeightMap[blockIdx] = sum / 289.0f;
            }
        }
        regionManager->GetRenderManager()->GetTerrainRenderer()->RebuildTerrainBuffers(m_rx, m_ry);
    }

    if (nvm) {
        regionManager->GetRenderManager()->GetNvmRenderer()->RebuildNavmeshBuffers(nvm, m_rx, m_ry);
    }
}

// ============================================================================
// PaintTerrainTextureAction Implementation
// ============================================================================
void PaintTerrainTextureAction::Undo(RegionManager* regionManager) {
    sro::formats::MapMesh* mesh = regionManager->GetRenderManager()->GetTerrainRenderer()->GetMapMesh(m_rx, m_ry);
    if (!mesh) return;

    for (const auto& delta : m_deltas) {
        auto& block = mesh->Blocks[delta.BlockIdx];
        block.Tiles[delta.TileIdx] = delta.OldTileVal;

        int tx = delta.TileIdx % 16;
        int tz = delta.TileIdx / 16;
        block.Vertices[tz * 17 + tx].TextureData = delta.OldTextureData[0];
        block.Vertices[tz * 17 + (tx + 1)].TextureData = delta.OldTextureData[1];
        block.Vertices[(tz + 1) * 17 + tx].TextureData = delta.OldTextureData[2];
        block.Vertices[(tz + 1) * 17 + (tx + 1)].TextureData = delta.OldTextureData[3];
    }

    regionManager->GetRenderManager()->GetTerrainRenderer()->RebuildTerrainBuffers(m_rx, m_ry);
}

void PaintTerrainTextureAction::Redo(RegionManager* regionManager) {
    sro::formats::MapMesh* mesh = regionManager->GetRenderManager()->GetTerrainRenderer()->GetMapMesh(m_rx, m_ry);
    if (!mesh) return;

    for (const auto& delta : m_deltas) {
        auto& block = mesh->Blocks[delta.BlockIdx];
        block.Tiles[delta.TileIdx] = delta.NewTileVal;

        int tx = delta.TileIdx % 16;
        int tz = delta.TileIdx / 16;
        block.Vertices[tz * 17 + tx].TextureData = delta.NewTextureData[0];
        block.Vertices[tz * 17 + (tx + 1)].TextureData = delta.NewTextureData[1];
        block.Vertices[(tz + 1) * 17 + tx].TextureData = delta.NewTextureData[2];
        block.Vertices[(tz + 1) * 17 + (tx + 1)].TextureData = delta.NewTextureData[3];
    }

    regionManager->GetRenderManager()->GetTerrainRenderer()->RebuildTerrainBuffers(m_rx, m_ry);
}

// ============================================================================
// PaintNavmeshFlagsAction Implementation
// ============================================================================
void PaintNavmeshFlagsAction::Undo(RegionManager* regionManager) {
    sro::formats::NavMesh* nvm = regionManager->GetNavMesh(m_rx, m_ry);
    if (!nvm) return;

    for (const auto& delta : m_deltas) {
        if (delta.TileIdx >= 0 && delta.TileIdx < static_cast<int>(nvm->TileMap.size())) {
            nvm->TileMap[delta.TileIdx].Flags = delta.OldFlags;
        }
    }

    regionManager->GetRenderManager()->GetNvmRenderer()->RebuildNavmeshBuffers(nvm, m_rx, m_ry);
}

void PaintNavmeshFlagsAction::Redo(RegionManager* regionManager) {
    sro::formats::NavMesh* nvm = regionManager->GetNavMesh(m_rx, m_ry);
    if (!nvm) return;

    for (const auto& delta : m_deltas) {
        if (delta.TileIdx >= 0 && delta.TileIdx < static_cast<int>(nvm->TileMap.size())) {
            nvm->TileMap[delta.TileIdx].Flags = delta.NewFlags;
        }
    }

    regionManager->GetRenderManager()->GetNvmRenderer()->RebuildNavmeshBuffers(nvm, m_rx, m_ry);
}

// ============================================================================
// ModifyWaterAction Implementation
// ============================================================================
void ModifyWaterAction::Undo(RegionManager* regionManager) {
    sro::formats::MapMesh* mesh = regionManager->GetRenderManager()->GetTerrainRenderer()->GetMapMesh(m_rx, m_ry);
    if (!mesh) return;

    auto& block = mesh->Blocks[m_blockIdx];
    block.WaterType = m_oldWaterType;
    block.WaterWaveType = m_oldWaveType;
    block.WaterHeight = m_oldWaterHeight;

    regionManager->GetRenderManager()->GetTerrainRenderer()->RebuildTerrainBuffers(m_rx, m_ry);
}

void ModifyWaterAction::Redo(RegionManager* regionManager) {
    sro::formats::MapMesh* mesh = regionManager->GetRenderManager()->GetTerrainRenderer()->GetMapMesh(m_rx, m_ry);
    if (!mesh) return;

    auto& block = mesh->Blocks[m_blockIdx];
    block.WaterType = m_newWaterType;
    block.WaterWaveType = m_newWaveType;
    block.WaterHeight = m_newWaterHeight;

    regionManager->GetRenderManager()->GetTerrainRenderer()->RebuildTerrainBuffers(m_rx, m_ry);
}

// ============================================================================
// ModifyNavmeshEdgeAction Implementation
// ============================================================================
void ModifyNavmeshEdgeAction::Undo(RegionManager* regionManager) {
    sro::formats::NavMesh* nvm = regionManager->GetNavMesh(m_rx, m_ry);
    if (!nvm) return;

    if (m_isGlobal) {
        if (m_isDelete) {
            // Restore deleted edge at its old position
            if (m_edgeIdx >= 0 && m_edgeIdx <= static_cast<int>(nvm->GlobalEdges.size())) {
                nvm->GlobalEdges.insert(nvm->GlobalEdges.begin() + m_edgeIdx, m_oldGlobalEdge);
            } else {
                nvm->GlobalEdges.push_back(m_oldGlobalEdge);
            }
            nvm->GlobalEdgeCount = static_cast<uint32_t>(nvm->GlobalEdges.size());
        } else {
            // Restore modified edge properties
            if (m_edgeIdx >= 0 && m_edgeIdx < static_cast<int>(nvm->GlobalEdges.size())) {
                nvm->GlobalEdges[m_edgeIdx] = m_oldGlobalEdge;
            }
        }
    } else {
        if (m_isDelete) {
            // Restore deleted edge
            if (m_edgeIdx >= 0 && m_edgeIdx <= static_cast<int>(nvm->InternalEdges.size())) {
                nvm->InternalEdges.insert(nvm->InternalEdges.begin() + m_edgeIdx, m_oldInternalEdge);
            } else {
                nvm->InternalEdges.push_back(m_oldInternalEdge);
            }
            nvm->InternalEdgeCount = static_cast<uint32_t>(nvm->InternalEdges.size());
        } else {
            // Restore modified edge
            if (m_edgeIdx >= 0 && m_edgeIdx < static_cast<int>(nvm->InternalEdges.size())) {
                nvm->InternalEdges[m_edgeIdx] = m_oldInternalEdge;
            }
        }
    }

    regionManager->GetRenderManager()->GetNvmRenderer()->RebuildNavmeshBuffers(nvm, m_rx, m_ry);
}

void ModifyNavmeshEdgeAction::Redo(RegionManager* regionManager) {
    sro::formats::NavMesh* nvm = regionManager->GetNavMesh(m_rx, m_ry);
    if (!nvm) return;

    if (m_isGlobal) {
        if (m_isDelete) {
            // Delete edge
            if (m_edgeIdx >= 0 && m_edgeIdx < static_cast<int>(nvm->GlobalEdges.size())) {
                nvm->GlobalEdges.erase(nvm->GlobalEdges.begin() + m_edgeIdx);
            }
            nvm->GlobalEdgeCount = static_cast<uint32_t>(nvm->GlobalEdges.size());
        } else {
            // Apply new edge properties
            if (m_edgeIdx >= 0 && m_edgeIdx < static_cast<int>(nvm->GlobalEdges.size())) {
                nvm->GlobalEdges[m_edgeIdx] = m_newGlobalEdge;
            }
        }
    } else {
        if (m_isDelete) {
            // Delete edge
            if (m_edgeIdx >= 0 && m_edgeIdx < static_cast<int>(nvm->InternalEdges.size())) {
                nvm->InternalEdges.erase(nvm->InternalEdges.begin() + m_edgeIdx);
            }
            nvm->InternalEdgeCount = static_cast<uint32_t>(nvm->InternalEdges.size());
        } else {
            // Apply new edge
            if (m_edgeIdx >= 0 && m_edgeIdx < static_cast<int>(nvm->InternalEdges.size())) {
                nvm->InternalEdges[m_edgeIdx] = m_newInternalEdge;
            }
        }
    }

    regionManager->GetRenderManager()->GetNvmRenderer()->RebuildNavmeshBuffers(nvm, m_rx, m_ry);
}
