#include "core/RegionId.h"
#include "core/SceneSpace.h"
#include "formats/MapFormats.h"
#include "formats/MapProject.h"
#include "parsers/MapPlacementParser.h"
#include "parsers/MapProjectParser.h"
#include "io/BinaryReader.h"
#include "sro/WorldMapCoords.h"
#include "sro_map/MinimapTileIndex.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

static int g_failures = 0;

#define TEST(cond) do { if (!(cond)) { std::cerr << "FAIL: " #cond " at " << __LINE__ << "\n"; ++g_failures; } } while(0)

static void TestRegionIdRoundTrip() {
    sro::RegionId id = sro::RegionId::FromRxRy(97, 167);
    TEST(id.Rx() == 97);
    TEST(id.Ry() == 167);
    auto c = id.ToCoord();
    TEST(c.rx == 97 && c.ry == 167);
}

static void TestSceneSpaceOffsets() {
    float ox = sro::SceneSpace::ObjectOffsetX(167, 167);
    float oz = sro::SceneSpace::ObjectOffsetZ(97, 97);
    TEST(ox == 0.0f);
    TEST(oz == 0.0f);
    float ox2 = sro::SceneSpace::ObjectOffsetX(168, 167);
    TEST(ox2 == 1920.0f);
}

static void WriteSyntheticLegacyO(const std::wstring& path) {
    sro::BinaryWriter w(path);
    w.WriteSignature("JMXVMAPO1001");
    for (int z = 0; z < 6; ++z) {
        for (int x = 0; x < 6; ++x) {
            uint16_t count = 0;
            w.Write(count);
        }
    }
}

static void WriteSyntheticO2(const std::wstring& path) {
    sro::BinaryWriter w(path);
    w.WriteSignature("JMXVMAPO1001");
    for (int z = 0; z < 6; ++z) {
        for (int x = 0; x < 6; ++x) {
            for (int lod = 0; lod < 4; ++lod) {
                uint16_t count = 0;
                w.Write(count);
            }
        }
    }
}

static void TestPlacementParserFormats() {
    const std::wstring legacyPath = L"test_legacy.o";
    const std::wstring o2Path = L"test_lod.o2";
    WriteSyntheticLegacyO(legacyPath);
    WriteSyntheticO2(o2Path);

    sro::formats::MapPlacements legacy;
    auto r1 = sro::MapPlacementParser::Read(legacyPath, legacy);
    TEST(r1.ok);
    TEST(legacy.Format == sro::formats::PlacementFormat::LegacyO);

    sro::formats::MapPlacements lod;
    auto r2 = sro::MapPlacementParser::Read(o2Path, lod);
    TEST(r2.ok);
    TEST(lod.Format == sro::formats::PlacementFormat::LodO2);

    std::filesystem::remove(legacyPath);
    std::filesystem::remove(o2Path);
}

static bool Near(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) <= eps;
}

static void TestCameraHeadingToMapIconRadians() {
    constexpr float kHalfPi = 3.14159265f / 2.0f;

    const float north = sro::map::CameraHeadingToMapIconRadians(0.0f, -1.0f);
    TEST(Near(north, kHalfPi));

    const float east = sro::map::CameraHeadingToMapIconRadians(1.0f, 0.0f);
    TEST(Near(east, 0.0f));

    const float south = sro::map::CameraHeadingToMapIconRadians(0.0f, 1.0f);
    TEST(Near(south, -kHalfPi));

    const float west = sro::map::CameraHeadingToMapIconRadians(-1.0f, 0.0f);
    TEST(Near(west, 3.14159265f) || Near(west, -3.14159265f));
}

