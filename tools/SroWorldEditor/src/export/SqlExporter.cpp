#include "export/SqlExporter.h"
#include "world/Region.h"
#include "project/Project.h"
#include <sstream>
#include <fstream>
#include <filesystem>

std::string SqlExporter::ExportSample(const World& world) {
    std::ostringstream sql;
    sql << "-- Silkroad Online World Editor SQL Export\n";
    sql << "-- Generated placeholder script\n\n";
    for (const auto& region : world.regions) {
        for (const auto& sp : region.spawns) {
            sql << "INSERT INTO _RefSpawn (CodeName, RegionID, PosX, PosY, PosZ, Count) VALUES ('"
                << sp.monsterCodeName << "', " << sp.regionId << ", "
                << sp.position.x << ", " << sp.position.y << ", " << sp.position.z << ", "
                << sp.count << ");\n";
        }
        for (const auto& npc : region.npcs) {
            sql << "INSERT INTO _RefNpc (CodeName, RegionID, PosX, PosY, PosZ) VALUES ('"
                << npc.codeName << "', " << npc.regionId << ", "
                << npc.transform.position.x << ", " << npc.transform.position.y << ", "
                << npc.transform.position.z << ");\n";
        }
    }
    return sql.str();
}

bool ClientExporter::ExportPatchFolder(const Project& project, const World& world, const std::string& outputPath) {
    namespace fs = std::filesystem;
    try {
        fs::create_directories(outputPath + "/Map");
        fs::create_directories(outputPath + "/Data/navmesh");
        for (const auto& region : world.regions) {
            if (region.modified) {
                std::string note = outputPath + "/Map/region_" + std::to_string(region.id) + "_export.txt";
                std::ofstream out(note);
                out << "Placeholder export for region " << region.id << " from project " << project.projectName << "\n";
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}
