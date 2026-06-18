#pragma once
#include <vector>
#include <optional>
#include "formats/NavMeshFormat.h"
#include "PlacementVM.h"
#include "parsers/BmsParser.h"

namespace sro::nav {

struct NavObjectBinding {
    int nvmObjectIndex = -1;
    const PlacementVM* placement = nullptr;
    const BmsNavMesh* bms = nullptr;
    std::vector<int> cellQuadIndices;
    bool foreignRegion = false;
    bool inNvmOnly = false;
    bool placementOnly = false;
};

struct NavMeshContext {
    std::vector<NavObjectBinding> bindings;
    const sro::formats::NavMesh* nvm = nullptr;

    const NavObjectBinding* FindByLocalUid(int16_t localUid) const;
    const NavObjectBinding* FindByNvmIndex(int idx) const;
    const NavObjectBinding* FindByPlacementUid(int16_t uid) const;
};

NavMeshContext BuildNavMeshContext(const sro::formats::NavMesh& nvm,
                                   const std::vector<PlacementVM>& placements,
                                   int centerRx, int centerRy);

} // namespace sro::nav