static void TestWorldMapCoordsRoundTrip() {
    const int rx = 97;
    const int ry = 167;
    const float localX = 960.0f;
    const float localZ = 960.0f;

    sro::map::SimpleWorldPos simple = sro::map::SceneLocalToSimpleWorld(rx, ry, localX, localZ);
    TEST(simple.x == 192.0f * static_cast<float>(rx - sro::map::kOriginRx) + localX / 10.0f);
    TEST(simple.z == 192.0f * static_cast<float>(ry - sro::map::kOriginRy) + localZ / 10.0f);

    int displayMx = 0;
    int displayMy = 0;
    sro::map::RegionToDisplay(rx, ry, displayMx, displayMy);
    TEST(displayMx == ry);
    TEST(displayMy == rx);

    int roundRx = 0;
    int roundRy = 0;
    sro::map::DisplayToRegion(displayMx, displayMy, roundRx, roundRy);
    TEST(roundRx == rx);
    TEST(roundRy == ry);

    sro::map::MapPixelPos pixel = sro::map::RegionLocalToMapPixel(rx, ry, localX, localZ, 46, 113);
    TEST(pixel.x == sro::map::MapCellToWorldX(ry, 46) + 16.0f);
    TEST(pixel.y == sro::map::MapCellToWorldY(rx, 113) + 16.0f);
}

static void TestWorldMapSouthGateOutside() {
    constexpr int minMx = 46;
    constexpr int maxMy = 113;
    constexpr int rx = 96;
    constexpr int ry = 168;
    const float localX = 941.0f;
    const float localZ = 200.0f;

    const sro::map::MapPixelPos gateOutside =
        sro::map::RegionLocalToMapPixel(rx, ry, localX, localZ, minMx, maxMy);
    const sro::map::MapPixelPos centerCell =
        sro::map::RegionLocalToMapPixel(rx, ry, 960.0f, 960.0f, minMx, maxMy);
    const sro::map::MapPixelPos cityCenter =
        sro::map::RegionLocalToMapPixel(97, 168, 960.0f, 960.0f, minMx, maxMy);
    const sro::map::MapPixelPos northSide =
        sro::map::RegionLocalToMapPixel(rx, ry, 960.0f, 1600.0f, minMx, maxMy);
    const sro::map::MapPixelPos southSide =
        sro::map::RegionLocalToMapPixel(rx, ry, 960.0f, 400.0f, minMx, maxMy);

    // South-gate exterior (low localZ) must be south of center-of-cell placement.
    TEST(gateOutside.y > centerCell.y);
    TEST(southSide.y > northSide.y);
    TEST(gateOutside.y > cityCenter.y - 48.0f);
}

