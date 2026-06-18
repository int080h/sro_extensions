#pragma once
#include "Core/Math.h"
#include "Parsers/BanParser.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

struct SkeletonBone {
    std::string Name;
    std::string ParentName;
    Vector4 RotationToParent{};
    Vector3 TranslationToParent{};
    Vector4 RotationToOrigin{};
    Vector3 TranslationToOrigin{};
    Vector4 RotationToLocal{};
    Vector3 TranslationToLocal{};
    Matrix4 LocalBindPose{};
    Matrix4 BindPose{};
    Matrix4 LocalArmaturePose{};
};

enum class FkMultiplyOrder {
    LocalThenParent = 0,
    ParentThenLocal = 1,
};

class SkeletonResource {
public:
    std::vector<SkeletonBone> Bones;
    std::map<std::string, size_t> BoneIndexByName;
    std::map<std::string, size_t> BoneIndexByLowerName;
    std::map<std::string, Matrix4> BindWorldByName;
    std::map<std::string, Matrix4> InvBindByName;

    bool LoadFromFile(const std::wstring& path);
    const SkeletonBone* FindBone(const std::string& name) const;
    const SkeletonBone* FindBoneCaseInsensitive(const std::string& name) const;

    FkMultiplyOrder GetFkMultiplyOrder() const { return m_fkMultiplyOrder; }
    void SetFkMultiplyOrder(FkMultiplyOrder order);

private:
    FkMultiplyOrder m_fkMultiplyOrder = FkMultiplyOrder::LocalThenParent;
    void RebuildBindWorldCache();
};

class SkeletonAnimator {
public:
    enum class SkinningDebugMode : int {
        BindWorldAnimInv = 0,
        BindWorldInvAnim = 1,
        BindPoseAnimInv = 2,
        BindPoseInvAnim = 3,
        BindWorldAnimInvParentFirst = 4,
        BindWorldInvAnimParentFirst = 5,
        BindPoseAnimInvParentFirst = 6,
        BindPoseInvAnimParentFirst = 7,
    };

    enum class BanLocalMode : int {
        Auto = 0,
        ReplaceAbsolute = 1,
        BindThenBan = 2,
        BanThenBind = 3,
    };

    void SetSkeleton(SkeletonResource* skeleton);
    bool SetClip(const BanResource* clip);
    void SetClipPath(const std::string& clipRelPath);
    void SetTimeMs(float timeMs);
    void SetBindOnlyFk(bool bindOnly) { m_bindOnlyFk = bindOnly; }
    void SetBanLocalMode(BanLocalMode mode) { m_banLocalMode = mode; }
    BanLocalMode GetBanLocalMode() const { return m_banLocalMode; }
    void SetSkinningDebugMode(SkinningDebugMode mode);
    SkinningDebugMode GetSkinningDebugMode() const { return m_skinningDebugMode; }

    const std::map<std::string, Matrix4>& BoneMatrices() const { return m_boneMatrices; }
    const std::map<std::string, Matrix4>& SkinMatrices() const { return m_skinMatrices; }
    float DurationMs() const { return m_durationMs; }

private:
    static Matrix4 QuaternionToMatrix(const Vector4& q);
    static Matrix4 InterpolateTransform(const BanKeyframe& a, const BanKeyframe& b, float t);
    Matrix4 ComputeBoneMatrix(const std::string& boneName, float timeMs) const;
    void RebuildBoneEvalOrder();
    void UpdateSkinMatrices();
    void SyncFkOrderToSkeleton();
    void RunFkDiagnosticsIfNeeded();
    void RunBanKf0Diagnostic();
    BanLocalMode EffectiveBanLocalMode() const;
    Matrix4 ApplyBanLocalMode(const SkeletonBone& bone, const Matrix4& banMat) const;
    static size_t ResolveBanKeyframeIndex(float timeMs, size_t keyframeCount,
        const std::vector<uint32_t>& keyframeTimesMs);
    FkMultiplyOrder CurrentFkOrder() const;
    Matrix4 ComposeFkWorld(const Matrix4& local, const Matrix4& parentWorld) const;

    SkeletonResource* m_skeleton = nullptr;
    const BanResource* m_clip = nullptr;
    std::string m_clipPath;
    SkinningDebugMode m_skinningDebugMode = SkinningDebugMode::BindWorldInvAnim;
    BanLocalMode m_banLocalMode = BanLocalMode::Auto;
    BanLocalMode m_detectedBanLocalMode = BanLocalMode::ReplaceAbsolute;
    bool m_bindOnlyFk = false;
    float m_timeMs = 0.f;
    float m_durationMs = 0.f;
    std::string m_fkDiagnosticKey;
    std::string m_banKf0DiagnosticKey;
    std::vector<size_t> m_boneEvalOrder;
    std::map<std::string, Matrix4> m_boneMatrices;
    std::map<std::string, Matrix4> m_skinMatrices;
};
