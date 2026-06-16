#include "project/ProjectSerializer.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static json Vec3ToJson(const Vector3& v) {
    return json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

static Vector3 Vec3FromJson(const json& j) {
    return Vector3(j.value("x", 0.0f), j.value("y", 0.0f), j.value("z", 0.0f));
}

bool ProjectSerializer::Save(const Project& project, const std::string& path) {
    json j;
    j["projectName"] = project.projectName;
    j["clientPath"] = project.clientPath;
    j["serverConnectionString"] = project.serverConnectionString;
    j["outputPath"] = project.outputPath;
    j["regions"] = project.loadedRegions;
    j["settings"] = {
        {"cameraSpeed", project.settings.cameraSpeed},
        {"showGrid", project.settings.showGrid},
        {"showRegionBounds", project.settings.showRegionBounds}
    };
    j["camera"] = {
        {"position", Vec3ToJson(project.cameraPosition)},
        {"yaw", project.cameraYaw},
        {"pitch", project.cameraPitch}
    };
    j["sroRegionRx"] = project.sroRegionRx;
    j["sroRegionRy"] = project.sroRegionRy;

    std::ofstream out(path);
    if (!out) return false;
    out << j.dump(2);
    return true;
}

bool ProjectSerializer::Load(const std::string& path, Project& outProject) {
    std::ifstream in(path);
    if (!in) return false;
    json j;
    in >> j;

    outProject.projectName = j.value("projectName", "Untitled");
    outProject.clientPath = j.value("clientPath", "");
    outProject.serverConnectionString = j.value("serverConnectionString", "");
    outProject.outputPath = j.value("outputPath", "");
    outProject.loadedRegions = j.value("regions", std::vector<int>{});
    if (j.contains("settings")) {
        outProject.settings.cameraSpeed = j["settings"].value("cameraSpeed", 250.0f);
        outProject.settings.showGrid = j["settings"].value("showGrid", true);
        outProject.settings.showRegionBounds = j["settings"].value("showRegionBounds", true);
    }
    if (j.contains("camera")) {
        outProject.cameraPosition = Vec3FromJson(j["camera"]["position"]);
        outProject.cameraYaw = j["camera"].value("yaw", -90.0f);
        outProject.cameraPitch = j["camera"].value("pitch", -30.0f);
    }
    outProject.sroRegionRx = j.value("sroRegionRx", 97);
    outProject.sroRegionRy = j.value("sroRegionRy", 167);
    outProject.filePath = path;
    outProject.modified = false;
    return true;
}
