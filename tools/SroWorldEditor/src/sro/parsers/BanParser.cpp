#include "BanParser.h"
#include "ParserHelpers.h"
#include <fstream>

bool BanResource::Read(const std::wstring& path) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return false;

    char sig[13] = {0};
    fs.read(sig, 12);
    Signature = sig;
    if (Signature.rfind("JMXVBAN", 0) != 0) return false;

    const bool is0102 = Signature.find("0102") != std::string::npos;
    if (is0102) {
        uint32_t int0 = 0, int1 = 0;
        ReadVal(fs, int0);
        ReadVal(fs, int1);
    }

    uint32_t nameLen = 0;
    ReadVal(fs, nameLen);
    if (!fs || nameLen > 256) return false;
    Name = ReadString(fs, nameLen);

    ReadVal(fs, DurationMs);
    ReadVal(fs, FramesPerSecond);
    ReadVal(fs, Type);

    uint32_t timeCount = 0;
    ReadVal(fs, timeCount);
    if (!fs || timeCount > 10000) return false;
    KeyframeTimesMs.resize(timeCount);
    for (uint32_t i = 0; i < timeCount; ++i) {
        ReadVal(fs, KeyframeTimesMs[i]);
    }

    uint32_t boneCount = 0;
    ReadVal(fs, boneCount);
    if (!fs || boneCount > 512) return false;

    BoneTracks.resize(boneCount);
    for (uint32_t b = 0; b < boneCount; ++b) {
        uint32_t boneNameLen = 0;
        ReadVal(fs, boneNameLen);
        if (!fs || boneNameLen > 128) return false;
        BoneTracks[b].BoneName = ReadString(fs, boneNameLen);

        uint32_t keyCount = 0;
        ReadVal(fs, keyCount);
        if (!fs || keyCount > 10000) return false;
        BoneTracks[b].Keyframes.resize(keyCount);
        for (uint32_t k = 0; k < keyCount; ++k) {
            ReadVal(fs, BoneTracks[b].Keyframes[k].Rotation.x);
            ReadVal(fs, BoneTracks[b].Keyframes[k].Rotation.y);
            ReadVal(fs, BoneTracks[b].Keyframes[k].Rotation.z);
            ReadVal(fs, BoneTracks[b].Keyframes[k].Rotation.w);
            ReadVal(fs, BoneTracks[b].Keyframes[k].Translation.x);
            ReadVal(fs, BoneTracks[b].Keyframes[k].Translation.y);
            ReadVal(fs, BoneTracks[b].Keyframes[k].Translation.z);
        }
    }

    return !BoneTracks.empty();
}
