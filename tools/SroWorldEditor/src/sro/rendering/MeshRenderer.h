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
#include "Parsers/BanParser.h"
#include "Parsers/CpdParser.h"
#include "Rendering/SkeletonAnimator.h"

class SkeletonAnimator;

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
        std::string BoneName;
        std::wstring TexturePath;
        std::vector<Vertex> BindVertices;
        std::vector<std::string> SkinBoneNames;
        std::vector<BmsVertexSkin> SkinWeights;
        Matrix4 BindBoneMatrix = Matrix4::Identity();
        bool HasVertexSkin = false;
        bool HasRigidBone = false;
        bool RigidBaked = false;
        bool Skinned = false; // legacy: true when HasVertexSkin || HasRigidBone
        bool IsAlphaBlend = false;
        bool IsWaterEffect = false;
        std::string MeshSourcePath;
    };

    struct ModelResource {
        std::vector<SubMesh> SubMeshes;
        Vector3 MinBounds = Vector3(99999.0f, 99999.0f, 99999.0f);
        Vector3 MaxBounds = Vector3(-99999.0f, -99999.0f, -99999.0f);
        int SubMeshCount = 0;
        int TotalVertices = 0;
        int TotalIndices = 0;
        int MissingTextureCount = 0;
        std::string SkeletonPath;
        std::string SourceBsrPath;
        std::vector<std::string> EffectPaths;
        std::vector<BsrParticleAttachment> ParticleAttachments;
    };

private:
    LPDIRECT3DDEVICE9 m_device;
    std::wstring m_clientPath;
    std::map<uint32_t, std::string> m_objectMapping;
    std::map<std::string, std::unique_ptr<ModelResource>> m_modelCache;
    std::map<std::string, std::unique_ptr<BsrResource>> m_bsrCache;
    std::map<std::string, std::unique_ptr<BmsMesh>> m_collisionMeshCache;
    std::map<std::string, std::unique_ptr<SkeletonResource>> m_skeletonCache;
    std::map<std::string, std::unique_ptr<BanResource>> m_banCache;
    TextureManager* m_texManager;
    Texture* m_defaultTexture = nullptr;
    bool m_batchActive = false;
    bool m_batchWireframe = false;
    bool m_batchDoubleSided = false;

    void LoadObjectIfo();
    std::wstring ResolveAssetPath(const std::string& relPath, const std::string& baseRelPath = {}) const;
    static std::string AssetCacheKey(const std::string& relPath, const std::string& baseRelPath);
    ModelResource* LoadBsrModelResource(const std::string& bsrRelPath);
    ModelResource* LoadCompoundModelResource(const std::string& cpdRelPath);
    static void MergeModelResource(ModelResource& dst, const ModelResource& src);
    void UploadSubMeshVertices(SubMesh& sub, const Matrix4* boneMatrix);
    void UploadSkinnedSubMeshVertices(SubMesh& sub, const std::map<std::string, Matrix4>& skinMats);
    void UploadRigidSubMeshVertices(SubMesh& sub, const Matrix4& skinMat);
    std::wstring ResolveTexturePath(const std::string& texRelPath, const std::wstring& mtrlDir,
                                    const std::string& mtrlRelPath, const std::string& bsrRelPath) const;
    static std::wstring FindCaseInsensitiveFile(const std::wstring& dir, const std::wstring& fileName);
    static bool IsWaterSubMesh(const std::string& materialName, const std::wstring& texturePath,
                               const std::string& meshSourcePath);
    static bool IsAlphaBlendHintSubMesh(const std::string& materialName, const std::wstring& texturePath,
                                        const std::string& meshSourcePath);

public:
    MeshRenderer(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath, TextureManager* texMgr);
    ~MeshRenderer();

    const std::wstring& GetClientPath() const { return m_clientPath; }
    const std::map<uint32_t, std::string>& GetObjectMapping() const { return m_objectMapping; }
    std::string GetModelBsrPath(uint32_t objId);
    ModelResource* PreloadModel(const std::string& bsrRelPath);
    const BsrResource* GetBsrResource(const std::string& bsrRelPath);
    // Loads the per-object RTNavMeshObj (BMS NavMeshOffset) following the real game
    // resource chain: BSR.CollisionPath -> .bms (or CPD.collisionResourcePath -> .bms).
    // Returns a non-owning pointer into the cache (valid for the session). When
    // outResolvedPath is non-null it receives the resolved collision .bms path.
    const BmsNavMesh* LoadCollisionNavMesh(const std::string& bsrRelPath, std::string* outResolvedPath = nullptr);
    // Resolves and caches a collision .bms by explicit path (relative to a base resource).
    const BmsNavMesh* LoadCollisionNavMeshByPath(const std::string& meshRelPath, const std::string& baseRelPath);
    SkeletonResource* LoadSkeleton(const std::string& skelRelPath, const std::string& baseRelPath = {});
    BanResource* LoadBanClip(const std::string& banRelPath, const std::string& baseRelPath = {});

    void BeginBatch(const Matrix4& view, const Matrix4& proj, int timeOfDay = 0, bool wireframe = false,
                    bool doubleSided = false);
    void DrawModel(const std::string& bsrRelPath, const Vector3& pos, float yaw, bool isSelected, bool isHidden,
                   const Matrix4& view, const Matrix4& proj, ModelResource* preloaded = nullptr);
    void DrawModelAnimated(const std::string& bsrRelPath, const Vector3& pos, float yaw, bool wireframe,
                           const SkeletonAnimator* animator, const SkeletonResource* skeleton,
                           const Matrix4& view, const Matrix4& proj);
    void EndBatch();
};