static void TestLocalMapTexturePixel() {
    constexpr int minMx = 166;
    constexpr int maxMx = 170;
    constexpr int minMy = 96;
    constexpr int maxMy = 99;
    constexpr int renderWidth = 1024;
    constexpr int renderHeight = 768;
    constexpr int textureWidth = 1024;
    constexpr int textureHeight = 1024;

    const sro::map::LocalMapViewState view = sro::map::LocalMapViewStateFromDef(
        166, 99, 170, 96, minMx, maxMx, minMy, maxMy, renderWidth, renderHeight, renderWidth,
        renderHeight, textureWidth, textureHeight);
    TEST(view.renderWidth == 1024.0f);
    TEST(view.renderHeight == 768.0f);
    TEST(view.worldMinX == 192.0f * 31.0f);
    TEST(view.worldMaxX == 192.0f * 36.0f);
    TEST(view.worldZSouth == 192.0f * 4.0f);
    TEST(view.worldZNorth == 192.0f * 8.0f);
    TEST(view.scaleX == 960.0f / 1024.0f);
    TEST(view.scaleY == 768.0f / 768.0f);
    TEST(view.uv.uvMinV == 0.0f);
    TEST(view.uv.uvMaxV == 0.75f);

    const sro::map::TexturePixelPos center =
        sro::map::LocalPoiToRenderPixel(-1, -1, 512.0f, 384.0f, 0.0f, 0.0f, view);
    TEST(center.x == 512.0f);
    TEST(center.y == 384.0f);

    const sro::map::TexturePixelPos southPoi =
        sro::map::LocalPoiToRenderPixel(96, 168, 100.0f, 700.0f, 8.0f, 8.0f, view);
    TEST(southPoi.x == 108.0f);
    TEST(southPoi.y == 708.0f);

    const sro::map::TexturePixelPos gatePlayer =
        sro::map::RegionLocalToRenderPixel(96, 168, 960.0f, 200.0f, view);
    TEST(gatePlayer.y > 650.0f);
    TEST(gatePlayer.y < 760.0f);
    TEST(std::fabs(gatePlayer.y - southPoi.y) < 60.0f);

    const sro::map::TexturePixelPos northRowPlayer =
        sro::map::RegionLocalToRenderPixel(97, 168, 960.0f, 200.0f, view);
    TEST(northRowPlayer.y < southPoi.y);

    const int rx = 97;
    const int ry = 168;
    const float localX = 960.0f;
    const float localZ = 1600.0f;

    const sro::map::TexturePixelPos player =
        sro::map::RegionLocalToRenderPixel(rx, ry, localX, localZ, view);
    TEST(player.x > 400.0f);
    TEST(player.x < 600.0f);
    TEST(player.y > 300.0f);
    TEST(player.y < 550.0f);
    TEST(player.y < southPoi.y);

    const sro::map::TexturePixelPos norm =
        sro::map::RenderPixelToNormalized(player, view.renderWidth, view.renderHeight);
    TEST(norm.y > 0.45f);
    TEST(norm.y < 0.75f);

    const sro::map::TexturePixelPos northPoi =
        sro::map::LocalPoiToRenderPixel(99, 167, 500.0f, 80.0f, 8.0f, 8.0f, view);
    TEST(northPoi.y < southPoi.y);
}

static void TestLocalMapWorldRectAndPoiFilter() {
    constexpr int minMx = 166;
    constexpr int maxMx = 170;
    constexpr int minMy = 96;
    constexpr int maxMy = 99;
    constexpr int renderWidth = 1024;
    constexpr int renderHeight = 768;

    const sro::map::LocalMapWorldRect rect =
        sro::map::LocalMapWorldRectFromCellBounds(minMx, maxMx, renderWidth, renderHeight, 0.0f, 0.0f);
    TEST(rect.w == 160.0f);
    TEST(rect.h == 120.0f);
    TEST(rect.h / rect.w == 768.0f / 1024.0f);

    TEST(sro::map::ShouldDrawInnerCityPoi(1, 1));
    TEST(!sro::map::ShouldDrawInnerCityPoi(0, 1));
    TEST(!sro::map::ShouldDrawInnerCityPoi(2, 1));

    TEST(sro::map::ShouldShowInnerCityPermanentLabel(1));
    TEST(!sro::map::ShouldShowInnerCityPermanentLabel(0));
    TEST(!sro::map::ShouldShowInnerCityPermanentLabel(2));

    TEST(sro::map::ShouldDrawInnerCityPermanentLabel(1, "Blacksmith", "Jangan"));
    TEST(!sro::map::ShouldDrawInnerCityPermanentLabel(1, "Jangan", "Jangan"));
    TEST(!sro::map::ShouldDrawInnerCityPermanentLabel(1, "", "Jangan"));
    TEST(sro::map::IsRedundantInnerCityLabel("Jangan", "Jangan"));
    TEST(!sro::map::IsRedundantInnerCityLabel("Blacksmith", "Jangan"));
}

static void TestLocalMapGridRect() {
    constexpr int minMx = 166;
    constexpr int maxMx = 170;
    constexpr int minMy = 96;
    constexpr int maxMy = 99;

    const sro::map::LocalMapWorldRect grid =
        sro::map::LocalMapWorldRectGridAligned(minMx, maxMx, minMy, maxMy, 0.0f, 0.0f);
    TEST(grid.w == 160.0f);
    TEST(grid.h == 128.0f);
}

