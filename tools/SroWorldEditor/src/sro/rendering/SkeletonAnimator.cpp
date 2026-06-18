#include "SkeletonAnimator.h"

#include "core/Logger.h"
#include "Parsers/ParserHelpers.h"

#include <fstream>

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <vector>

#include <cmath>



namespace {



Matrix4 BuildBoneMatrix(const Vector4& rot, const Vector3& trans) {

    Matrix4 m = Matrix4::Identity();

    float xx = rot.x * rot.x, yy = rot.y * rot.y, zz = rot.z * rot.z;

    float xy = rot.x * rot.y, xz = rot.x * rot.z, yz = rot.y * rot.z;

    float wx = rot.w * rot.x, wy = rot.w * rot.y, wz = rot.w * rot.z;



    m.m[0][0] = 1.0f - 2.0f * (yy + zz);

    m.m[0][1] = 2.0f * (xy + wz);

    m.m[0][2] = 2.0f * (xz - wy);

    m.m[1][0] = 2.0f * (xy - wz);

    m.m[1][1] = 1.0f - 2.0f * (xx + zz);

    m.m[1][2] = 2.0f * (yz + wx);

    m.m[2][0] = 2.0f * (xz + wy);

    m.m[2][1] = 2.0f * (yz - wx);

    m.m[2][2] = 1.0f - 2.0f * (xx + yy);

    m.m[3][0] = trans.x;

    m.m[3][1] = trans.y;

    m.m[3][2] = trans.z;

    return m;

}



Vector4 NormalizeQuat(Vector4 q) {

    const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);

    if (len > 1e-6f) {

        q.x /= len;

        q.y /= len;

        q.z /= len;

        q.w /= len;

    }

    return q;

}



Vector4 SlerpQuat(Vector4 a, Vector4 b, float t) {

    a = NormalizeQuat(a);

    b = NormalizeQuat(b);

    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    if (dot < 0.f) {

        b = {-b.x, -b.y, -b.z, -b.w};

        dot = -dot;

    }

    if (dot > 0.9995f) {

        Vector4 r = {

            a.x + t * (b.x - a.x),

            a.y + t * (b.y - a.y),

            a.z + t * (b.z - a.z),

            a.w + t * (b.w - a.w),

        };

        return NormalizeQuat(r);

    }

    const float theta0 = std::acosf((std::min)(1.f, (std::max)(-1.f, dot)));

    const float theta = theta0 * t;

    const float sinTheta = std::sinf(theta);

    const float sinTheta0 = std::sinf(theta0);

    const float s0 = std::cosf(theta) - dot * sinTheta / sinTheta0;

    const float s1 = sinTheta / sinTheta0;

    return NormalizeQuat({

        s0 * a.x + s1 * b.x,

        s0 * a.y + s1 * b.y,

        s0 * a.z + s1 * b.z,

        s0 * a.w + s1 * b.w,

    });

}



Matrix4 MultiplyBoneMatrices(const Matrix4& a, const Matrix4& b) {

    Matrix4 out{};

    for (int r = 0; r < 4; ++r) {

        for (int c = 0; c < 4; ++c) {

            out.m[r][c] = a.m[r][0] * b.m[0][c] + a.m[r][1] * b.m[1][c]

                + a.m[r][2] * b.m[2][c] + a.m[r][3] * b.m[3][c];

        }

    }

    return out;

}

std::string BoneNameKey(const std::string& name) {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return key;
}

float MaxSkinMatrixDeviation(const Matrix4& m) {
    float maxDev = 0.f;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            const float expected = (r == c) ? 1.f : 0.f;
            maxDev = (std::max)(maxDev, std::fabs(m.m[r][c] - expected));
        }
    }
    return maxDev;
}

float MaxMatrixPairDeviation(const Matrix4& a, const Matrix4& b) {
    float maxDev = 0.f;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            maxDev = (std::max)(maxDev, std::fabs(a.m[r][c] - b.m[r][c]));
        }
    }
    return maxDev;
}

