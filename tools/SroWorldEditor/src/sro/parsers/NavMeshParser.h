#pragma once
#include "formats/NavMeshFormat.h"
#include "io/ParseResult.h"
#include <string>

namespace sro {

class NavMeshParser {
public:
    static ParseResult Read(const std::wstring& path, formats::NavMesh& out);
    static ParseResult Write(const std::wstring& path, const formats::NavMesh& mesh);
};

} // namespace sro
