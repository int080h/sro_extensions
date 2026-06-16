#pragma once
#include "formats/MapProject.h"
#include "io/ParseResult.h"
#include <string>

namespace sro {

class MapProjectParser {
public:
    static ParseResult Read(const std::wstring& path, formats::MapProject& out);
};

} // namespace sro