const Matrix4* BindWorldForBone(const SkeletonResource& skeleton, const std::string& boneName) {
    auto it = skeleton.BindWorldByName.find(boneName);
    return (it != skeleton.BindWorldByName.end()) ? &it->second : nullptr;
}

Matrix4 ComposeFkWorld(const Matrix4& local, const Matrix4& parentWorld, FkMultiplyOrder order) {
    return order == FkMultiplyOrder::ParentThenLocal
        ? MultiplyBoneMatrices(parentWorld, local)
        : MultiplyBoneMatrices(local, parentWorld);
}

std::string ToLowerPath(const std::string& path) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower;
}

bool IsBindRestClipPath(const std::string& clipPath) {
    if (clipPath.empty()) return false;
    const std::string lower = ToLowerPath(clipPath);
    return lower.find("idle") != std::string::npos || lower.find("stand") != std::string::npos;
}

bool IsParentFirstSkinningMode(SkeletonAnimator::SkinningDebugMode mode) {
    return static_cast<int>(mode) >= 4;
}

} // namespace



bool SkeletonResource::LoadFromFile(const std::wstring& path) {

    std::ifstream fs(path, std::ios::binary);

    if (!fs.is_open()) return false;



    char sig[13] = {0};

    fs.read(sig, 12);

    if (std::string(sig).rfind("JMXVBSK", 0) != 0) return false;



    uint32_t boneCount = 0;

    ReadVal(fs, boneCount);

    if (boneCount == 0 || boneCount >= 10000) return false;



    Bones.clear();

    BoneIndexByName.clear();
    BoneIndexByLowerName.clear();

    Bones.reserve(boneCount);



    for (uint32_t i = 0; i < boneCount; ++i) {

        SkeletonBone bone;

        uint8_t type = 0;

        ReadVal(fs, type);

        (void)type;



        uint32_t nameLen = 0;

        ReadVal(fs, nameLen);

        bone.Name = ReadString(fs, nameLen);



        uint32_t parentLen = 0;

        ReadVal(fs, parentLen);

        bone.ParentName = ReadString(fs, parentLen);



        ReadVal(fs, bone.RotationToParent.x);

        ReadVal(fs, bone.RotationToParent.y);

        ReadVal(fs, bone.RotationToParent.z);

        ReadVal(fs, bone.RotationToParent.w);



        ReadVal(fs, bone.TranslationToParent.x);

        ReadVal(fs, bone.TranslationToParent.y);

        ReadVal(fs, bone.TranslationToParent.z);



        ReadVal(fs, bone.RotationToOrigin.x);

        ReadVal(fs, bone.RotationToOrigin.y);

        ReadVal(fs, bone.RotationToOrigin.z);

        ReadVal(fs, bone.RotationToOrigin.w);



        ReadVal(fs, bone.TranslationToOrigin.x);

        ReadVal(fs, bone.TranslationToOrigin.y);

        ReadVal(fs, bone.TranslationToOrigin.z);



        ReadVal(fs, bone.RotationToLocal.x);
        ReadVal(fs, bone.RotationToLocal.y);
        ReadVal(fs, bone.RotationToLocal.z);
        ReadVal(fs, bone.RotationToLocal.w);

        ReadVal(fs, bone.TranslationToLocal.x);
        ReadVal(fs, bone.TranslationToLocal.y);
        ReadVal(fs, bone.TranslationToLocal.z);



        uint32_t childCount = 0;

        ReadVal(fs, childCount);

        for (uint32_t c = 0; c < childCount; ++c) {

            uint32_t childNameLen = 0;

            ReadVal(fs, childNameLen);

            fs.seekg(childNameLen, std::ios::cur);

        }



        bone.LocalBindPose = BuildBoneMatrix(bone.RotationToParent, bone.TranslationToParent);
        bone.BindPose = BuildBoneMatrix(bone.RotationToOrigin, bone.TranslationToOrigin);
        bone.LocalArmaturePose = BuildBoneMatrix(bone.RotationToLocal, bone.TranslationToLocal);
        BoneIndexByName[bone.Name] = Bones.size();
        BoneIndexByLowerName[BoneNameKey(bone.Name)] = Bones.size();

        Bones.push_back(std::move(bone));

    }

    RebuildBindWorldCache();

    float maxBindSpaceDev = 0.f;
    for (const auto& bone : Bones) {
        if (const Matrix4* bindWorld = BindWorldForBone(*this, bone.Name)) {
            maxBindSpaceDev = (std::max)(maxBindSpaceDev, MaxMatrixPairDeviation(*bindWorld, bone.BindPose));
        }
    }
    Logger::Instance().Info(
        "SkeletonAnimator: loaded " + std::to_string(Bones.size()) + " bones; max BindWorld vs BindPose deviation="
        + std::to_string(maxBindSpaceDev));

    return !Bones.empty();
}

