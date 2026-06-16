#pragma once
#include <string>
#include <vector>

enum class ValidationSeverity {
    Error,
    Warning,
    Info
};

struct ValidationMessage {
    ValidationSeverity severity = ValidationSeverity::Info;
    std::string message;
    int regionId = 0;
    std::string objectId;
};
