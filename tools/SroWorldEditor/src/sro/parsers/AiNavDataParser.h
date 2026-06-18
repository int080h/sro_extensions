#pragma once
#include "formats/AiNavDataFormat.h"
#include "io/ParseResult.h"
#include <string>

namespace sro {

class AiNavDataParser {
public:
    static ParseResult Read(const std::wstring& path, formats::AiNavData& out);
    static ParseResult Write(const std::wstring& path, const formats::AiNavData& data);
    static ParseResult ReadFromBuffer(const uint8_t* data, size_t size, formats::AiNavData& out);
    static std::vector<uint8_t> WriteToBuffer(const formats::AiNavData& data);
};

} // namespace sro
