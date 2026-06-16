#pragma once
#include <d3d9.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// ============================================================================
// Direct3D 9 texture wrapper and texture cache manager
// ============================================================================

class Texture {
public:
    LPDIRECT3DTEXTURE9 pTexture = nullptr;
    UINT Width = 0;
    UINT Height = 0;

    Texture() = default;
    ~Texture();

    static std::unique_ptr<Texture> LoadFromDdsBytes(LPDIRECT3DDEVICE9 device, const std::vector<uint8_t>& ddsBytes);
};

class TextureManager {
private:
    LPDIRECT3DDEVICE9 m_device;
    std::map<std::wstring, std::unique_ptr<Texture>> m_cache;
    int m_loadedThisFrame = 0;
    static constexpr int MAX_LOADS_PER_FRAME = 6; // Increased from 2 for faster texture pop-in
    std::unique_ptr<Texture> m_defaultTexture;

    void CreateDefaultTexture();

public:
    TextureManager(LPDIRECT3DDEVICE9 device);
    
    void NewFrame() { m_loadedThisFrame = 0; }
    Texture* GetTexture(const std::wstring& path, bool progressive = false);
    Texture* GetDefaultTexture();
    void Clear();

    size_t GetCacheSize() const { return m_cache.size(); }
    int GetLoadedThisFrame() const { return m_loadedThisFrame; }
    int GetMaxLoadsPerFrame() const { return MAX_LOADS_PER_FRAME; }
};

// DDS decode helpers (used by Texture and WorldMapControl atlas builder)
struct DecodedImage {
    int Width = 0;
    int Height = 0;
    std::vector<uint8_t> Bgra;
};

bool DecodeDdsToBgra(const std::vector<uint8_t>& ddsBytes, DecodedImage& out);
void DownsampleNearest(const DecodedImage& src, int dstSize, uint8_t* dstBgra);
