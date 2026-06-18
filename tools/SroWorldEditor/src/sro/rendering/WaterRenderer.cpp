#include "rendering/WaterRenderer.h"

#include "Core/Log.h"
#include "Core/Utils.h"
#include "rendering/D3dx9ShaderAssembler.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

constexpr float kPi = 3.14159265358979323846f;

void MatrixToShaderConstants(const Matrix4& m, float out[16]) {
    // D3D9 vs1.1 m4x4 expects columns in cN..cN+3 registers.
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            out[col * 4 + row] = m.m[row][col];
        }
    }
}

void Matrix3x3ToShaderConstants(const Matrix4& m, float out[12]) {
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            out[col * 3 + row] = m.m[row][col];
        }
    }
}

Vector3 TransformPoint(const Matrix4& m, const Vector3& p) {
    return Vector3(
        p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0],
        p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1],
        p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2]);
}

}  // namespace

WaterRenderer::~WaterRenderer() { Shutdown(); }

void WaterRenderer::Initialize(LPDIRECT3DDEVICE9 device, TextureManager* texManager,
                               const std::wstring& clientPath) {
    Shutdown();
    m_device = device;
    m_texManager = texManager;
    m_clientPath = clientPath;
    if (!m_device || !m_texManager || m_clientPath.empty()) return;

    EnsureDeclaration();
    LoadShaders();
    if (!IsReady()) {
        Logger::Instance().Warning(
            "WaterRenderer: failed to load oceanwater shaders — terrain water will use fallback");
    }
}

void WaterRenderer::Shutdown() {
    if (m_vs) {
        m_vs->Release();
        m_vs = nullptr;
    }
    if (m_ps) {
        m_ps->Release();
        m_ps = nullptr;
    }
    if (m_decl) {
        m_decl->Release();
        m_decl = nullptr;
    }
    m_animFrames.clear();
    m_secondaryTextures.clear();
    m_loggedMissing.clear();
    m_device = nullptr;
    m_texManager = nullptr;
}

void WaterRenderer::InvalidateTextureCache() {
    m_animFrames.clear();
    m_secondaryTextures.clear();
}

