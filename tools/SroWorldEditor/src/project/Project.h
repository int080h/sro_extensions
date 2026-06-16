#pragma once
#include <string>
#include <vector>
#include "core/MathTypes.h"

struct ProjectSettings {
    float cameraSpeed = 250.0f;
    bool showGrid = true;
    bool showRegionBounds = true;
};

struct Project {
    std::string projectName = "Untitled";
    std::string clientPath;
    std::string serverConnectionString;
    std::string outputPath;
    std::vector<int> loadedRegions;
    ProjectSettings settings;
    int sroRegionRx = 97;
    int sroRegionRy = 167;
    Vector3 cameraPosition{960.0f, 150.0f, 960.0f};
    float cameraYaw = -90.0f;
    float cameraPitch = -30.0f;
    bool modified = false;
    std::string filePath;
};
