#pragma once
#include "Core/Math.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

struct BanKeyframe {
    Vector4 Rotation{};
    Vector3 Translation{};
};

struct BanBoneTrack {
    std::string BoneName;
    std::vector<BanKeyframe> Keyframes;
};

class BanResource {
public:
    std::string Signature;
    std::string Name;
    uint32_t DurationMs = 0;
    uint32_t FramesPerSecond = 30;
    uint32_t Type = 0;
    std::vector<uint32_t> KeyframeTimesMs;
    std::vector<BanBoneTrack> BoneTracks;

    bool Read(const std::wstring& path);
    float DurationSeconds() const { return DurationMs / 1000.0f; }
};
