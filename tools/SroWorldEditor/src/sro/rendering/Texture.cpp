#include "Texture.h"
#include "Core/Log.h"
#include "Parsers/DdjParser.h"
#include <algorithm>
#include <cstring>

// ============================================================================
// DXT Block Decoding Helpers
// ============================================================================

static void DecodeDxt1(const uint8_t *input, size_t inputSize, int width,
                       int height, uint8_t *output) {
  int blockWidth = (width + 3) / 4;
  int blockHeight = (height + 3) / 4;
  int offset = 0;

  for (int by = 0; by < blockHeight; by++) {
    for (int bx = 0; bx < blockWidth; bx++) {
      if (offset + 8 > inputSize)
        break;
      uint16_t c0 = *reinterpret_cast<const uint16_t *>(input + offset);
      uint16_t c1 = *reinterpret_cast<const uint16_t *>(input + offset + 2);
      uint32_t code = *reinterpret_cast<const uint32_t *>(input + offset + 4);
      offset += 8;

      uint8_t r0 = ((c0 >> 11) & 31) * 255 / 31;
      uint8_t g0 = ((c0 >> 5) & 63) * 255 / 63;
      uint8_t b0 = (c0 & 31) * 255 / 31;

      uint8_t r1 = ((c1 >> 11) & 31) * 255 / 31;
      uint8_t g1 = ((c1 >> 5) & 63) * 255 / 63;
      uint8_t b1 = (c1 & 31) * 255 / 31;

      int shift = 0;
      for (int py = 0; py < 4; py++) {
        int gy = by * 4 + py;
        for (int px = 0; px < 4; px++) {
          int gx = bx * 4 + px;
          int colorCode = (code >> shift) & 3;
          shift += 2;

          if (gx >= width || gy >= height)
            continue;

          uint8_t r = 0, g = 0, b = 0, a = 255;
          if (c0 > c1) {
            switch (colorCode) {
            case 0: r = r0; g = g0; b = b0; break;
            case 1: r = r1; g = g1; b = b1; break;
            case 2: r = (2 * r0 + r1) / 3; g = (2 * g0 + g1) / 3; b = (2 * b0 + b1) / 3; break;
            case 3: r = (r0 + 2 * r1) / 3; g = (g0 + 2 * g1) / 3; b = (b0 + 2 * b1) / 3; break;
            }
          } else {
            switch (colorCode) {
            case 0: r = r0; g = g0; b = b0; break;
            case 1: r = r1; g = g1; b = b1; break;
            case 2: r = (r0 + r1) / 2; g = (g0 + g1) / 2; b = (b0 + b1) / 2; break;
            case 3: r = 0; g = 0; b = 0; a = 0; break;
            }
          }

          int outOffset = (gy * width + gx) * 4;
          output[outOffset] = b;
          output[outOffset + 1] = g;
          output[outOffset + 2] = r;
          output[outOffset + 3] = a;
        }
      }
    }
  }
}