static void TestLocalPoiTextureSpaceAndFallback() {
    const sro::map::LocalMapViewState view = sro::map::LocalMapViewStateFromDef(
        166, 99, 170, 96, 166, 170, 96, 99, 1024, 768, 1024, 768, 1024, 1024);

    const sro::map::TexturePixelPos southPoi =
        sro::map::LocalPoiToRenderPixel(96, 168, 100.0f, 700.0f, 8.0f, 8.0f, view);
    const sro::map::TexturePixelPos texSpacePoi =
        sro::map::LocalPoiToRenderPixel(97, 167, 100.0f, 900.0f, 8.0f, 8.0f, view);
    TEST(texSpacePoi.y > 600.0f);
    TEST(texSpacePoi.y < 700.0f);
    TEST(texSpacePoi.y < southPoi.y + 50.0f);

    const sro::map::TexturePixelPos fallbackPoi =
        sro::map::LocalPoiToRenderPixel(97, 167, 2000.0f, 2000.0f, 8.0f, 8.0f, view);
    TEST(fallbackPoi.y >= 0.0f);
    TEST(fallbackPoi.y <= view.renderHeight);
}

static void TestHotanLocalMapCoordSpace() {
    // Id=3 from wlocalmap.txt: tex 512x1024, render 512x838, coord 640x1047, regions 134-136 x 91-95
    constexpr int minMx = 134;
    constexpr int maxMx = 136;
    constexpr int minMy = 91;
    constexpr int maxMy = 95;
    constexpr int coordWidth = 640;
    constexpr int coordHeight = 1047;
    constexpr int renderWidth = 512;
    constexpr int renderHeight = 838;
    constexpr int textureWidth = 512;
    constexpr int textureHeight = 1024;

    const sro::map::LocalMapViewState view = sro::map::LocalMapViewStateFromDef(
        134, 95, 136, 91, minMx, maxMx, minMy, maxMy, coordWidth, coordHeight, renderWidth,
        renderHeight, textureWidth, textureHeight);

    TEST(view.coordWidth == 640.0f);
    TEST(view.coordHeight == 1047.0f);
    TEST(view.renderWidth == 512.0f);
    TEST(view.renderHeight == 838.0f);
    TEST(view.uv.uvMinU == 0.0f);
    TEST(view.uv.uvMaxU == 1.0f);
    TEST(view.uv.uvMinV == 0.0f);
    TEST(Near(view.uv.uvMaxV, 838.0f / 1024.0f));

    const sro::map::LocalMapWorldRect rect =
        sro::map::LocalMapWorldRectFromCellBounds(minMx, maxMx, renderWidth, renderHeight, 0.0f,
                                                 0.0f);
    TEST(rect.w == 96.0f);
    TEST(Near(rect.h, 96.0f * 838.0f / 512.0f));

    const sro::map::TexturePixelPos corner =
        sro::map::LocalPoiToRenderPixel(-1, -1, static_cast<float>(coordWidth),
                                        static_cast<float>(coordHeight), 0.0f, 0.0f, view);
    TEST(Near(corner.x, static_cast<float>(renderWidth)));
    TEST(Near(corner.y, static_cast<float>(renderHeight)));

    const sro::map::TexturePixelPos northPoi =
        sro::map::LocalPoiToRenderPixel(95, 134, 320.0f, 100.0f, 8.0f, 8.0f, view);
    const sro::map::TexturePixelPos southPoi =
        sro::map::LocalPoiToRenderPixel(91, 136, 320.0f, 900.0f, 8.0f, 8.0f, view);
    TEST(northPoi.y < southPoi.y);
    TEST(southPoi.x > 0.0f && southPoi.x <= renderWidth);
    TEST(southPoi.y > 0.0f && southPoi.y <= renderHeight);
}

