#pragma once
#include "validation/ValidationMessage.h"
#include <vector>

struct PlacementVM;

class Validator {
public:
    static std::vector<ValidationMessage> RunSampleValidation();
    static std::vector<ValidationMessage> ValidateWorld(const class World& world);
    static std::vector<ValidationMessage> ValidateCollisions(const std::vector<PlacementVM>& placements, int regionId);
};
