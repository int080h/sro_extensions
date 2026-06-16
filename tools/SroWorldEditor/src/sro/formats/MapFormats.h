#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace sro::formats {

struct MapVertex {
    float Height = 0;
    uint16_t TextureData = 0;
    uint8_t Brightness = 0;
};

struct MapBlock {
    uint32_t Flag = 0;
    uint16_t EnvironmentID = 0;
    MapVertex Vertices[17 * 17]{};
    int8_t WaterType = 0;
    uint8_t WaterWaveType = 0;
    float WaterHeight = 0;
    uint16_t Tiles[16 * 16]{};
    float HeightMax = 0;
    float HeightMin = 0;
    uint8_t Reserved[20]{};
};

struct MapMesh {
    std::string Signature = "JMXVMAPM1000";
    MapBlock Blocks[36]{};
};

struct MapObject {
    uint32_t ObjID = 0;
    float PosX = 0, PosY = 0, PosZ = 0;
    int16_t IsStatic = 0;
    float Yaw = 0;
    int16_t UID = 0;
    int16_t Short0 = 0;
    uint8_t IsBig = 0;
    uint8_t IsStruct = 0;
    uint16_t RegionID = 0;
};

enum class PlacementFormat { LegacyO, LodO2 };

struct MapPlacements {
    std::string Signature = "JMXVMAPO1001";
    PlacementFormat Format = PlacementFormat::LodO2;
    std::vector<MapObject> Blocks[6][6][4];

    int LodCount() const {
        return Format == PlacementFormat::LegacyO ? 1 : 4;
    }
};

} // namespace sro::formats