static void TestConstantinopleLocalMapUvAndPoi() {
    // Id=5: tex 1024x1024, render 890x1024, coord=render, regions 77-81 x 102-108
    constexpr int minMx = 77;
    constexpr int maxMx = 81;
    constexpr int minMy = 102;
    constexpr int maxMy = 108;
    constexpr int renderWidth = 890;
    constexpr int renderHeight = 1024;
    constexpr int textureWidth = 1024;
    constexpr int textureHeight = 1024;

    const sro::map::LocalMapViewState view = sro::map::LocalMapViewStateFromDef(
        77, 108, 81, 102, minMx, maxMx, minMy, maxMy, renderWidth, renderHeight, renderWidth,
        renderHeight, textureWidth, textureHeight);

    TEST(view.uv.uvMinU == 0.0f);
    TEST(Near(view.uv.uvMaxU, 890.0f / 1024.0f));
    TEST(view.uv.uvMinV == 0.0f);
    TEST(view.uv.uvMaxV == 1.0f);

    const sro::map::TexturePixelPos eastPoi =
        sro::map::LocalPoiToRenderPixel(102, 81, 833.0f, 400.0f, 8.0f, 8.0f, view);
    TEST(Near(eastPoi.x, 841.0f));

    const sro::map::TexturePixelPos northPoi =
        sro::map::LocalPoiToRenderPixel(108, 77, 445.0f, 80.0f, 8.0f, 8.0f, view);
    const sro::map::TexturePixelPos southPoi =
        sro::map::LocalPoiToRenderPixel(102, 81, 445.0f, 829.0f, 8.0f, 8.0f, view);
    TEST(northPoi.y < southPoi.y);
}

static void TestAlexandriaLocalMapUv() {
    // Id=12: tex 1024x1024, render 878x1024, coord=render, regions 46-51 x 89-96
    constexpr int minMx = 46;
    constexpr int maxMx = 51;
    constexpr int minMy = 89;
    constexpr int maxMy = 96;
    constexpr int renderWidth = 878;
    constexpr int renderHeight = 1024;
    constexpr int textureWidth = 1024;
    constexpr int textureHeight = 1024;

    const sro::map::LocalMapViewState view = sro::map::LocalMapViewStateFromDef(
        46, 96, 51, 89, minMx, maxMx, minMy, maxMy, renderWidth, renderHeight, renderWidth,
        renderHeight, textureWidth, textureHeight);

    TEST(view.uv.uvMinU == 0.0f);
    TEST(Near(view.uv.uvMaxU, 878.0f / 1024.0f));
    TEST(view.uv.uvMinV == 0.0f);
    TEST(view.uv.uvMaxV == 1.0f);

    const sro::map::TexturePixelPos corner =
        sro::map::LocalPoiToRenderPixel(-1, -1, static_cast<float>(renderWidth),
                                        static_cast<float>(renderHeight), 0.0f, 0.0f, view);
    TEST(Near(corner.x, static_cast<float>(renderWidth)));
    TEST(Near(corner.y, static_cast<float>(renderHeight)));
}

static bool IsMegaTileActive(const sro::formats::MapProject& project, int mx, int my) {
    for (int x = mx; x <= mx + 3; ++x) {
        for (int y = my - 3; y <= my; ++y) {
            if (project.ActiveRegions.count({y, x}) != 0) return true;
        }
    }
    return false;
}

static size_t CountActiveMegaTiles(const sro::formats::MapProject& project) {
    size_t count = 0;
    for (int mx = sro::map::kWorldTileMinMx; mx <= sro::map::kWorldTileMaxMx; mx += sro::map::kWorldTileStep) {
        for (int my = sro::map::kWorldTileStartMy; my >= sro::map::kWorldTileEndMy; my -= sro::map::kWorldTileStep) {
            if (IsMegaTileActive(project, mx, my)) ++count;
        }
    }
    return count;
}