void SkeletonResource::SetFkMultiplyOrder(FkMultiplyOrder order) {
    if (m_fkMultiplyOrder == order) return;
    m_fkMultiplyOrder = order;
    RebuildBindWorldCache();
}

void SkeletonResource::RebuildBindWorldCache() {
    BindWorldByName.clear();
    InvBindByName.clear();
    if (Bones.empty()) return;

    const size_t n = Bones.size();
    std::vector<size_t> order;
    std::vector<bool> done(n, false);
    while (order.size() < n) {
        bool progressed = false;
        for (size_t i = 0; i < n; ++i) {
            if (done[i]) continue;
            const auto& bone = Bones[i];
            if (bone.ParentName.empty()) {
                order.push_back(i);
                done[i] = true;
                progressed = true;
                continue;
            }
            const SkeletonBone* parent = FindBoneCaseInsensitive(bone.ParentName);
            if (parent) {
                auto pit = BoneIndexByName.find(parent->Name);
                if (pit != BoneIndexByName.end() && done[pit->second]) {
                    order.push_back(i);
                    done[i] = true;
                    progressed = true;
                }
            }
        }
        if (!progressed) break;
    }
    for (size_t i = 0; i < n; ++i) {
        if (!done[i]) order.push_back(i);
    }

    for (size_t boneIdx : order) {
        const auto& bone = Bones[boneIdx];
        Matrix4 bindWorld;
        if (bone.ParentName.empty()) {
            bindWorld = bone.LocalBindPose;
        } else {
            const Matrix4 parentWorld = [&]() {
                if (const SkeletonBone* parent = FindBoneCaseInsensitive(bone.ParentName)) {
                    auto pit = BindWorldByName.find(parent->Name);
                    if (pit != BindWorldByName.end()) return pit->second;
                }
                return Matrix4::Identity();
            }();
            bindWorld = ComposeFkWorld(bone.LocalBindPose, parentWorld, m_fkMultiplyOrder);
        }
        BindWorldByName[bone.Name] = bindWorld;
        InvBindByName[bone.Name] = Matrix4Inverse(bindWorld);
    }
}



const SkeletonBone* SkeletonResource::FindBone(const std::string& name) const {

    auto it = BoneIndexByName.find(name);

    if (it == BoneIndexByName.end()) return nullptr;

    return &Bones[it->second];

}

const SkeletonBone* SkeletonResource::FindBoneCaseInsensitive(const std::string& name) const {
    if (const SkeletonBone* bone = FindBone(name)) return bone;
    auto it = BoneIndexByLowerName.find(BoneNameKey(name));
    if (it == BoneIndexByLowerName.end()) return nullptr;
    return &Bones[it->second];
}



