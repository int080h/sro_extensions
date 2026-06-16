#pragma once
#include <d3d9.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Rendering/Texture.h"
#include "Core/Math.h"
#include "Rendering/Camera.h"
#include "Parsers/BmsParser.h"
#include "Parsers/BmtParser.h"
#include "Parsers/BsrParser.h"
#include "Parsers/CpdParser.h"

// ============================================================================
// 3D Model Mesh Renderer — loads and draws BSR/BMS object models
// ============================================================================

class MeshRenderer {
public:
    struct Vertex {
        Vector3 Pos;
        Vector3 Normal;
        Vector2 UV;
        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
    };

    struct SubMesh {
        LPDIRECT3DVERTEXBUFFER9 Vb = nullptr;
        LPDIRECT3DINDEXBUFFER9 Ib = nullptr;
        int IndexCount = 0;
        int VertexCount = 0;
        std::string MaterialName;
        std::wstring TexturePath;
    };

    struct ModelResource {
        std::vector<SubMesh> SubMeshes;
        Vector3 MinBounds = Vector3(99999.0f, 99999.0f, 99999.0f);
        Vector3 MaxBounds = Vector3(-99999.0f, -99999.0f, -99999.0f);
        int SubMeshCount = 0;
        int TotalVertices = 0;
        int TotalIndices = 0;
    };

private:
    LPDIRECT3DDEVICE9 m_device;
    std::wstring m_clientPath;
    std::map<uint32_t, std::string> m_objectMapping;
    std::map<std::string, std::unique_ptr<ModelResource>> m_modelCache;
    TextureManager* m_texManager;
    Texture* m_defaultTexture = nullptr;
    bool m_batchActive = false;
    bool m_batchWireframe = false;

    void LoadObjectIfo();
    ModelResource* LoadBsrModelResource(const std::string& bsrRelPath);
    ModelResource* LoadCompoundModelResource(const std::string& cpdRelPath);
    static void MergeModelResource(ModelResource& dst, const ModelResource& src);

public:
    MeshRenderer(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath, TextureManager* texMgr);
    ~MeshRenderer();

    const std::map<uint32_t, std::string>& GetObjectMapping() const { return m_objectMapping; }
    std::string GetModelBsrPath(uint32_t objId);
    ModelResource* PreloadModel(const std::string& bsrRelPath);

    // Batch rendering: call BeginBatch once, draw all models, then EndBatch
    void BeginBatch(const Matrix4& view, const Matrix4& proj, int timeOfDay = 0, bool wireframe = false);
    void DrawModel(const std::string& bsrRelPath, const Vector3& pos, float yaw, bool isSelected, bool isHidden, const Matrix4& view, const Matrix4& proj);
    void EndBatch();
};