bool WaterRenderer::EnsureDeclaration() {
    if (m_decl || !m_device) return m_decl != nullptr;

    const D3DVERTEXELEMENT9 elements[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1},
        {0, 36, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 44, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        {0, 52, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
        D3DDECL_END(),
    };

    return SUCCEEDED(m_device->CreateVertexDeclaration(elements, &m_decl));
}

bool WaterRenderer::LoadShaders() {
    if (!m_device) return false;

    const std::wstring shaderRoots[] = {
        m_clientPath + L"/Data/shader",
        m_clientPath + L"/data/shader",
        m_clientPath + L"/Shader",
    };

    std::wstring vsPath;
    std::wstring psPath;
    for (const auto& root : shaderRoots) {
        const std::wstring vs = root + L"/oceanwater.vsh";
        const std::wstring ps = root + L"/oceanwater.psh";
        if (FileExists(vs) && FileExists(ps)) {
            vsPath = vs;
            psPath = ps;
            break;
        }
    }
    if (vsPath.empty()) {
        Logger::Instance().Warning("WaterRenderer: oceanwater.vsh/psh not found under Data/shader");
        return false;
    }

    if (!D3dx9ShaderAssembler::Instance().Available()) {
        Logger::Instance().Warning(
            "WaterRenderer: d3dx9_43.dll not found — install DirectX End-User Runtime for client water shaders");
        return false;
    }

    std::vector<DWORD> vsBytecode;
    std::vector<DWORD> psBytecode;
    std::string asmError;
    if (!D3dx9ShaderAssembler::Instance().AssembleFile(vsPath, vsBytecode, asmError)) {
        Logger::Instance().Warning("oceanwater.vsh assemble failed: " + asmError);
        return false;
    }
    if (!D3dx9ShaderAssembler::Instance().AssembleFile(psPath, psBytecode, asmError)) {
        Logger::Instance().Warning("oceanwater.psh assemble failed: " + asmError);
        return false;
    }

    const HRESULT createVs = m_device->CreateVertexShader(vsBytecode.data(), &m_vs);
    const HRESULT createPs = m_device->CreatePixelShader(psBytecode.data(), &m_ps);

    if (FAILED(createVs) || FAILED(createPs)) {
        if (m_vs) {
            m_vs->Release();
            m_vs = nullptr;
        }
        if (m_ps) {
            m_ps->Release();
            m_ps = nullptr;
        }
        return false;
    }

    Logger::Instance().Info("WaterRenderer: loaded client oceanwater shaders");
    return true;
}

std::wstring WaterRenderer::ResolveWaterTexturePath(const wchar_t* fileName) const {
    const std::wstring roots[] = {
        m_clientPath + L"/Map/water",
        m_clientPath + L"/map/water",
        m_clientPath + L"/Data/shader/water",
        m_clientPath + L"/data/shader/water",
    };
    for (const auto& root : roots) {
        const std::wstring path = root + L"/" + fileName;
        if (FileExists(path)) return path;
    }
    return {};
}

Texture* WaterRenderer::GetAnimatedBumpTexture(uint8_t waveType, float timeSeconds) {
    (void)waveType;
    const float frameRate = 12.0f;
    const int frame = static_cast<int>(timeSeconds * frameRate) % kAnimFrameCount;
    const int texId = kAnimBaseId + frame;

    auto it = m_animFrames.find(texId);
    if (it != m_animFrames.end()) return it->second;

    wchar_t fileName[32]{};
    swprintf_s(fileName, L"water%u.ddj", texId);
    const std::wstring path = ResolveWaterTexturePath(fileName);
    if (path.empty()) return nullptr;

    Texture* tex = m_texManager->GetTexture(path, false);
    m_animFrames[texId] = tex;
    return tex;
}

Texture* WaterRenderer::GetSecondaryTexture(uint8_t waveType) {
    auto cached = m_secondaryTextures.find(waveType);
    if (cached != m_secondaryTextures.end()) return cached->second;

    wchar_t fileName[32]{};
    if (waveType == 2) {
        wcscpy_s(fileName, L"water201.ddj");
    } else {
        const unsigned waveIdx = std::clamp(static_cast<unsigned>(waveType), 1u, 3u);
        swprintf_s(fileName, L"wave%u.ddj", waveIdx);
    }

    const std::wstring path = ResolveWaterTexturePath(fileName);
    Texture* tex = nullptr;
    if (!path.empty()) {
        tex = m_texManager->GetTexture(path, false);
    }
    if (!tex) {
        tex = GetAnimatedBumpTexture(waveType, 0.0f);
    }

    m_secondaryTextures[waveType] = tex;
    if (!tex && m_loggedMissing.insert(waveType).second) {
        Logger::Instance().Warning("WaterRenderer: secondary water texture missing for wave type " +
                                   std::to_string(waveType));
    }
    return tex;
}

bool WaterRenderer::CreateBlockMesh(LPDIRECT3DVERTEXBUFFER9* outVb, LPDIRECT3DINDEXBUFFER9* outIb,
                                    int* outIndexCount, float blockX, float blockZ,
                                    float waterHeight) {
    if (!m_device || !outVb || !outIb || !outIndexCount) return false;

    constexpr int grid = kWaterGrid;
    constexpr int vertCount = grid * grid;
    constexpr int quadCount = (grid - 1) * (grid - 1);
    constexpr int indexCount = quadCount * 6;

    std::vector<WaterVertex> verts(static_cast<size_t>(vertCount));
    std::vector<uint16_t> indices(static_cast<size_t>(indexCount));

    const float step = 320.0f / static_cast<float>(grid - 1);
    int v = 0;
    for (int z = 0; z < grid; ++z) {
        for (int x = 0; x < grid; ++x) {
            const float lx = static_cast<float>(x) * step;
            const float lz = static_cast<float>(z) * step;
            WaterVertex& vert = verts[static_cast<size_t>(v++)];
            vert.Pos = Vector3(blockX + lx, waterHeight, blockZ + lz);
            vert.Normal = Vector3(0.0f, 1.0f, 0.0f);
            vert.TangentDir = Vector3(1.0f, 0.0f, 0.0f);
            vert.UV0 = Vector2(lx / 320.0f, lz / 320.0f);
            vert.UV1 = Vector2((blockX + lx) / 640.0f, (blockZ + lz) / 640.0f);
            vert.WaveScale = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    int idx = 0;
    for (int z = 0; z < grid - 1; ++z) {
        for (int x = 0; x < grid - 1; ++x) {
            const uint16_t i0 = static_cast<uint16_t>(z * grid + x);
            const uint16_t i1 = static_cast<uint16_t>(i0 + 1);
            const uint16_t i2 = static_cast<uint16_t>(i0 + grid);
            const uint16_t i3 = static_cast<uint16_t>(i2 + 1);
            indices[static_cast<size_t>(idx++)] = i0;
            indices[static_cast<size_t>(idx++)] = i2;
            indices[static_cast<size_t>(idx++)] = i1;
            indices[static_cast<size_t>(idx++)] = i1;
            indices[static_cast<size_t>(idx++)] = i2;
            indices[static_cast<size_t>(idx++)] = i3;
        }
    }

    LPDIRECT3DVERTEXBUFFER9 vb = nullptr;
    LPDIRECT3DINDEXBUFFER9 ib = nullptr;
    if (FAILED(m_device->CreateVertexBuffer(static_cast<UINT>(verts.size() * sizeof(WaterVertex)),
                                            D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &vb, nullptr))) {
        return false;
    }
    void* vbData = nullptr;
    if (SUCCEEDED(vb->Lock(0, 0, &vbData, 0))) {
        std::memcpy(vbData, verts.data(), verts.size() * sizeof(WaterVertex));
        vb->Unlock();
    }

    if (FAILED(m_device->CreateIndexBuffer(static_cast<UINT>(indices.size() * sizeof(uint16_t)),
                                           D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED,
                                           &ib, nullptr))) {
        vb->Release();
        return false;
    }
    void* ibData = nullptr;
    if (SUCCEEDED(ib->Lock(0, 0, &ibData, 0))) {
        std::memcpy(ibData, indices.data(), indices.size() * sizeof(uint16_t));
        ib->Unlock();
    }

    *outVb = vb;
    *outIb = ib;
    *outIndexCount = indexCount;
    return true;
}

void WaterRenderer::UploadConstants(const Matrix4& world, const Matrix4& view, const Matrix4& proj,
                                    const Vector3& cameraWorldPos, float timeSeconds) {
    const Matrix4 wvp = world * view * proj;
    const Matrix4 invWorld = Matrix4Inverse(world);

    float c0[4] = {0.0f, 0.5f, 1.0f, 2.0f};
    float c1[4] = {4.0f, kPi * 0.5f, kPi, kPi * 2.0f};
    float c2[4] = {1.0f, -1.0f / 6.0f, 1.0f / 120.0f, -1.0f / 5040.0f};
    float c3[4] = {1.0f / 2.0f, -1.0f / 24.0f, 1.0f / 720.0f, -1.0f / 40320.0f};
    float c4[16]{};
    float c8[4]{};
    float c9[4] = {200.0f, 400.0f, -200.0f, 1.0f};
    float c10[4] = {1.02f, 0.1f, 0.0f, 0.0f};
    float c11[4] = {80.0f, 100.0f, 5.0f, 5.0f};
    float c12[4] = {0.0f, 0.2f, 0.0f, 0.0f};
    float c13[4] = {0.2f, 0.15f, 0.4f, 0.4f};
    float c14[4] = {0.25f, 0.0f, -0.7f, -0.8f};
    float c15[4] = {0.0f, 0.15f, -0.7f, 0.1f};
    float c16[4] = {timeSeconds, sinf(timeSeconds), 0.0f, 0.0f};
    float c17[4] = {0.031f, 0.04f, -0.03f, 0.02f};
    float c18[12]{};
    float ps0[4] = {0.0f, 0.5f, 1.0f, 0.25f};
    float ps1[4] = {0.8f, 0.76f, 0.62f, 1.0f};

    MatrixToShaderConstants(wvp, c4);
    Matrix3x3ToShaderConstants(world, c18);

    const Vector3 camModel = TransformPoint(invWorld, cameraWorldPos);
    c8[0] = camModel.x;
    c8[1] = camModel.y;
    c8[2] = camModel.z;
    c8[3] = 1.0f;

    m_device->SetVertexShaderConstantF(0, c0, 1);
    m_device->SetVertexShaderConstantF(1, c1, 1);
    m_device->SetVertexShaderConstantF(2, c2, 1);
    m_device->SetVertexShaderConstantF(3, c3, 1);
    m_device->SetVertexShaderConstantF(4, c4, 4);
    m_device->SetVertexShaderConstantF(8, c8, 1);
    m_device->SetVertexShaderConstantF(9, c9, 1);
    m_device->SetVertexShaderConstantF(10, c10, 1);
    m_device->SetVertexShaderConstantF(11, c11, 1);
    m_device->SetVertexShaderConstantF(12, c12, 1);
    m_device->SetVertexShaderConstantF(13, c13, 1);
    m_device->SetVertexShaderConstantF(14, c14, 1);
    m_device->SetVertexShaderConstantF(15, c15, 1);
    m_device->SetVertexShaderConstantF(16, c16, 1);
    m_device->SetVertexShaderConstantF(17, c17, 1);
    m_device->SetVertexShaderConstantF(18, c18, 3);

    m_device->SetPixelShaderConstantF(0, ps0, 1);
    m_device->SetPixelShaderConstantF(1, ps1, 1);
}

void WaterRenderer::SaveRenderState() {
    if (!m_device || m_stateSaved) return;
    m_device->GetFVF(&m_oldFvf);
    m_device->GetVertexShader(&m_oldVs);
    m_device->GetPixelShader(&m_oldPs);
    m_device->GetVertexDeclaration(&m_oldDecl);
    m_stateSaved = true;
}

void WaterRenderer::RestoreRenderState() {
    if (!m_device || !m_stateSaved) return;
    m_device->SetVertexShader(m_oldVs);
    m_device->SetPixelShader(m_oldPs);
    m_device->SetVertexDeclaration(m_oldDecl);
    m_device->SetFVF(m_oldFvf);
    if (m_oldVs) m_oldVs->Release();
    if (m_oldPs) m_oldPs->Release();
    if (m_oldDecl) m_oldDecl->Release();
    m_oldVs = nullptr;
    m_oldPs = nullptr;
    m_oldDecl = nullptr;
    m_stateSaved = false;
}

void WaterRenderer::Draw(const Matrix4& world, const Matrix4& view, const Matrix4& proj,
                         const Vector3& cameraWorldPos, uint8_t waveType, int8_t waterType,
                         LPDIRECT3DVERTEXBUFFER9 vb, LPDIRECT3DINDEXBUFFER9 ib, int indexCount,
                         float timeSeconds) {
    if (!IsReady() || !vb || !ib || indexCount <= 0) return;

    Texture* bumpTex = GetAnimatedBumpTexture(waveType, timeSeconds);
    Texture* secondaryTex = GetSecondaryTexture(waveType);
    if (!bumpTex || !bumpTex->pTexture) return;

    SaveRenderState();

    m_device->SetVertexDeclaration(m_decl);
    m_device->SetVertexShader(m_vs);
    m_device->SetPixelShader(m_ps);

    UploadConstants(world, view, proj, cameraWorldPos, timeSeconds);

    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    if (waterType == 1) {
        m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(220, 235, 255, 210));
        m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    } else {
        m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    }
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

    m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_device->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_device->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

    m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    m_device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    m_device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    m_device->SetTexture(0, bumpTex->pTexture);
    m_device->SetTexture(1, secondaryTex && secondaryTex->pTexture ? secondaryTex->pTexture
                                                                     : bumpTex->pTexture);

    m_device->SetStreamSource(0, vb, 0, sizeof(WaterVertex));
    m_device->SetIndices(ib);
    m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, kWaterGrid * kWaterGrid, 0,
                                   indexCount / 3);

    m_device->SetTexture(1, nullptr);
    m_device->SetTexture(0, nullptr);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    RestoreRenderState();
}