static void TestDungeonFloorGrouping() {
    std::vector<sro::map::DungeonFloorGroupInput> rows = {
        {2001, "Dunhuang", "DH_A01", 1, 4}, {2002, "Dunhuang", "DH_A01", 2, 4},
        {2003, "Dunhuang", "DH_A01", 3, 4}, {2004, "Dunhuang", "DH_A01", 4, 4},
        {2005, "QinTomb", "QT_A01", 1, 6},   {2006, "QinTomb", "QT_A01", 2, 6},
        {2007, "QinTomb", "QT_A01", 3, 6},   {2008, "QinTomb", "QT_A01", 4, 6},
        {2009, "QinTomb", "QT_A01", 5, 6},   {2010, "QinTomb", "QT_A01", 6, 6},
    };

    const std::vector<int> qinFloors = sro::map::GroupDungeonFloorIds(rows, 2005);
    TEST(qinFloors.size() == 6u);
    TEST(qinFloors.front() == 2005);
    TEST(qinFloors.back() == 2010);

    const std::vector<int> dhFloors = sro::map::GroupDungeonFloorIds(rows, 2003);
    TEST(dhFloors.size() == 4u);
    TEST(dhFloors[2] == 2003);

    TEST(sro::map::GroupDungeonFloorIds(rows, 9999).empty());

    std::vector<sro::map::DungeonFloorGroupInput> mismatchedNames = {
        {2005, "name_a", "QT_A01", 1, 6}, {2006, "name_b", "QT_A01", 2, 6},
        {2007, "name_c", "QT_A01", 3, 6}, {2008, "name_d", "QT_A01", 4, 6},
        {2009, "name_e", "QT_A01", 5, 6}, {2010, "name_f", "QT_A01", 6, 6},
    };
    const std::vector<int> byKey = sro::map::GroupDungeonFloorIds(mismatchedNames, 2005);
    TEST(byKey.size() == 6u);
    TEST(byKey[1] == 2006);

    TEST(sro::map::DungeonGroupKeyFromFolder("QT_A01_FLOOR01") == "QT_A01");
}

static void TestDungeonFloorLabels() {
    TEST(sro::map::FormatFloorLabel('B', 1) == "B1");
    TEST(sro::map::FormatFloorLabel('B', 4) == "B4");
    TEST(sro::map::FormatFloorLabel('F', 2) == "F2");
    TEST(sro::map::FormatFloorLabel('\0', 1).empty());
}

static void TestDungeonFloorWorldRect() {
    const sro::map::LocalMapWorldRect rect =
        sro::map::DungeonFloorWorldRectFromRender(125, 132, 768, 1024);
    TEST(Near(rect.w, 768.0f));
    TEST(Near(rect.h, 1024.0f));
    TEST(Near(rect.x0, 0.0f));
    TEST(Near(rect.y0, 0.0f));
}

static void TestDungeonFloorLayout() {
    const sro::map::DungeonFloorLayout b1 =
        sro::map::DungeonFloorLayoutFromDef(768, 1024, 6, 8);
    TEST(Near(b1.tileScaleX, 1.0f));
    TEST(Near(b1.tileScaleY, 1.0f));
    TEST(Near(b1.tileScaleX, b1.tileScaleY));

    const sro::map::DungeonFloorLayout b2 =
        sro::map::DungeonFloorLayoutFromDef(1408, 640, 22, 10);
    TEST(Near(b2.tileScaleX, 0.5f));
    TEST(Near(b2.tileScaleY, 0.5f));
    TEST(Near(b2.tileScaleX, b2.tileScaleY));

    const sro::map::DungeonFloorLayout b3 =
        sro::map::DungeonFloorLayoutFromDef(768, 768, 24, 24);
    TEST(Near(b3.tileScaleX, 0.25f));
    TEST(Near(b3.tileScaleY, 0.25f));
    TEST(Near(b3.tileScaleX, b3.tileScaleY));

    const sro::map::DungeonFloorLayout fromBounds =
        sro::map::DungeonFloorLayoutFromDef(768, 1024, 0, 0, 125, 130, 125, 132);
    TEST(Near(fromBounds.tileScaleX, 1.0f));
    TEST(Near(fromBounds.tileScaleY, 1.0f));
}

