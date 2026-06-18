#pragma once
#include "Parsers/EfpParser.h"
#include "Rendering/Texture.h"
#include <cmath>

struct UVCellTransform {
    float OffsetU = 0.0f;
    float OffsetV = 0.0f;
    float ScaleU = 1.0f;
    float ScaleV = 1.0f;

    static UVCellTransform Identity() { return UVCellTransform{}; }

    static UVCellTransform ForCell(int col, int row, int totalCols, int totalRows) {
        UVCellTransform t;
        if (totalCols > 0 && totalRows > 0) {
            t.ScaleU = 1.0f / static_cast<float>(totalCols);
            t.ScaleV = 1.0f / static_cast<float>(totalRows);
            t.OffsetU = static_cast<float>(col) * t.ScaleU;
            t.OffsetV = static_cast<float>(row) * t.ScaleV;
        }
        return t;
    }

    static UVCellTransform FromVector4(const Vector4& v) {
        UVCellTransform t;
        t.OffsetU = v.x;
        t.OffsetV = v.y;
        t.ScaleU = v.z;
        t.ScaleV = v.w;
        return t;
    }

    static UVCellTransform Lerp(const UVCellTransform& a, const UVCellTransform& b, float t) {
        UVCellTransform out;
        out.OffsetU = a.OffsetU + (b.OffsetU - a.OffsetU) * t;
        out.OffsetV = a.OffsetV + (b.OffsetV - a.OffsetV) * t;
        out.ScaleU = a.ScaleU + (b.ScaleU - a.ScaleU) * t;
        out.ScaleV = a.ScaleV + (b.ScaleV - a.ScaleV) * t;
        return out;
    }
};

struct SpriteAtlasLayout {
    int Cols = 1;
    int Rows = 1;
    int FrameCount = 1;
};

struct SpriteAnimSample {
    UVCellTransform Cell = UVCellTransform::Identity();
    UVCellTransform NextCell = UVCellTransform::Identity();
    float Blend = 0.0f;
};

inline SpriteAtlasLayout InferAtlasLayoutFromTexture(Texture* tex) {
    SpriteAtlasLayout layout;
    if (!tex || tex->Width == 0 || tex->Height == 0) {
        return layout;
    }
    if (tex->Width == 256 && tex->Height == 256) {
        layout.Cols = 4;
        layout.Rows = 3;
    } else if (tex->Width == 128 && tex->Height == 128) {
        layout.Cols = 4;
        layout.Rows = 4;
    } else if (tex->Width == 256 && tex->Height == 512) {
        layout.Cols = 4;
        layout.Rows = 8;
    } else if (tex->Width == 512 && tex->Height == 256) {
        layout.Cols = 8;
        layout.Rows = 4;
    } else {
        layout.Cols = 1;
        layout.Rows = 1;
    }
    layout.FrameCount = layout.Cols * layout.Rows;
    return layout;
}

inline constexpr float kDefaultSpriteAnimFps = 12.0f;

inline UVCellTransform CellTransformAtTime(const SpriteAtlasLayout& layout, float timeSeconds,
                                           float fps = kDefaultSpriteAnimFps) {
    if (layout.FrameCount <= 1) {
        return UVCellTransform::Identity();
    }
    int frame = static_cast<int>(floorf(timeSeconds * fps)) % layout.FrameCount;
    if (frame < 0) frame += layout.FrameCount;
    return UVCellTransform::ForCell(frame % layout.Cols, frame / layout.Cols,
                                    layout.Cols, layout.Rows);
}

inline float ShapeInterpT(float frac, float interpFactor) {
    if (interpFactor <= 0.0f) return frac;
    if (interpFactor >= 1.0f) return frac;
    return powf(frac, interpFactor);
}

inline SpriteAnimSample SampleSpriteAnim(const FrameTextureSlideData& slide, float normalizedLife) {
    SpriteAnimSample sample;
    if (!slide.HasKeys()) {
        SpriteAtlasLayout layout;
        layout.Cols = slide.Cols();
        layout.Rows = slide.Rows();
        layout.FrameCount = layout.Cols * layout.Rows;
        if (layout.FrameCount <= 1) {
            sample.Cell = UVCellTransform::Identity();
            return sample;
        }
        const float fps = slide.Speed > 0.0f ? slide.Speed : kDefaultSpriteAnimFps;
        const float t = normalizedLife * static_cast<float>(layout.FrameCount);
        const int frameA = static_cast<int>(floorf(t)) % layout.FrameCount;
        const int frameB = (frameA + 1) % layout.FrameCount;
        const float frac = ShapeInterpT(t - floorf(t), slide.Left.z);
        sample.Cell = UVCellTransform::ForCell(frameA % layout.Cols, frameA / layout.Cols,
                                               layout.Cols, layout.Rows);
        sample.NextCell = UVCellTransform::ForCell(frameB % layout.Cols, frameB / layout.Cols,
                                                   layout.Cols, layout.Rows);
        sample.Blend = frac;
        return sample;
    }

    const float clamped = (normalizedLife < 0.0f) ? 0.0f : (normalizedLife > 1.0f ? 1.0f : normalizedLife);
    const float maxIndex = static_cast<float>(slide.Keys.size() - 1);
    const float f = clamped * maxIndex;
    const int idxA = static_cast<int>(floorf(f));
    const int idxB = (idxA + 1 < static_cast<int>(slide.Keys.size())) ? idxA + 1 : idxA;
    const float frac = ShapeInterpT(f - static_cast<float>(idxA), slide.Left.z);

    sample.Cell = UVCellTransform::FromVector4(slide.Keys[static_cast<size_t>(idxA)]);
    sample.NextCell = UVCellTransform::FromVector4(slide.Keys[static_cast<size_t>(idxB)]);
    sample.Blend = (idxA == idxB) ? 0.0f : frac;
    return sample;
}

inline SpriteAnimSample SampleSpriteAnimAtTime(const FrameTextureSlideData& slide, float timeSeconds) {
    float speed = slide.Speed;
    if (speed > 50.0f) {
        speed = speed / 8.333f;
    } else if (speed <= 0.0f) {
        speed = kDefaultSpriteAnimFps;
    }
    const float period = (slide.End > slide.Start) ? (slide.End - slide.Start) : 1.0f;
    float normalized = fmodf(timeSeconds * speed / period, 1.0f);
    if (normalized < 0.0f) normalized += 1.0f;
    return SampleSpriteAnim(slide, normalized);
}

inline SpriteAnimSample SampleSpriteAnimForTexture(const FrameTextureSlideData* slide, Texture* tex,
                                                   float normalizedLife) {
    if (slide && slide->Valid && (slide->HasKeys() || slide->Cols() > 1)) {
        return SampleSpriteAnim(*slide, normalizedLife);
    }
    const SpriteAtlasLayout layout = InferAtlasLayoutFromTexture(tex);
    FrameTextureSlideData fallback;
    fallback.Left = Vector3(static_cast<float>(layout.Cols),
                            static_cast<float>(layout.Rows), 1.0f);
    fallback.Valid = layout.FrameCount > 1;
    return SampleSpriteAnim(fallback, normalizedLife);
}