static void DecodeDxt5(const uint8_t *input, size_t inputSize, int width,
                       int height, uint8_t *output) {
  int blockWidth = (width + 3) / 4;
  int blockHeight = (height + 3) / 4;
  int offset = 0;

  for (int by = 0; by < blockHeight; by++) {
    for (int bx = 0; bx < blockWidth; bx++) {
      if (offset + 16 > inputSize)
        break;

      uint8_t a0 = input[offset];
      uint8_t a1 = input[offset + 1];

      uint64_t alphaIndices = 0;
      for (int i = 0; i < 6; i++) {
        alphaIndices |= (static_cast<uint64_t>(input[offset + 2 + i])) << (8 * i);
      }

      uint16_t c0 = *reinterpret_cast<const uint16_t *>(input + offset + 8);
      uint16_t c1 = *reinterpret_cast<const uint16_t *>(input + offset + 10);
      uint32_t code = *reinterpret_cast<const uint32_t *>(input + offset + 12);
      offset += 16;

      uint8_t r0 = ((c0 >> 11) & 31) * 255 / 31;
      uint8_t g0 = ((c0 >> 5) & 63) * 255 / 63;
      uint8_t b0 = (c0 & 31) * 255 / 31;

      uint8_t r1 = ((c1 >> 11) & 31) * 255 / 31;
      uint8_t g1 = ((c1 >> 5) & 63) * 255 / 63;
      uint8_t b1 = (c1 & 31) * 255 / 31;

      int shiftColor = 0;
      int shiftAlpha = 0;
      for (int py = 0; py < 4; py++) {
        int gy = by * 4 + py;
        for (int px = 0; px < 4; px++) {
          int gx = bx * 4 + px;
          int colorCode = (code >> shiftColor) & 3;
          int alphaCode = (alphaIndices >> shiftAlpha) & 7;
          shiftColor += 2;
          shiftAlpha += 3;

          if (gx >= width || gy >= height)
            continue;

          uint8_t r = 0, g = 0, b = 0;
          switch (colorCode) {
          case 0: r = r0; g = g0; b = b0; break;
          case 1: r = r1; g = g1; b = b1; break;
          case 2: r = (2 * r0 + r1) / 3; g = (2 * g0 + g1) / 3; b = (2 * b0 + b1) / 3; break;
          case 3: r = (r0 + 2 * r1) / 3; g = (g0 + 2 * g1) / 3; b = (b0 + 2 * b1) / 3; break;
          }

          uint8_t a = 255;
          if (a0 > a1) {
            switch (alphaCode) {
            case 0: a = a0; break;
            case 1: a = a1; break;
            case 2: a = (6 * a0 + a1) / 7; break;
            case 3: a = (5 * a0 + 2 * a1) / 7; break;
            case 4: a = (4 * a0 + 3 * a1) / 7; break;
            case 5: a = (3 * a0 + 4 * a1) / 7; break;
            case 6: a = (2 * a0 + 5 * a1) / 7; break;
            case 7: a = (a0 + 6 * a1) / 7; break;
            }
          } else {
            switch (alphaCode) {
            case 0: a = a0; break;
            case 1: a = a1; break;
            case 2: a = (4 * a0 + a1) / 5; break;
            case 3: a = (3 * a0 + 2 * a1) / 5; break;
            case 4: a = (2 * a0 + 3 * a1) / 5; break;
            case 5: a = (a0 + 4 * a1) / 5; break;
            case 6: a = 0; break;
            case 7: a = 255; break;
            }
          }

          int outOffset = (gy * width + gx) * 4;
          output[outOffset] = b;
          output[outOffset + 1] = g;
          output[outOffset + 2] = r;
          output[outOffset + 3] = a;
        }
      }
    }
  }
}

static void DecodeDxt3(const uint8_t *input, size_t inputSize, int width,
                       int height, uint8_t *output) {
  int blockWidth = (width + 3) / 4;
  int blockHeight = (height + 3) / 4;
  int offset = 0;

  for (int by = 0; by < blockHeight; by++) {
    for (int bx = 0; bx < blockWidth; bx++) {
      if (offset + 16 > inputSize)
        break;

      const uint8_t *alphaBlock = input + offset;
      uint16_t c0 = *reinterpret_cast<const uint16_t *>(input + offset + 8);
      uint16_t c1 = *reinterpret_cast<const uint16_t *>(input + offset + 10);
      uint32_t code = *reinterpret_cast<const uint32_t *>(input + offset + 12);
      offset += 16;

      uint8_t r0 = ((c0 >> 11) & 31) * 255 / 31;
      uint8_t g0 = ((c0 >> 5) & 63) * 255 / 63;
      uint8_t b0 = (c0 & 31) * 255 / 31;

      uint8_t r1 = ((c1 >> 11) & 31) * 255 / 31;
      uint8_t g1 = ((c1 >> 5) & 63) * 255 / 63;
      uint8_t b1 = (c1 & 31) * 255 / 31;

      int shiftColor = 0;
      for (int py = 0; py < 4; py++) {
        int gy = by * 4 + py;
        uint16_t alphaRow = *reinterpret_cast<const uint16_t *>(alphaBlock + py * 2);
        for (int px = 0; px < 4; px++) {
          int gx = bx * 4 + px;
          int colorCode = (code >> shiftColor) & 3;
          shiftColor += 2;

          if (gx >= width || gy >= height)
            continue;

          uint8_t r = 0, g = 0, b = 0;
          switch (colorCode) {
          case 0: r = r0; g = g0; b = b0; break;
          case 1: r = r1; g = g1; b = b1; break;
          case 2: r = (2 * r0 + r1) / 3; g = (2 * g0 + g1) / 3; b = (2 * b0 + b1) / 3; break;
          case 3: r = (r0 + 2 * r1) / 3; g = (g0 + 2 * g1) / 3; b = (b0 + 2 * b1) / 3; break;
          }

          uint8_t a4 = (alphaRow >> (px * 4)) & 0xF;
          uint8_t a = a4 | (a4 << 4);

          int outOffset = (gy * width + gx) * 4;
          output[outOffset] = b;
          output[outOffset + 1] = g;
          output[outOffset + 2] = r;
          output[outOffset + 3] = a;
        }
      }
    }
  }
}

