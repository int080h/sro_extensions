#pragma once
#include <string>

class SqlExporter {
public:
    static std::string ExportSample(const class World& world);
};

class ClientExporter {
public:
    static bool ExportPatchFolder(const class Project& project, const class World& world, const std::string& outputPath);
};