void SkeletonAnimator::RebuildBoneEvalOrder() {
    m_boneEvalOrder.clear();
    if (!m_skeleton) return;

    const size_t n = m_skeleton->Bones.size();
    std::vector<bool> done(n, false);
    while (m_boneEvalOrder.size() < n) {
        bool progressed = false;
        for (size_t i = 0; i < n; ++i) {
            if (done[i]) continue;
            const auto& bone = m_skeleton->Bones[i];
            if (bone.ParentName.empty()) {
                m_boneEvalOrder.push_back(i);
                done[i] = true;
                progressed = true;
                continue;
            }
            if (const SkeletonBone* parent = m_skeleton->FindBoneCaseInsensitive(bone.ParentName)) {
                auto pit = m_skeleton->BoneIndexByName.find(parent->Name);
                if (pit != m_skeleton->BoneIndexByName.end() && done[pit->second]) {
                    m_boneEvalOrder.push_back(i);
                    done[i] = true;
                    progressed = true;
                }
            }
        }
        if (!progressed) break;
    }
    for (size_t i = 0; i < n; ++i) {
        if (!done[i]) m_boneEvalOrder.push_back(i);
    }
}

void SkeletonAnimator::UpdateSkinMatrices() {
    m_skinMatrices.clear();
    if (!m_skeleton) return;

    const bool checkRestPose = (!m_clip || IsBindRestClipPath(m_clipPath)) && m_timeMs <= 0.5f;
    float maxRestDeviation = 0.f;
    for (const auto& bone : m_skeleton->Bones) {
        const Matrix4 bindWorld = [&]() {
            if (const Matrix4* bw = BindWorldForBone(*m_skeleton, bone.Name)) return *bw;
            return bone.LocalBindPose;
        }();
        auto animIt = m_boneMatrices.find(bone.Name);
        const Matrix4 animWorld = (animIt != m_boneMatrices.end()) ? animIt->second : bindWorld;

        const int modeIdx = static_cast<int>(m_skinningDebugMode);
        const bool useBindPoseInv = (modeIdx % 4) >= 2;
        const bool invFirst = (modeIdx % 4) == 1 || (modeIdx % 4) == 3;

        Matrix4 invBind;
        if (useBindPoseInv) {
            invBind = Matrix4Inverse(bone.BindPose);
        } else {
            auto invIt = m_skeleton->InvBindByName.find(bone.Name);
            invBind = (invIt != m_skeleton->InvBindByName.end())
                ? invIt->second : Matrix4Inverse(bindWorld);
        }

        const Matrix4 skinMat = invFirst
            ? MultiplyBoneMatrices(invBind, animWorld)
            : MultiplyBoneMatrices(animWorld, invBind);

        m_skinMatrices[bone.Name] = skinMat;
        m_skinMatrices[BoneNameKey(bone.Name)] = skinMat;
        if (checkRestPose) {
            maxRestDeviation = (std::max)(maxRestDeviation, MaxSkinMatrixDeviation(skinMat));
        }
    }

    if (checkRestPose && maxRestDeviation > 0.01f) {
        static std::unordered_set<std::string> loggedRestWarnings;
        const std::string warnKey = m_clipPath + "|" + std::to_string(static_cast<int>(m_skinningDebugMode))
            + "|" + std::to_string(static_cast<int>(CurrentFkOrder()));
        if (loggedRestWarnings.insert(warnKey).second) {
            Logger::Instance().Warning(
                "SkeletonAnimator: idle bind-rest skin matrices deviate from identity (max="
                + std::to_string(maxRestDeviation) + ", clip=" + m_clipPath
                + ", timeMs=" + std::to_string(m_timeMs) + ")");
        }
    }
}

FkMultiplyOrder SkeletonAnimator::CurrentFkOrder() const {
    return IsParentFirstSkinningMode(m_skinningDebugMode)
        ? FkMultiplyOrder::ParentThenLocal
        : FkMultiplyOrder::LocalThenParent;
}

Matrix4 SkeletonAnimator::ComposeFkWorld(const Matrix4& local, const Matrix4& parentWorld) const {
    return ::ComposeFkWorld(local, parentWorld, CurrentFkOrder());
}