// ============================================================================
// DDS to BGRA decoding (public — used by atlas builder)
// ============================================================================

bool DecodeDdsToBgra(const std::vector<uint8_t> &ddsBytes, DecodedImage &out) {
  if (ddsBytes.size() < 128)
    return false;

  uint32_t magic = *reinterpret_cast<const uint32_t *>(&ddsBytes[0]);
  if (magic != 0x20534444)
    return false;

  int height = *reinterpret_cast<const int32_t *>(&ddsBytes[12]);
  int width = *reinterpret_cast<const int32_t *>(&ddsBytes[16]);
  uint32_t fourCC = *reinterpret_cast<const uint32_t *>(&ddsBytes[84]);
  uint32_t rgbBitCount = *reinterpret_cast<const uint32_t *>(&ddsBytes[88]);
  uint32_t pfFlags = *reinterpret_cast<const uint32_t *>(&ddsBytes[80]);
  uint32_t alphaMask = *reinterpret_cast<const uint32_t *>(&ddsBytes[104]);

  const uint8_t *inputData = ddsBytes.data() + 128;
  size_t inputSize = ddsBytes.size() - 128;

  out.Width = width;
  out.Height = height;
  out.Bgra.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);

  if (fourCC == 0x31545844) {
    DecodeDxt1(inputData, inputSize, width, height, out.Bgra.data());
    return true;
  }
  if (fourCC == 0x33545844) {
    DecodeDxt3(inputData, inputSize, width, height, out.Bgra.data());
    return true;
  }
  if (fourCC == 0x35545844) {
    DecodeDxt5(inputData, inputSize, width, height, out.Bgra.data());
    return true;
  }
  if (fourCC == 0 && rgbBitCount == 32) {
    size_t copySize = (std::min)(inputSize, out.Bgra.size());
    std::memcpy(out.Bgra.data(), inputData, copySize);
    if ((pfFlags & 0x1) == 0 || alphaMask == 0) {
      for (int i = 0; i < width * height; ++i) {
        out.Bgra[i * 4 + 3] = 255;
      }
    }
    return true;
  }
  if (fourCC == 0 && rgbBitCount == 24) {
    size_t readOffset = 0;
    for (int i = 0; i < width * height; ++i) {
      if (readOffset + 3 > inputSize)
        break;
      out.Bgra[i * 4 + 0] = inputData[readOffset + 0];
      out.Bgra[i * 4 + 1] = inputData[readOffset + 1];
      out.Bgra[i * 4 + 2] = inputData[readOffset + 2];
      out.Bgra[i * 4 + 3] = 255;
      readOffset += 3;
    }
    return true;
  }

  return false;
}

