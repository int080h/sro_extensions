#include "BmtParser.h"
#include "ParserHelpers.h"
#include <fstream>

bool BmtParser::Read(const std::wstring& path) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return false;

    char sig[13] = {0};
    fs.read(sig, 12);
    Signature = sig;
    if (Signature.rfind("JMXVBMT", 0) != 0) return false;

    uint32_t count = 0;
    ReadVal(fs, count);
    if (!fs) return false;
    if (count > 100000) return false;
    Entries.resize(count);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t nameLen = 0;
        ReadVal(fs, nameLen);
        if (!fs) return false;
        Entries[i].Name = ReadString(fs, nameLen);

        // Skip 17 floats (68 bytes) of material properties
        fs.seekg(68, std::ios::cur);

        // Read unk byte
        fs.seekg(1, std::ios::cur);

        ReadVal(fs, Entries[i].HasTexture);

        // Read unkShort
        fs.seekg(2, std::ios::cur);

        uint32_t texNameLen = 0;
        ReadVal(fs, texNameLen);
        if (!fs) return false;
        if (texNameLen > 0) {
            Entries[i].TextureName = ReadString(fs, texNameLen);
        }

        // Skip unkFloat (4 bytes) and padding (3 bytes)
        fs.seekg(7, std::ios::cur);
        if (!fs) return false;
    }
    return true;
}
