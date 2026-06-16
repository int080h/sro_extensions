#include "validation/Validator.h"
#include "world/Region.h"
#include <set>

std::vector<ValidationMessage> Validator::RunSampleValidation() {
    return {
        {ValidationSeverity::Error, "Teleport destination is blocked.", 25000, "tp_25000"},
        {ValidationSeverity::Error, "Monster spawn is inside collision.", 25001, "spawn_25001"},
        {ValidationSeverity::Warning, "Too many objects in one region.", 25000, ""},
        {ValidationSeverity::Warning, "NPC has missing shop link.", 25002, "npc_25002"},
        {ValidationSeverity::Error, "Object has missing model file.", 25001, "obj_gate_25001"},
        {ValidationSeverity::Error, "Duplicate unique ID found.", 25000, "obj_gate_25000"},
    };
}

std::vector<ValidationMessage> Validator::ValidateWorld(const World& world) {
    std::vector<ValidationMessage> msgs;
    std::set<std::string> ids;

    for (const auto& region : world.regions) {
        for (const auto& obj : region.objects) {
            if (ids.count(obj.id)) {
                msgs.push_back({ValidationSeverity::Error, "Duplicate unique ID: " + obj.id, region.id, obj.id});
            }
            ids.insert(obj.id);
            if (obj.modelPath.empty()) {
                msgs.push_back({ValidationSeverity::Error, "Object has missing model file.", region.id, obj.id});
            }
        }
        for (const auto& npc : region.npcs) {
            if (npc.shopLink == 0 && npc.questLink == 0) {
                msgs.push_back({ValidationSeverity::Warning, "NPC has missing shop link.", region.id, npc.id});
            }
        }
        if (region.objects.size() > 100) {
            msgs.push_back({ValidationSeverity::Warning, "Too many objects in one region.", region.id, ""});
        }
    }
    return msgs;
}
