#pragma once
#include "project/Project.h"
#include <string>

class ProjectSerializer {
public:
    static bool Save(const Project& project, const std::string& path);
    static bool Load(const std::string& path, Project& outProject);
};
