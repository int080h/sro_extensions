#pragma once
#include "formats/NavMeshFormat.h"
#include "parsers/BmsParser.h"
#include <string>
#include <vector>

namespace sro::nav {

struct FieldConsumer {
    const char* section;
    const char* field;
    const char* consumer; // viz | passability | validation | runtime-only
};

// Checklist of JMXVNVM / JMXVBMS fields and their editor consumers.
std::vector<FieldConsumer> NavMeshFieldConsumers();
std::vector<std::string> AuditNavMeshUsage(const sro::formats::NavMesh& nav);
std::vector<std::string> AuditBmsNavUsage(const BmsNavMesh& bms, uint32_t navFlag);

} // namespace sro::nav
