#include "NavMeshParser.h"
#include "io/BinaryReader.h"

namespace sro {

ParseResult NavMeshParser::Read(const std::wstring& path, formats::NavMesh& out) {
    BinaryReader reader(path);
    if (!reader.IsOpen()) return ParseResult::Fail("Cannot open file", path);

    if (!reader.ReadSignature(out.Signature)) return ParseResult::Fail("Failed to read signature", path);
    if (out.Signature.rfind("JMXVNVM", 0) != 0) return ParseResult::Fail("Invalid JMXVNVM signature", path);

    int16_t objCount = 0;
    if (!reader.Read(objCount)) return ParseResult::Fail("Truncated object count", path);
    out.Objects.resize(objCount);

    for (int i = 0; i < objCount; ++i) {
        auto& obj = out.Objects[i];
        if (!reader.Read(obj.ResID)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.PosX)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.PosY)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.PosZ)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.IsStatic)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.Yaw)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.UID)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.UnkShort)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.IsBig)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.IsStruct)) return ParseResult::Fail("Truncated object", path);
        if (!reader.Read(obj.RegionID)) return ParseResult::Fail("Truncated object", path);

        uint16_t edgeCount = 0;
        if (!reader.Read(edgeCount)) return ParseResult::Fail("Truncated edges", path);
        obj.Edges.resize(edgeCount);
        for (uint16_t j = 0; j < edgeCount; ++j) {
            if (!reader.Read(obj.Edges[j].LinkedObjID)) return ParseResult::Fail("Truncated edge", path);
            if (!reader.Read(obj.Edges[j].LinkedObjEdgeID)) return ParseResult::Fail("Truncated edge", path);
            if (!reader.Read(obj.Edges[j].EdgeID)) return ParseResult::Fail("Truncated edge", path);
        }
    }

    if (!reader.Read(out.TotalCellCount)) return ParseResult::Fail("Truncated cells", path);
    if (!reader.Read(out.OpenCellCount)) return ParseResult::Fail("Truncated cells", path);
    out.Cells.resize(out.TotalCellCount);

    for (uint32_t i = 0; i < out.TotalCellCount; ++i) {
        auto& cell = out.Cells[i];
        if (!reader.Read(cell.MinX)) return ParseResult::Fail("Truncated cell", path);
        if (!reader.Read(cell.MinY)) return ParseResult::Fail("Truncated cell", path);
        if (!reader.Read(cell.MaxX)) return ParseResult::Fail("Truncated cell", path);
        if (!reader.Read(cell.MaxY)) return ParseResult::Fail("Truncated cell", path);

        uint8_t cellObjCount = 0;
        if (!reader.Read(cellObjCount)) return ParseResult::Fail("Truncated cell objects", path);
        cell.ObjIndices.resize(cellObjCount);
        for (uint8_t j = 0; j < cellObjCount; ++j) {
            if (!reader.Read(cell.ObjIndices[j])) return ParseResult::Fail("Truncated cell object", path);
        }
    }

    if (!reader.Read(out.GlobalEdgeCount)) return ParseResult::Fail("Truncated global edges", path);
    out.GlobalEdges.resize(out.GlobalEdgeCount);
    for (uint32_t i = 0; i < out.GlobalEdgeCount; ++i) {
        auto& edge = out.GlobalEdges[i];
        if (!reader.Read(edge.MinX)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.MinY)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.MaxX)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.MaxY)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.Flags)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.D0)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.D1)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.C0)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.C1)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.R0)) return ParseResult::Fail("Truncated global edge", path);
        if (!reader.Read(edge.R1)) return ParseResult::Fail("Truncated global edge", path);
    }

    if (!reader.Read(out.InternalEdgeCount)) return ParseResult::Fail("Truncated internal edges", path);
    out.InternalEdges.resize(out.InternalEdgeCount);
    for (uint32_t i = 0; i < out.InternalEdgeCount; ++i) {
        auto& edge = out.InternalEdges[i];
        if (!reader.Read(edge.MinX)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.MinY)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.MaxX)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.MaxY)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.Flags)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.D0)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.D1)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.C0)) return ParseResult::Fail("Truncated internal edge", path);
        if (!reader.Read(edge.C1)) return ParseResult::Fail("Truncated internal edge", path);
    }

    out.TileMap.resize(96 * 96);
    for (int i = 0; i < 96 * 96; ++i) {
        if (!reader.Read(out.TileMap[i].CellID)) return ParseResult::Fail("Truncated tile map", path);
        if (!reader.Read(out.TileMap[i].Flags)) return ParseResult::Fail("Truncated tile map", path);
        if (!reader.Read(out.TileMap[i].TextureID)) return ParseResult::Fail("Truncated tile map", path);
    }

    size_t remaining = reader.Remaining();
    if (remaining < 180) return ParseResult::Fail("Truncated height/plane data", path);

    size_t heightMapBytes = remaining - 180;
    size_t heightMapFloats = heightMapBytes / 4;
    out.HeightMap.resize(heightMapFloats);
    for (size_t i = 0; i < heightMapFloats; ++i) {
        if (!reader.Read(out.HeightMap[i])) return ParseResult::Fail("Truncated height map", path);
    }

    out.PlaneTypeMap.resize(36);
    if (!reader.ReadBytes(out.PlaneTypeMap.data(), 36)) return ParseResult::Fail("Truncated plane types", path);

    out.PlaneHeightMap.resize(36);
    for (int i = 0; i < 36; ++i) {
        if (!reader.Read(out.PlaneHeightMap[i])) return ParseResult::Fail("Truncated plane heights", path);
    }

    return ParseResult::Success(path);
}

