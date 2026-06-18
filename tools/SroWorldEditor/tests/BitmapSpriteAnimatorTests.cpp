#include "Rendering/BitmapSpriteAnimator.h"
#include <cmath>
#include <iostream>

static int g_animFailures = 0;

#define ANIM_TEST(cond) do { if (!(cond)) { std::cerr << "FAIL: " #cond " at " << __LINE__ << "\n"; ++g_animFailures; } } while(0)

static FrameTextureSlideData MakeFireSlide() {
    FrameTextureSlideData slide;
    slide.Left = Vector3(4.0f, 3.0f, 0.9f);
    slide.Valid = true;
    slide.Keys = {
        {0.0f, 0.0f, 0.25f, 0.333333f},
        {0.25f, 0.0f, 0.25f, 0.333333f},
        {0.5f, 0.0f, 0.25f, 0.333333f},
    };
    return slide;
}

static void TestSpriteAnimEndpoints() {
    const FrameTextureSlideData slide = MakeFireSlide();
    const SpriteAnimSample start = SampleSpriteAnim(slide, 0.0f);
    ANIM_TEST(std::fabs(start.Cell.OffsetU - 0.0f) < 0.001f);
    ANIM_TEST(std::fabs(start.Cell.ScaleU - 0.25f) < 0.001f);

    const SpriteAnimSample mid = SampleSpriteAnim(slide, 0.5f);
    ANIM_TEST(mid.Cell.OffsetU >= 0.0f);
    ANIM_TEST(mid.Blend >= 0.0f);

    const SpriteAnimSample end = SampleSpriteAnim(slide, 1.0f);
    ANIM_TEST(std::fabs(end.Cell.OffsetU - 0.5f) < 0.001f);
}

static void TestSpriteAnimInterpolation() {
    const FrameTextureSlideData slide = MakeFireSlide();
    const SpriteAnimSample sample = SampleSpriteAnim(slide, 0.25f);
    ANIM_TEST(sample.Blend >= 0.0f && sample.Blend <= 1.0f);
    ANIM_TEST(sample.Cell.ScaleU > 0.0f);
    ANIM_TEST(sample.NextCell.ScaleU > 0.0f);
}

static void TestInferAtlasLayout() {
    FrameTextureSlideData slide;
    slide.Left = Vector3(4.0f, 3.0f, 1.0f);
    slide.Valid = true;
    ANIM_TEST(slide.Cols() == 4);
    ANIM_TEST(slide.Rows() == 3);
    ANIM_TEST(slide.Cols() * slide.Rows() == 12);
}

void RunBitmapSpriteAnimatorTests() {
    TestSpriteAnimEndpoints();
    TestSpriteAnimInterpolation();
    TestInferAtlasLayout();
}

int BitmapSpriteAnimatorTestFailures() {
    return g_animFailures;
}