void SkeletonAnimator::SyncFkOrderToSkeleton() {
    if (!m_skeleton) return;
    m_skeleton->SetFkMultiplyOrder(CurrentFkOrder());
}

void SkeletonAnimator::SetSkinningDebugMode(SkinningDebugMode mode) {
    m_skinningDebugMode = mode;
    SyncFkOrderToSkeleton();
}

void SkeletonAnimator::RunFkDiagnosticsIfNeeded() {
    if (!m_skeleton || !m_clip || !IsBindRestClipPath(m_clipPath) || m_timeMs > 0.5f) return;

    const std::string diagKey = m_clipPath + "|" + std::to_string(static_cast<int>(m_skinningDebugMode))
        + "|bindOnly=" + (m_bindOnlyFk ? "1" : "0");
    if (m_fkDiagnosticKey == diagKey) return;
    m_fkDiagnosticKey = diagKey;

    struct BoneDeviation {
        std::string name;
        float deviation = 0.f;
    };
    std::vector<BoneDeviation> deviations;
    deviations.reserve(m_skeleton->Bones.size());
    for (const auto& bone : m_skeleton->Bones) {
        auto animIt = m_boneMatrices.find(bone.Name);
        if (animIt == m_boneMatrices.end()) continue;
        const Matrix4* bindWorld = BindWorldForBone(*m_skeleton, bone.Name);
        if (!bindWorld) continue;
        deviations.push_back({bone.Name, MaxMatrixPairDeviation(animIt->second, *bindWorld)});
    }
    std::sort(deviations.begin(), deviations.end(),
        [](const BoneDeviation& a, const BoneDeviation& b) { return a.deviation > b.deviation; });

    const size_t reportCount = (std::min)(deviations.size(), size_t{5});
    std::string msg = "SkeletonAnimator FK diagnostic (idle @ " + std::to_string(m_timeMs) + " ms): top "
        + std::to_string(reportCount) + " animWorld vs BindWorld deviations";
    for (size_t i = 0; i < reportCount; ++i) {
        msg += "; " + deviations[i].name + "=" + std::to_string(deviations[i].deviation);
    }
    Logger::Instance().Info(msg);
}

SkeletonAnimator::BanLocalMode SkeletonAnimator::EffectiveBanLocalMode() const {
    return m_banLocalMode == BanLocalMode::Auto ? BanLocalMode::ReplaceAbsolute : m_banLocalMode;
}

Matrix4 SkeletonAnimator::ApplyBanLocalMode(const SkeletonBone& bone, const Matrix4& banMat) const {
    switch (EffectiveBanLocalMode()) {
    case BanLocalMode::ReplaceAbsolute:
        return banMat;
    case BanLocalMode::BanThenBind:
        return MultiplyBoneMatrices(banMat, bone.LocalBindPose);
    case BanLocalMode::BindThenBan:
    default:
        return MultiplyBoneMatrices(bone.LocalBindPose, banMat);
    }
}

size_t SkeletonAnimator::ResolveBanKeyframeIndex(float timeMs, size_t keyframeCount,
    const std::vector<uint32_t>& keyframeTimesMs) {
    if (keyframeCount == 0) return 0;
    if (keyframeTimesMs.empty()) return 0;

    const float t = timeMs;
    if (t <= static_cast<float>(keyframeTimesMs.front())) return 0;
    if (t >= static_cast<float>(keyframeTimesMs.back())) return keyframeCount - 1;

    const size_t segmentCount = (std::min)(keyframeTimesMs.size(), keyframeCount);
    for (size_t i = 0; i + 1 < segmentCount; ++i) {
        if (t >= static_cast<float>(keyframeTimesMs[i]) && t <= static_cast<float>(keyframeTimesMs[i + 1]))
            return i;
    }
    return keyframeCount - 1;
}

