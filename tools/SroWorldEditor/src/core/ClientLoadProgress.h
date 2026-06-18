#pragma once
#include <string>

struct ClientLoadProgress {
    std::string stage;
    int step = 0;
    int totalSteps = 1;
    float fraction = 0.f;
    bool active = false;
    bool done = false;
    bool failed = false;
    std::string error;
};