static void TestMinimapHudCoords() {
    TEST(Near(sro::map::MinimapHudU(960.0f), 0.5f));
    TEST(Near(sro::map::MinimapHudV(960.0f), 0.5f));
    TEST(Near(sro::map::MinimapHudU(0.0f), 0.0f));
    TEST(Near(sro::map::MinimapHudV(1920.0f), 1.0f));
    TEST(Near(sro::map::MinimapHudVScreen(0.0f), 1.0f));
    TEST(Near(sro::map::MinimapHudVScreen(1920.0f), 0.0f));
    TEST(Near(sro::map::MinimapHudVScreen(960.0f), 0.5f));
}

static void TestMinimapTileFilename() {
    const std::wstring name = sro::map::FilenameForRegion(97, 167);
    TEST(name == L"167x97.ddj");
}

static void TestMinimapHudGridAlignment() {
    int col = 0;
    int row = 0;
    sro::map::MinimapHudCellToGrid(98, 172, 98, 172, col, row);
    TEST(col == 1);
    TEST(row == 1);

    sro::map::MinimapHudCellToGrid(98, 173, 98, 172, col, row);
    TEST(col == 2);
    TEST(row == 1);

    sro::map::MinimapHudCellToGrid(99, 172, 98, 172, col, row);
    TEST(col == 1);
    TEST(row == 0);

    int rx = 0;
    int ry = 0;
    sro::map::MinimapHudNeighborRegion(0, 0, 98, 172, rx, ry);
    TEST(rx == 98);
    TEST(ry == 172);
    sro::map::MinimapHudNeighborRegion(1, 1, 98, 172, rx, ry);
    TEST(rx == 99);
    TEST(ry == 173);
}

static void TestMapProjectActiveRegions() {
    const std::filesystem::path repoRoot =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    const std::filesystem::path mfoPath = repoRoot / "Client_Rigid" / "Map" / "mapinfo.mfo";
    if (!std::filesystem::exists(mfoPath)) {
        std::cout << "SKIP: mapinfo.mfo not found at " << mfoPath.string() << "\n";
        return;
    }

    sro::formats::MapProject project;
    auto result = sro::MapProjectParser::Read(mfoPath.wstring(), project);
    TEST(result.ok);
    TEST(project.ActiveRegions.size() >= 4000);
    TEST(project.ActiveRegions.count({97, 167}) != 0);
    TEST(project.ActiveRegions.count({96, 162}) != 0);

    const size_t activeMegaTiles = CountActiveMegaTiles(project);
    TEST(activeMegaTiles >= 240);
    TEST(activeMegaTiles < 320);
}

int main() {
    TestRegionIdRoundTrip();
    TestSceneSpaceOffsets();
    TestWorldMapCoordsRoundTrip();
    TestWorldMapSouthGateOutside();
    TestLocalMapTexturePixel();
    TestLocalMapWorldRectAndPoiFilter();
    TestLocalMapGridRect();
    TestLocalPoiTextureSpaceAndFallback();
    TestHotanLocalMapCoordSpace();
    TestConstantinopleLocalMapUvAndPoi();
    TestAlexandriaLocalMapUv();
    TestCameraHeadingToMapIconRadians();
    TestDungeonFloorGrouping();
    TestDungeonFloorLabels();
    TestDungeonFloorWorldRect();
    TestDungeonFloorLayout();
    TestMinimapHudCoords();
    TestMinimapTileFilename();
    TestMinimapHudGridAlignment();
    TestPlacementParserFormats();
    TestMapProjectActiveRegions();
    if (g_failures == 0) {
        std::cout << "All SRO core tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
