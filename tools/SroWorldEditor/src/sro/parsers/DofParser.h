#pragma once
#include "formats/DofFormat.h"
#include "io/ParseResult.h"
#include <string>

namespace sro {

class DofParser {
public:
    static ParseResult Read(const std::wstring& path, formats::DofDungeon& out);
    static ParseResult Write(const std::wstring& path, const formats::DofDungeon& data);
};

} // namespace sro
