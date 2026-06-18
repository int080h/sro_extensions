#pragma once

#include <d3d9.h>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/MathTypes.h"
#include "rendering/Texture.h"

// Client water mesh layout matching oceanwater.vsh inputs:
// v0 position, v3 normal, v8 tangent direction (position1), v7/v4 texcoords, v5 tangent (wave scale in .x)
struct WaterVertex {
    Vector3 Pos;
    Vector3 Normal;
    Vector3 TangentDir;
    Vector2 UV0;
    Vector2 UV1;
    Vector4 WaveScale;  // .x used by shader (0 = full displacement)
};

class WaterRenderer {
public:
    static constexpr int kWaterGrid = 17;
    static constexpr int kAnimFrameCount = 30;
    static constexpr int kAnimBaseId = 101;

    WaterRenderer() = default;
    ~WaterRenderer();

    void Initialize(LPDIRECT3DDEVICE9 device, TextureManager* texManager, const std::wstring& clientPath);
    void Shutdown();

    bool IsReady() const { return m_vs != nullptr && m_ps != nullptr && m_decl != nullptr; }

    // Build indexed water surface for one 320m block (17x17 verts, 16x16 quads).
    bool CreateBlockMesh(LPDIRECT3DVERTEXBUFFER9* outVb, LPDIRECT3DINDEXBUFFER9* outIb,
                         int* outIndexCount, float blockX, float blockZ, float waterHeight);

    void Draw(const Matrix4& world, const Matrix4& view, const Matrix4& proj,
              const Vector3& cameraWorldPos, uint8_t waveType, int8_t waterType,
              LPDIRECT3DVERTEXBUFFER9 vb, LPDIRECT3DINDEXBUFFER9 ib, int indexCount,
              float timeSeconds);

    Texture* GetAnimatedBumpTexture(uint8_t waveType, float timeSeconds);
    Texture* GetSecondaryTexture(uint8_t waveType);
    void InvalidateTextureCache();

private:
    bool LoadShaders();
    bool EnsureDeclaration();
    void UploadConstants(const Matrix4& world, const Matrix4& view, const Matrix4& proj,
                         const Vector3& cameraWorldPos, float timeSeconds);
    void SaveRenderState();
    void RestoreRenderState();
    std::wstring ResolveWaterTexturePath(const wchar_t* fileName) const;

    LPDIRECT3DDEVICE9 m_device = nullptr;
    TextureManager* m_texManager = nullptr;
    std::wstring m_clientPath;

    IDirect3DVertexShader9* m_vs = nullptr;
    IDirect3DPixelShader9* m_ps = nullptr;
    IDirect3DVertexDeclaration9* m_decl = nullptr;

    std::map<int, Texture*> m_animFrames;
    std::map<uint8_t, Texture*> m_secondaryTextures;
    std::set<uint8_t> m_loggedMissing;

    bool m_stateSaved = false;
    DWORD m_oldFvf = 0;
    IDirect3DVertexShader9* m_oldVs = nullptr;
    IDirect3DPixelShader9* m_oldPs = nullptr;
    IDirect3DVertexDeclaration9* m_oldDecl = nullptr;
};
