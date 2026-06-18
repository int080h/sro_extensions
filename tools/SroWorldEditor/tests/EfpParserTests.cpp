#include "parsers/EfpParser.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

static int g_efpFailures = 0;

#define TEST(cond) do { if (!(cond)) { std::cerr << "FAIL: " #cond " at " << __LINE__ << "\n"; ++g_efpFailures; } } while(0)

static std::wstring ClientParticlesPath(const char* rel) {
    return L"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/" +
           std::wstring(rel, rel + std::strlen(rel));
}

static void TestRedOrangeFlameEfp() {
    EfpResource res;
    EfpParser parser;
    const std::wstring path = ClientParticlesPath("map/guild/red_orange_flame_01.efp");
    TEST(parser.Read(path, res));
    TEST(!res.RenderItems.empty());
    TEST(res.HasTextureSlide);

    const EfpRenderItem* fireItem = nullptr;
    for (const EfpRenderItem& item : res.RenderItems) {
        if (!item.TexturePaths.empty() &&
            item.TexturePaths.front().find("fire001") != std::string::npos) {
            fireItem = &item;
            break;
        }
    }
    TEST(fireItem != nullptr);
    if (!fireItem) return;

    TEST(fireItem->SlideData.Valid);
    TEST(fireItem->SlideData.Cols() == 4);
    TEST(fireItem->SlideData.Rows() == 3);
    TEST(fireItem->SlideData.Keys.size() == 24);
    TEST(std::fabs(fireItem->SlideData.Keys[0].x - 0.0f) < 0.001f);
    TEST(std::fabs(fireItem->SlideData.Keys[0].y - 0.0f) < 0.001f);
    TEST(std::fabs(fireItem->SlideData.Keys[0].z - 0.25f) < 0.001f);
    TEST(std::fabs(fireItem->SlideData.Keys[0].w - 0.333333f) < 0.001f);
    TEST(!res.Objects.empty());
}

static void TestWaterPapyunEfp() {
    EfpResource res;
    EfpParser parser;
    const std::wstring path = ClientParticlesPath("map/pokpopapyun.efp");
    TEST(parser.Read(path, res));
    TEST(!res.RenderItems.empty());

    const EfpRenderItem* waterItem = nullptr;
    for (const EfpRenderItem& item : res.RenderItems) {
        if (!item.TexturePaths.empty() &&
            item.TexturePaths.front().find("waterpapyun") != std::string::npos) {
            waterItem = &item;
            break;
        }
    }
    TEST(waterItem != nullptr);
    if (!waterItem) return;

    TEST(waterItem->SlideData.Valid);
    TEST(waterItem->SlideData.Cols() == 4);
    TEST(waterItem->SlideData.Rows() == 4);
    TEST(waterItem->SlideData.Keys.size() == 17);
    TEST(std::fabs(waterItem->SlideData.Keys[0].z - 0.25f) < 0.001f);
    TEST(std::fabs(waterItem->SlideData.Keys[0].w - 0.25f) < 0.001f);
}

static void TestGlowPlateDoesNotInheritFireSlide() {
    EfpResource res;
    EfpParser parser;
    const std::wstring path = ClientParticlesPath("dun/blue_flame_stonelantern.efp");
    TEST(parser.Read(path, res));
    TEST(!res.RenderItems.empty());

    const EfpRenderItem* fireItem = nullptr;
    const EfpRenderItem* glowItem = nullptr;
    for (const EfpRenderItem& item : res.RenderItems) {
        if (item.TexturePaths.empty()) continue;
        const std::string& tex = item.TexturePaths.front();
        if (tex.find("fire2.ddj") != std::string::npos ||
            tex.find("fire001-b.ddj") != std::string::npos) {
            fireItem = &item;
        }
        if (tex.find("01basic2.ddj") != std::string::npos) {
            glowItem = &item;
        }
    }

    TEST(fireItem != nullptr);
    TEST(glowItem != nullptr);
    if (!fireItem || !glowItem) return;

    TEST(fireItem->TextureSlide);
    TEST(fireItem->SlideData.Valid);
    TEST(!glowItem->TextureSlide);
    TEST(!glowItem->SlideData.Valid);
}

void RunEfpParserTests() {
    TestRedOrangeFlameEfp();
    TestWaterPapyunEfp();
    TestGlowPlateDoesNotInheritFireSlide();
}

int EfpParserTestFailures() {
    return g_efpFailures;
}