void DownsampleNearest(const DecodedImage &src, int dstSize, uint8_t *dstBgra) {
  for (int y = 0; y < dstSize; ++y) {
    int srcY = y * src.Height / dstSize;
    for (int x = 0; x < dstSize; ++x) {
      int srcX = x * src.Width / dstSize;
      size_t srcOffset = (static_cast<size_t>(srcY) * src.Width + srcX) * 4;
      size_t dstOffset = (static_cast<size_t>(y) * dstSize + x) * 4;
      std::memcpy(dstBgra + dstOffset, src.Bgra.data() + srcOffset, 4);
    }
  }
}

// ============================================================================
// Texture class implementation
// ============================================================================

Texture::~Texture() {
  if (pTexture) {
    pTexture->Release();
    pTexture = nullptr;
  }
}

std::unique_ptr<Texture> Texture::LoadFromDdsBytes(LPDIRECT3DDEVICE9 device,
                          const std::vector<uint8_t> &ddsBytes) {
  if (ddsBytes.size() < 128) {
    LogMsg("[Texture] Failed LoadFromDdsBytes: size < 128");
    return nullptr;
  }

  uint32_t magic = *reinterpret_cast<const uint32_t *>(&ddsBytes[0]);
  if (magic != 0x20534444) {
    LogMsg("[Texture] Failed LoadFromDdsBytes: magic != 0x20534444");
    return nullptr;
  }

  int height = *reinterpret_cast<const int32_t *>(&ddsBytes[12]);
  int width = *reinterpret_cast<const int32_t *>(&ddsBytes[16]);
  uint32_t fourCC = *reinterpret_cast<const uint32_t *>(&ddsBytes[84]);
  uint32_t rgbBitCount = *reinterpret_cast<const uint32_t *>(&ddsBytes[88]);
  uint32_t pfFlags = *reinterpret_cast<const uint32_t *>(&ddsBytes[80]);
  uint32_t alphaMask = *reinterpret_cast<const uint32_t *>(&ddsBytes[104]);
  int mipMapCount = *reinterpret_cast<const int32_t *>(&ddsBytes[28]);
  if (mipMapCount <= 0) mipMapCount = 1;

  std::vector<uint8_t> bgraBytes;
  D3DFORMAT format = D3DFMT_A8R8G8B8;
  bool isCompressed = false;

  if (fourCC == 0x31545844) { format = D3DFMT_DXT1; isCompressed = true; }
  else if (fourCC == 0x32545844) { format = D3DFMT_DXT2; isCompressed = true; }
  else if (fourCC == 0x33545844) { format = D3DFMT_DXT3; isCompressed = true; }
  else if (fourCC == 0x34545844) { format = D3DFMT_DXT4; isCompressed = true; }
  else if (fourCC == 0x35545844) { format = D3DFMT_DXT5; isCompressed = true; }
  else if (fourCC == 0 && (rgbBitCount == 32 || rgbBitCount == 24 || rgbBitCount == 16)) {
      format = D3DFMT_A8R8G8B8;
      isCompressed = false;
  } else {
    char errLog[256];
    sprintf_s(errLog, "[Texture] Failed LoadFromDdsBytes: Unrecognized format! fourCC=0x%08X, rgbBitCount=%d, pfFlags=0x%08X",
              fourCC, rgbBitCount, pfFlags);
    LogMsg(errLog);
    return nullptr;
  }

  LPDIRECT3DTEXTURE9 pTexture = nullptr;
  HRESULT hr = device->CreateTexture(
      static_cast<UINT>(width), static_cast<UINT>(height), mipMapCount, 0,
      format, D3DPOOL_MANAGED, &pTexture, nullptr);

  if (FAILED(hr)) {
    char errLog[256];
    sprintf_s(errLog, "[Texture] CreateTexture FAILED! hr=0x%08X, width=%d, height=%d, format=%d, levels=%d",
              hr, width, height, format, mipMapCount);
    LogMsg(errLog);
    return nullptr;
  }

  size_t fileOffset = 128;
  const uint8_t *inputData = ddsBytes.data();
  size_t inputSize = ddsBytes.size();

  for (int level = 0; level < mipMapCount; ++level) {
    int levelWidth = (std::max)(1, width >> level);
    int levelHeight = (std::max)(1, height >> level);

    D3DLOCKED_RECT locked;
    if (FAILED(pTexture->LockRect(level, &locked, nullptr, 0))) {
        break;
    }

    if (isCompressed) {
      size_t blockWidth = (levelWidth + 3) / 4;
      size_t blockHeight = (levelHeight + 3) / 4;
      size_t bytesPerBlockRow = blockWidth * (format == D3DFMT_DXT1 ? 8 : 16);
      size_t levelSize = blockWidth * blockHeight * (format == D3DFMT_DXT1 ? 8 : 16);

      if (fileOffset + levelSize <= inputSize) {
        for (size_t y = 0; y < blockHeight; ++y) {
          std::memcpy(static_cast<char *>(locked.pBits) + y * locked.Pitch,
                      inputData + fileOffset + y * bytesPerBlockRow, bytesPerBlockRow);
        }
        fileOffset += levelSize;
      }
    } else {
      size_t pixelCount = levelWidth * levelHeight;
      bgraBytes.assign(pixelCount * 4, 0);

      if (rgbBitCount == 32) {
        size_t levelSize = pixelCount * 4;
        if (fileOffset + levelSize <= inputSize) {
          std::memcpy(bgraBytes.data(), inputData + fileOffset, levelSize);
          if ((pfFlags & 0x1) == 0 || alphaMask == 0) {
            for (size_t i = 0; i < pixelCount; ++i) { bgraBytes[i * 4 + 3] = 255; }
          }
          fileOffset += levelSize;
        }
      } else if (rgbBitCount == 24) {
        size_t levelSize = pixelCount * 3;
        if (fileOffset + levelSize <= inputSize) {
          const uint8_t* src = inputData + fileOffset;
          for (size_t i = 0; i < pixelCount; ++i) {
            bgraBytes[i * 4 + 0] = src[i * 3 + 0];
            bgraBytes[i * 4 + 1] = src[i * 3 + 1];
            bgraBytes[i * 4 + 2] = src[i * 3 + 2];
            bgraBytes[i * 4 + 3] = 255;
          }
          fileOffset += levelSize;
        }
      } else if (rgbBitCount == 16) {
        size_t levelSize = pixelCount * 2;
        if (fileOffset + levelSize <= inputSize) {
          uint32_t rMask = *reinterpret_cast<const uint32_t *>(&ddsBytes[92]);
          uint32_t gMask = *reinterpret_cast<const uint32_t *>(&ddsBytes[96]);
          uint32_t bMask = *reinterpret_cast<const uint32_t *>(&ddsBytes[100]);
          const uint8_t* src = inputData + fileOffset;

          if (alphaMask == 0x8000 && rMask == 0x7c00 && gMask == 0x3e0 && bMask == 0x1f) {
            for (size_t i = 0; i < pixelCount; ++i) {
              uint16_t val = *reinterpret_cast<const uint16_t *>(src + i * 2);
              bgraBytes[i * 4] = static_cast<uint8_t>((val & 0x001f) * 255 / 31);
              bgraBytes[i * 4 + 1] = static_cast<uint8_t>(((val & 0x03e0) >> 5) * 255 / 31);
              bgraBytes[i * 4 + 2] = static_cast<uint8_t>(((val & 0x7c00) >> 10) * 255 / 31);
              bgraBytes[i * 4 + 3] = (val & 0x8000) ? 255 : 0;
            }
          } else if (alphaMask == 0xf000 && rMask == 0x0f00 && gMask == 0x00f0 && bMask == 0x000f) {
            for (size_t i = 0; i < pixelCount; ++i) {
              uint16_t val = *reinterpret_cast<const uint16_t *>(src + i * 2);
              bgraBytes[i * 4] = static_cast<uint8_t>((val & 0x000f) * 17);
              bgraBytes[i * 4 + 1] = static_cast<uint8_t>(((val & 0x00f0) >> 4) * 17);
              bgraBytes[i * 4 + 2] = static_cast<uint8_t>(((val & 0x0f00) >> 8) * 17);
              bgraBytes[i * 4 + 3] = static_cast<uint8_t>(((val & 0xf000) >> 12) * 17);
            }
          } else {
            for (size_t i = 0; i < pixelCount; ++i) {
              uint16_t val = *reinterpret_cast<const uint16_t *>(src + i * 2);
              uint8_t r = 0, g = 0, b = 0;
              if (rMask == 0xf800 && gMask == 0x07e0 && bMask == 0x001f) {
                r = static_cast<uint8_t>(((val & 0xf800) >> 11) * 255 / 31);
                g = static_cast<uint8_t>(((val & 0x07e0) >> 5) * 255 / 63);
                b = static_cast<uint8_t>((val & 0x001f) * 255 / 31);
              } else {
                r = static_cast<uint8_t>(((val >> 10) & 0x1f) * 255 / 31);
                g = static_cast<uint8_t>(((val >> 5) & 0x1f) * 255 / 31);
                b = static_cast<uint8_t>((val & 0x1f) * 255 / 31);
              }
              bgraBytes[i * 4] = b; bgraBytes[i * 4 + 1] = g;
              bgraBytes[i * 4 + 2] = r; bgraBytes[i * 4 + 3] = 255;
            }
          }
          fileOffset += levelSize;
        }
      }

      for (int y = 0; y < levelHeight; ++y) {
        std::memcpy(static_cast<char *>(locked.pBits) + y * locked.Pitch,
                    bgraBytes.data() + y * levelWidth * 4, levelWidth * 4);
      }
    }

    pTexture->UnlockRect(level);
  }

  auto tex = std::make_unique<Texture>();
  tex->pTexture = pTexture;
  tex->Width = width;
  tex->Height = height;
  return tex;
}