void SkeletonAnimator::RunBanKf0Diagnostic() {
    if (!m_skeleton || !m_clip || m_clip->BoneTracks.empty()) return;

    const std::string key = m_clipPath + "|" + std::to_string(m_skeleton->Bones.size());
    if (m_banKf0DiagnosticKey == key) return;
    m_banKf0DiagnosticKey = key;

    int closerToIdentity = 0;
    int closerToBind = 0;
    float maxDevBind = 0.f;
    float maxDevIdentity = 0.f;
    int tracked = 0;

    for (const auto& track : m_clip->BoneTracks) {
        if (track.Keyframes.empty()) continue;
        const SkeletonBone* bone = m_skeleton->FindBoneCaseInsensitive(track.BoneName);
        if (!bone) continue;

        const Matrix4 banMat = BuildBoneMatrix(
            track.Keyframes[0].Rotation, track.Keyframes[0].Translation);
        const float devBind = MaxMatrixPairDeviation(banMat, bone->LocalBindPose);
        const float devIdentity = MaxMatrixPairDeviation(banMat, Matrix4::Identity());
        maxDevBind = (std::max)(maxDevBind, devBind);
        maxDevIdentity = (std::max)(maxDevIdentity, devIdentity);
        if (devIdentity < devBind) ++closerToIdentity;
        else ++closerToBind;
        ++tracked;
    }

    Logger::Instance().Info(
        "SkeletonAnimator: BAN kf0 diagnostic clip=" + m_clipPath
        + " tracks=" + std::to_string(tracked)
        + " closerToIdentity=" + std::to_string(closerToIdentity)
        + " closerToBind=" + std::to_string(closerToBind)
        + " maxDevIdentity=" + std::to_string(maxDevIdentity)
        + " maxDevBind=" + std::to_string(maxDevBind)
        + " autoMode=ReplaceAbsolute");
}

void SkeletonAnimator::SetSkeleton(SkeletonResource* skeleton) {
    m_skeleton = skeleton;
    m_boneMatrices.clear();
    m_skinMatrices.clear();
    m_fkDiagnosticKey.clear();
    m_banKf0DiagnosticKey.clear();
    RebuildBoneEvalOrder();
    SyncFkOrderToSkeleton();
}



void SkeletonAnimator::SetClipPath(const std::string& clipRelPath) {
    if (m_clipPath != clipRelPath) {
        m_clipPath = clipRelPath;
        m_fkDiagnosticKey.clear();
        m_banKf0DiagnosticKey.clear();
    }
}

bool SkeletonAnimator::SetClip(const BanResource* clip) {

    m_clip = clip;

    m_durationMs = clip ? static_cast<float>(clip->DurationMs) : 0.f;
    m_fkDiagnosticKey.clear();
    m_banKf0DiagnosticKey.clear();

    return clip != nullptr;

}



Matrix4 SkeletonAnimator::QuaternionToMatrix(const Vector4& q) {

    return BuildBoneMatrix(q, Vector3{});

}



Matrix4 SkeletonAnimator::InterpolateTransform(const BanKeyframe& a, const BanKeyframe& b, float t) {

    const Vector4 rot = SlerpQuat(a.Rotation, b.Rotation, t);

    Vector3 trans = {

        a.Translation.x + (b.Translation.x - a.Translation.x) * t,

        a.Translation.y + (b.Translation.y - a.Translation.y) * t,

        a.Translation.z + (b.Translation.z - a.Translation.z) * t,

    };

    return BuildBoneMatrix(rot, trans);

}