ParseResult NavMeshParser::Write(const std::wstring& path, const formats::NavMesh& mesh) {
    BinaryWriter writer(path);
    if (!writer.IsOpen()) return ParseResult::Fail("Cannot create file", path);
    if (!writer.WriteSignature(mesh.Signature)) return ParseResult::Fail("Write signature failed", path);

    int16_t objCount = static_cast<int16_t>(mesh.Objects.size());
    if (!writer.Write(objCount)) return ParseResult::Fail("Write failed", path);

    for (const auto& obj : mesh.Objects) {
        if (!writer.Write(obj.ResID)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.PosX)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.PosY)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.PosZ)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.IsStatic)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.Yaw)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.UID)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.UnkShort)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.IsBig)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.IsStruct)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(obj.RegionID)) return ParseResult::Fail("Write failed", path);

        uint16_t edgeCount = static_cast<uint16_t>(obj.Edges.size());
        if (!writer.Write(edgeCount)) return ParseResult::Fail("Write failed", path);
        for (const auto& edge : obj.Edges) {
            if (!writer.Write(edge.LinkedObjID)) return ParseResult::Fail("Write failed", path);
            if (!writer.Write(edge.LinkedObjEdgeID)) return ParseResult::Fail("Write failed", path);
            if (!writer.Write(edge.EdgeID)) return ParseResult::Fail("Write failed", path);
        }
    }

    if (!writer.Write(mesh.TotalCellCount)) return ParseResult::Fail("Write failed", path);
    if (!writer.Write(mesh.OpenCellCount)) return ParseResult::Fail("Write failed", path);

    for (const auto& cell : mesh.Cells) {
        if (!writer.Write(cell.MinX)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(cell.MinY)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(cell.MaxX)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(cell.MaxY)) return ParseResult::Fail("Write failed", path);
        uint8_t cellObjCount = static_cast<uint8_t>(cell.ObjIndices.size());
        if (!writer.Write(cellObjCount)) return ParseResult::Fail("Write failed", path);
        for (uint16_t idx : cell.ObjIndices) {
            if (!writer.Write(idx)) return ParseResult::Fail("Write failed", path);
        }
    }

    if (!writer.Write(mesh.GlobalEdgeCount)) return ParseResult::Fail("Write failed", path);
    for (const auto& edge : mesh.GlobalEdges) {
        if (!writer.Write(edge.MinX)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.MinY)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.MaxX)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.MaxY)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.Flags)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.D0)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.D1)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.C0)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.C1)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.R0)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.R1)) return ParseResult::Fail("Write failed", path);
    }

    if (!writer.Write(mesh.InternalEdgeCount)) return ParseResult::Fail("Write failed", path);
    for (const auto& edge : mesh.InternalEdges) {
        if (!writer.Write(edge.MinX)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.MinY)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.MaxX)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.MaxY)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.Flags)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.D0)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.D1)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.C0)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(edge.C1)) return ParseResult::Fail("Write failed", path);
    }

    for (const auto& tile : mesh.TileMap) {
        if (!writer.Write(tile.CellID)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(tile.Flags)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(tile.TextureID)) return ParseResult::Fail("Write failed", path);
    }

    for (float h : mesh.HeightMap) {
        if (!writer.Write(h)) return ParseResult::Fail("Write failed", path);
    }

    if (!writer.WriteBytes(mesh.PlaneTypeMap.data(), mesh.PlaneTypeMap.size())) return ParseResult::Fail("Write failed", path);

    for (float ph : mesh.PlaneHeightMap) {
        if (!writer.Write(ph)) return ParseResult::Fail("Write failed", path);
    }

    return ParseResult::Success(path);
}

} // namespace sro