// ============================================================================
// TextureManager implementation
// ============================================================================

TextureManager::TextureManager(LPDIRECT3DDEVICE9 device) : m_device(device) {
  CreateDefaultTexture();
}

void TextureManager::CreateDefaultTexture() {
  LPDIRECT3DTEXTURE9 pTex = nullptr;
  HRESULT hr = m_device->CreateTexture(2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTex, nullptr);
  if (SUCCEEDED(hr) && pTex) {
    D3DLOCKED_RECT locked;
    if (SUCCEEDED(pTex->LockRect(0, &locked, nullptr, 0))) {
      DWORD* pixels = static_cast<DWORD*>(locked.pBits);
      // Solid dark olive/greenish-grey color that blends nicely with terrain at night
      DWORD col = D3DCOLOR_ARGB(255, 32, 40, 32);
      pixels[0] = col;
      pixels[1] = col;
      pixels[2] = col;
      pixels[3] = col;
      pTex->UnlockRect(0);
    }
    m_defaultTexture = std::make_unique<Texture>();
    m_defaultTexture->pTexture = pTex;
    m_defaultTexture->Width = 2;
    m_defaultTexture->Height = 2;
  }
}

Texture* TextureManager::GetDefaultTexture() {
  return m_defaultTexture.get();
}

Texture *TextureManager::GetTexture(const std::wstring &path, bool progressive) {
  auto it = m_cache.find(path);
  if (it != m_cache.end()) {
    return it->second.get();
  }

  if (progressive && m_loadedThisFrame >= MAX_LOADS_PER_FRAME) {
    return nullptr;
  }

  std::vector<uint8_t> ddsBytes = DdjParser::LoadToDdsBytes(path);
  if (ddsBytes.empty()) {
    LogMsgW(L"[TextureManager] Error: Failed to load DDJ/DDS bytes: " + path);
    m_cache[path] = nullptr;
    return nullptr;
  }

  auto tex = Texture::LoadFromDdsBytes(m_device, ddsBytes);
  if (!tex) {
    LogMsgW(L"[TextureManager] Error: Failed to parse DDS texture: " + path);
    m_cache[path] = nullptr;
    return nullptr;
  }

  if (progressive) {
    m_loadedThisFrame++;
  }

  Texture *ptr = tex.get();
  m_cache[path] = std::move(tex);
  return ptr;
}

void TextureManager::Clear() { m_cache.clear(); }