void SkeletonAnimator::SetTimeMs(float timeMs) {

    if (m_clip && m_durationMs > 0.f)
        timeMs = (std::max)(0.f, (std::min)(timeMs, m_durationMs));
    m_timeMs = timeMs;

    m_boneMatrices.clear();

    if (!m_skeleton) return;



    if (!m_clip) {
        for (const auto& bone : m_skeleton->Bones) {
            if (const Matrix4* bindWorld = BindWorldForBone(*m_skeleton, bone.Name)) {
                m_boneMatrices[bone.Name] = *bindWorld;
            } else {
                m_boneMatrices[bone.Name] = bone.LocalBindPose;
            }
        }
        UpdateSkinMatrices();
        return;
    }



    std::map<std::string, Matrix4> localMatrices;

    for (const auto& bone : m_skeleton->Bones)
        localMatrices[bone.Name] = bone.LocalBindPose;



    static std::unordered_set<std::string> warnedOrphanBanTracks;

    if (!m_bindOnlyFk) {
        RunBanKf0Diagnostic();

        for (const auto& track : m_clip->BoneTracks) {

            if (track.Keyframes.empty()) continue;

            const SkeletonBone* skelBone = m_skeleton->FindBoneCaseInsensitive(track.BoneName);
            if (!skelBone) {
                if (warnedOrphanBanTracks.insert(track.BoneName).second) {
                    Logger::Instance().Warning(
                        "SkeletonAnimator: BAN track bone not found in skeleton: " + track.BoneName);
                }
                continue;
            }
            const std::string& trackBone = skelBone->Name;

            const size_t idx = ResolveBanKeyframeIndex(
                timeMs, track.Keyframes.size(), m_clip->KeyframeTimesMs);

            Matrix4 banMat;
            if (idx + 1 < track.Keyframes.size() && !m_clip->KeyframeTimesMs.empty()) {
                const float t0 = static_cast<float>(
                    m_clip->KeyframeTimesMs[(std::min)(idx, m_clip->KeyframeTimesMs.size() - 1)]);
                const float t1 = (idx + 1 < m_clip->KeyframeTimesMs.size())
                    ? static_cast<float>(m_clip->KeyframeTimesMs[idx + 1]) : t0 + 1.f;
                float alpha = 0.f;
                if (timeMs <= t0) {
                    alpha = 0.f;
                } else if (timeMs >= t1) {
                    alpha = 1.f;
                } else if (t1 > t0) {
                    alpha = (timeMs - t0) / (t1 - t0);
                }
                banMat = InterpolateTransform(track.Keyframes[idx], track.Keyframes[idx + 1], alpha);
            } else {
                banMat = BuildBoneMatrix(
                    track.Keyframes[idx].Rotation, track.Keyframes[idx].Translation);
            }

            localMatrices[trackBone] = ApplyBanLocalMode(*skelBone, banMat);
        }
    }



    for (size_t boneIdx : m_boneEvalOrder) {
        const auto& bone = m_skeleton->Bones[boneIdx];
        const Matrix4 local = localMatrices[bone.Name];
        if (bone.ParentName.empty()) {
            m_boneMatrices[bone.Name] = local;
        } else {
            const Matrix4 parentWorld = [&]() {
                if (const SkeletonBone* parent = m_skeleton->FindBoneCaseInsensitive(bone.ParentName)) {
                    auto pit = m_boneMatrices.find(parent->Name);
                    if (pit != m_boneMatrices.end()) return pit->second;
                }
                return Matrix4::Identity();
            }();
            m_boneMatrices[bone.Name] = ComposeFkWorld(local, parentWorld);
        }
    }

    RunFkDiagnosticsIfNeeded();
    UpdateSkinMatrices();
}



Matrix4 SkeletonAnimator::ComputeBoneMatrix(const std::string& boneName, float timeMs) const {

    (void)timeMs;

    auto it = m_boneMatrices.find(boneName);

    if (it != m_boneMatrices.end()) return it->second;

    if (const auto* bone = m_skeleton ? m_skeleton->FindBoneCaseInsensitive(boneName) : nullptr) {
        if (const Matrix4* bindWorld = BindWorldForBone(*m_skeleton, bone->Name))
            return *bindWorld;
        return bone->LocalBindPose;
    }

    return Matrix4::Identity();

}

