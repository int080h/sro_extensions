#pragma once
#include "formats/MapFormats.h"
#include "io/BinaryReader.h"
#include "io/ParseResult.h"
#include <string>

namespace sro {

class MapPlacementParser {
public:
    static ParseResult Read(const std::wstring& path, formats::MapPlacements& out);
    static ParseResult Write(const std::wstring& path, const formats::MapPlacements& placements);

private:
    static ParseResult ReadLegacy(BinaryReader& reader, formats::MapPlacements& out, const std::wstring& path);
    static ParseResult ReadLod(BinaryReader& reader, formats::MapPlacements& out, const std::wstring& path);
    static bool ReadObject(BinaryReader& reader, formats::MapObject& obj, bool hasRegionId);
};

} // namespace sro
