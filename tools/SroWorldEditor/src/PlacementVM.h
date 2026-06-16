#pragma once
#include "formats/MapFormats.h"
#include <string>

struct PlacementVM {
    sro::formats::MapObject Object;
    std::string BsrPath;
    int BlockZ = 0, BlockX = 0, Lod = 0, Index = 0;
    int LoadedRx = 0, LoadedRy = 0;
};
