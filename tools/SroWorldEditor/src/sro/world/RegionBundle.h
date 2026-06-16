#pragma once
#include "core/RegionCoord.h"
#include "formats/MapFormats.h"
#include "formats/NavMeshFormat.h"
#include <string>
#include <memory>
#include <optional>

namespace sro {

enum class RegionLoadState {
    NotAttempted,
    Missing,
    Failed,
    Loaded
};

struct RegionBundle {
    RegionCoord coord;
    RegionLoadState terrainState = RegionLoadState::NotAttempted;
    RegionLoadState placementState = RegionLoadState::NotAttempted;
    RegionLoadState navState = RegionLoadState::NotAttempted;

    std::optional<formats::MapMesh> terrain;
    std::optional<formats::MapPlacements> placements;
    std::optional<formats::NavMesh> navMesh;

    std::wstring placementSourcePath;
    std::wstring terrainSourcePath;
    std::wstring navSourcePath;

    std::string terrainError;
    std::string placementError;
    std::string navError;
};

} // namespace sro
