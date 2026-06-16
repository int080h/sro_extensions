#pragma once
#include "validation/ValidationMessage.h"

class Validator {
public:
    static std::vector<ValidationMessage> RunSampleValidation();
    static std::vector<ValidationMessage> ValidateWorld(const class World& world);
};
