#pragma once
#include "formats/MapFormats.h"
#include "io/ParseResult.h"
#include <string>

namespace sro {

class MapMeshParser {
public:
    static ParseResult Read(const std::wstring& path, formats::MapMesh& out);
    static ParseResult Write(const std::wstring& path, const formats::MapMesh& mesh);
};

} // namespace sro
