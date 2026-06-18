#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "inspect/ObjectInspector.h"
#include "index/TextDataCatalog.h"
#include "index/StringTableCatalog.h"
#include "assets/AssetPathResolver.h"
#include "rendering/MeshRenderer.h"
#include "rendering/EffectRenderer.h"
#include "rendering/SkeletonAnimator.h"
#include "ViewportFramebuffer.h"
#include "core/MathTypes.h"
#include "core/Logger.h"
#include "core/FileSystem.h"
#include "core/TextDecode.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

namespace {

struct PreviewState {
    ViewportFramebuffer framebuffer;
    SkeletonAnimator animator;
    SkeletonResource skeleton;
    std::string skeletonPath;
    std::string activeClipPath;
    std::string lastModelPath;
    std::string status;
    bool skeletonLoaded = false;
    bool clipLoaded = false;
    int clipCount = 0;
    bool frameValid = false;
    UINT lastFrameW = 0;
    UINT lastFrameH = 0;
    DWORD lastRenderTick = 0;
    std::string lastRenderModelPath;
    std::string lastRenderClipPath;
    float lastRenderAnimTimeMs = -1.0f;
    float lastPreviewYaw = 0.0f;
    float lastPreviewPitch = 0.0f;
    float lastPreviewDistance = 0.0f;
    float lastPreviewPanX = 0.0f;
    float lastPreviewPanY = 0.0f;
    bool lastPreviewWireframe = false;
    bool lastPreviewEffects = false;
    bool lastBindOnlyFk = false;
    int lastBanLocalMode = -1;
};

PreviewState g_preview;

struct ObjectViewerCatalogCache {
    const sro::TextDataCatalog* textData = nullptr;
    const sro::StringTableCatalog* strings = nullptr;
    const MeshRenderer* meshRenderer = nullptr;
    int filterTab = -1;
    std::string search;
    size_t mapEntryCount = 0;
    std::vector<const sro::GameObjectRef*> filtered;
    std::vector<std::pair<uint32_t, std::string>> mapEntries;
};

ObjectViewerCatalogCache g_catalogCache;

static sro::CatalogFilter TabToFilter(int tab) {
    switch (tab) {
    case 1: return sro::CatalogFilter::Characters;
    case 2: return sro::CatalogFilter::Items;
    case 3: return sro::CatalogFilter::Npcs;
    case 4: return sro::CatalogFilter::Monsters;
    case 5: return sro::CatalogFilter::MapObjects;
    default: return sro::CatalogFilter::All;
    }
}

static const char* FilterLabel(int tab) {
    switch (tab) {
    case 1: return "characters";
    case 2: return "items";
    case 3: return "NPCs";
    case 4: return "monsters";
    case 5: return "map objects";
    default: return "objects";
    }
}

static std::string ResolveDisplayName(const EditorContext& ctx, const sro::GameObjectRef& ref) {
    const sro::StringTableCatalog* strings = ctx.sroAssets ? &ctx.sroAssets->Strings() : nullptr;
    auto tryKey = [&](const std::string& key) -> std::string {
        if (key.empty() || !strings) return {};
        if (const auto* s = strings->FindEnglish(key)) {
            const std::string label = TextDecode::StripTranslationQuotes(*s);
            if (TextDecode::IsPrintableDisplayText(label)) return label;
        }
        return {};
    };

    if (std::string label = tryKey(ref.stringKey); !label.empty()) return label;

    if (!ref.codeName.empty() && ref.codeName.rfind("SN_", 0) != 0) {
        if (std::string label = tryKey("SN_" + ref.codeName); !label.empty()) return label;
    }

    if (std::string label = tryKey(ref.displayName); !label.empty()) return label;

    const std::string stripped = TextDecode::StripTranslationQuotes(ref.displayName);
    if (TextDecode::IsPrintableDisplayText(stripped) && stripped != ref.codeName) return stripped;
    return {};
}

static std::string EllipsizeToWidth(const std::string& text, float maxWidth) {
    if (text.empty() || maxWidth <= 0.f) return text;
    if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) return text;
    const char* ellipsis = "...";
    const float ellipsisW = ImGui::CalcTextSize(ellipsis).x;
    if (maxWidth <= ellipsisW) return ellipsis;

    int lo = 0;
    int hi = static_cast<int>(text.size());
    while (lo < hi) {
        const int mid = (lo + hi + 1) / 2;
        const std::string trial = text.substr(0, static_cast<size_t>(mid)) + ellipsis;
        if (ImGui::CalcTextSize(trial.c_str()).x <= maxWidth) lo = mid;
        else hi = mid - 1;
    }
    return text.substr(0, static_cast<size_t>(lo)) + ellipsis;
}

static void DrawTruncatedLabelValue(const char* label, const std::string& value) {
    const std::string prefix = std::string(label) + ": ";
    const float prefixW = ImGui::CalcTextSize(prefix.c_str()).x;
    const float valueMaxW = (std::max)(ImGui::GetContentRegionAvail().x - prefixW, 1.f);
    const std::string valueShown = EllipsizeToWidth(value, valueMaxW);
    const std::string line = prefix + valueShown;
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::TextUnformatted(line.c_str());
    if (valueShown.size() < value.size() && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", value.c_str());
}

static void DrawInspectLeaf(const char* label, const std::string& value) {
    DrawTruncatedLabelValue(label, value);
}

static void ResetObjectViewerPreviewCamera(ObjectViewerState& ov) {
    ov.ResetPreviewCamera();
}

static void HandlePreviewCameraInput(ObjectViewerState& ov, bool hovered) {
    if (!hovered) return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseWheel != 0.f) {
        ov.previewDistance *= powf(0.9f, io.MouseWheel);
        ov.previewDistance = std::clamp(ov.previewDistance, 0.5f, 20.f);
        io.MouseWheel = 0.f;
    }

    const ImVec2 delta = io.MouseDelta;
    const bool orbitDrag = ImGui::IsMouseDown(ImGuiMouseButton_Left) && !io.KeyShift
        || ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool panDrag = ImGui::IsMouseDown(ImGuiMouseButton_Middle)
        || (ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.KeyShift);

    if (orbitDrag && (delta.x != 0.f || delta.y != 0.f)) {
        ov.previewYaw += delta.x * 0.01f;
        ov.previewPitch += delta.y * 0.01f;
        ov.previewPitch = std::clamp(ov.previewPitch, -1.2f, 1.2f);
    } else if (panDrag && (delta.x != 0.f || delta.y != 0.f)) {
        const float panScale = ov.previewDistance * 0.002f;
        ov.previewPanX += delta.x * panScale;
        ov.previewPanY -= delta.y * panScale;
    }
}

static std::string ToLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool PathContainsToken(const std::string& path, const char* token) {
    return ToLowerCopy(path).find(token) != std::string::npos;
}

static bool NearlyEqual(float a, float b, float eps = 0.0001f) {
    return fabsf(a - b) <= eps;
}

static std::string ExtractRegionToken(const BsrResource& bsr, const std::string& modelPath) {
    static const char* kRegions[] = {"europe", "china", "arabia", "western", "turkey", "korea", "japan"};
    auto findIn = [](const std::string& path) -> std::string {
        if (path.empty()) return {};
        const std::string lower = ToLowerCopy(path);
        for (const char* region : kRegions) {
            if (lower.find(region) != std::string::npos) return region;
        }
        return {};
    };

    if (!bsr.SkeletonPath.empty()) {
        if (std::string r = findIn(bsr.SkeletonPath); !r.empty()) return r;
    }
    if (!modelPath.empty()) {
        if (std::string r = findIn(modelPath); !r.empty()) return r;
    }

    std::map<std::string, int> clipVotes;
    for (const auto& clipPath : bsr.AnimationPaths) {
        if (std::string r = findIn(clipPath); !r.empty()) ++clipVotes[r];
    }
    if (!clipVotes.empty()) {
        auto best = clipVotes.begin();
        for (auto it = clipVotes.begin(); it != clipVotes.end(); ++it) {
            if (it->second > best->second) best = it;
        }
        return best->first;
    }

    for (const auto& meshPath : bsr.MeshPaths) {
        if (std::string r = findIn(meshPath); !r.empty()) return r;
    }
    return {};
}

static bool ClipMatchesRegion(const std::string& clipPath, const std::string& region) {
    if (region.empty()) return true;
    return ToLowerCopy(clipPath).find(region) != std::string::npos;
}

static const std::string* PickFirstMatchingClip(const BsrResource& bsr, const std::string& region,
                                                bool (*pathMatch)(const std::string&)) {
    for (const auto& p : bsr.AnimationPaths) {
        if (!pathMatch(p)) continue;
        if (ClipMatchesRegion(p, region)) return &p;
    }
    return nullptr;
}

static std::string PickDefaultIdleClip(const BsrResource& bsr, const std::string& modelPath) {
    if (bsr.AnimationPaths.empty()) return {};

    const std::string region = ExtractRegionToken(bsr, modelPath);

    for (const auto& group : bsr.AniGroups) {
        const std::string gn = ToLowerCopy(group.GroupName);
        if (gn.find("idle") != std::string::npos || gn.find("stand") != std::string::npos) {
            for (uint32_t idx : group.AnimationIndices) {
                if (idx >= bsr.AnimationPaths.size() || bsr.AnimationPaths[idx].empty()) continue;
                const std::string& p = bsr.AnimationPaths[idx];
                if (ClipMatchesRegion(p, region)) return p;
            }
        }
    }

    if (const std::string* p = PickFirstMatchingClip(bsr, region, [](const std::string& path) {
            const std::string lower = ToLowerCopy(path);
            return lower.find("_idle.") != std::string::npos || lower.find("\\idle") != std::string::npos
                || lower.find("/idle") != std::string::npos || lower.find("idle_") != std::string::npos;
        })) return *p;

    if (const std::string* p = PickFirstMatchingClip(bsr, region, [](const std::string& path) {
            return PathContainsToken(path, "_stand.") || PathContainsToken(path, "\\stand")
                || PathContainsToken(path, "/stand");
        })) return *p;

    if (const std::string* p = PickFirstMatchingClip(bsr, region, [](const std::string& path) {
            return PathContainsToken(path, "_sta.") || PathContainsToken(path, "\\sta")
                || PathContainsToken(path, "_sta_");
        })) return *p;

    for (const auto& p : bsr.AnimationPaths) {
        if (ClipMatchesRegion(p, region)) return p;
    }
    return region.empty() ? bsr.AnimationPaths[0] : std::string{};
}

static int FindClipIndex(const BsrResource& bsr, const std::string& clipPath) {
    if (clipPath.empty()) return -1;
    for (size_t i = 0; i < bsr.AnimationPaths.size(); ++i) {
        if (bsr.AnimationPaths[i] == clipPath)
            return static_cast<int>(i);
    }
    return -1;
}

static bool ResolveSelectedClip(const BsrResource& bsr, const std::string& modelPath, ObjectViewerState& ov) {
    if (bsr.AnimationPaths.empty()) {
        ov.selectedAnimPath.clear();
        ov.selectedAnimIndex = -1;
        return false;
    }

    if (ov.selectedAnimIndex >= 0
        && static_cast<size_t>(ov.selectedAnimIndex) < bsr.AnimationPaths.size()
        && bsr.AnimationPaths[static_cast<size_t>(ov.selectedAnimIndex)] == ov.selectedAnimPath) {
        return true;
    }

    const int pathIndex = FindClipIndex(bsr, ov.selectedAnimPath);
    if (pathIndex >= 0) {
        ov.selectedAnimIndex = pathIndex;
        return true;
    }

    ov.selectedAnimPath = PickDefaultIdleClip(bsr, modelPath);
    ov.selectedAnimIndex = FindClipIndex(bsr, ov.selectedAnimPath);
    return ov.selectedAnimIndex >= 0;
}

void ResetPreviewCacheForModel(const std::string& modelPath, ObjectViewerState* ov) {
    if (g_preview.lastModelPath == modelPath) return;
    g_preview.lastModelPath = modelPath;
    g_preview.skeletonPath.clear();
    g_preview.activeClipPath.clear();
    g_preview.skeleton = SkeletonResource{};
    g_preview.animator.SetSkeleton(nullptr);
    g_preview.animator.SetClip(nullptr);
    g_preview.skeletonLoaded = false;
    g_preview.clipLoaded = false;
    g_preview.clipCount = 0;
    g_preview.frameValid = false;
    g_preview.lastRenderModelPath.clear();
    g_preview.lastRenderClipPath.clear();
    g_preview.lastRenderAnimTimeMs = -1.0f;
    if (ov) {
        ov->selectedAnimPath.clear();
        ov->selectedAnimIndex = -1;
        ov->animTimeMs = 0.f;
    }
}

void DrawInspectTree(const ObjectInspectNode& node) {
    if (node.children.empty()) {
        DrawInspectLeaf(node.label.c_str(), node.value);
        return;
    }
    if (ImGui::TreeNode(node.label.c_str(), "%s (%s)", node.label.c_str(), node.value.c_str())) {
        for (const auto& child : node.children)
            DrawInspectTree(child);
        ImGui::TreePop();
    }
}

void RenderObjectPreview(EditorContext& ctx, Editor& editor, float w, float h, float dt) {
    if (!ctx.sroMeshRenderer) {
        if (editor.IsClientLoading())
            g_preview.status = "Loading client data...";
        else
            g_preview.status = "Import client data to preview models.";
        return;
    }
    if (w < 4.f || h < 4.f) return;

    auto* device = editor.Viewport().GetDevice();
    if (!device) {
        g_preview.status = "D3D device unavailable.";
        return;
    }

    const std::string& modelPath = ctx.objectViewer.inspectedBsrPath;
    if (modelPath.empty()) {
        g_preview.status = "No model path for this object.";
        return;
    }

    ResetPreviewCacheForModel(modelPath, &ctx.objectViewer);

    g_preview.framebuffer.Initialize(device);
    const UINT targetW = static_cast<UINT>(w);
    const UINT targetH = static_cast<UINT>(h);
    if (!g_preview.framebuffer.EnsureSize(targetW, targetH)) {
        g_preview.status = "Failed to allocate preview framebuffer.";
        return;
    }

    const DWORD now = GetTickCount();
    const bool sameFrameInputs =
        g_preview.frameValid &&
        g_preview.lastFrameW == targetW &&
        g_preview.lastFrameH == targetH &&
        g_preview.lastRenderModelPath == modelPath &&
        g_preview.lastRenderClipPath == ctx.objectViewer.selectedAnimPath &&
        NearlyEqual(g_preview.lastRenderAnimTimeMs, ctx.objectViewer.animTimeMs) &&
        NearlyEqual(g_preview.lastPreviewYaw, ctx.objectViewer.previewYaw) &&
        NearlyEqual(g_preview.lastPreviewPitch, ctx.objectViewer.previewPitch) &&
        NearlyEqual(g_preview.lastPreviewDistance, ctx.objectViewer.previewDistance) &&
        NearlyEqual(g_preview.lastPreviewPanX, ctx.objectViewer.previewPanX) &&
        NearlyEqual(g_preview.lastPreviewPanY, ctx.objectViewer.previewPanY) &&
        g_preview.lastPreviewWireframe == ctx.objectViewer.previewWireframe &&
        g_preview.lastPreviewEffects == ctx.objectViewer.previewEffects &&
        g_preview.lastBindOnlyFk == ctx.objectViewer.bindOnlyFk &&
        g_preview.lastBanLocalMode == ctx.objectViewer.banLocalMode;

    if (sameFrameInputs) {
        if (!ctx.objectViewer.animPlaying && !ctx.objectViewer.previewEffects) {
            return;
        }
        if (!ctx.objectViewer.animPlaying && ctx.objectViewer.previewEffects &&
            now - g_preview.lastRenderTick < 50) {
            return;
        }
    }

    if (!g_preview.framebuffer.BeginRender()) {
        g_preview.status = "Failed to begin preview render.";
        return;
    }

    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(28, 32, 40, 255), 1.f, 0);

    auto* mr = ctx.sroMeshRenderer;
    auto* model = mr->PreloadModel(modelPath);
    if (!model) {
        sro::AssetPathResolver resolver;
        resolver.SetClientPath(mr->GetClientPath());
        const std::wstring resolved = resolver.ResolveRelativePath(modelPath);
        g_preview.status = "Failed to load mesh: " + modelPath;
        Logger::Instance().Warning("Object Viewer preview failed: " + modelPath
            + " resolved=" + ToNarrow(resolved) + " exists=" + (FileExists(resolved) ? "yes" : "no"));
        g_preview.framebuffer.EndRender();
        D3DVIEWPORT9 vp{};
        vp.Width = (DWORD)ImGui::GetIO().DisplaySize.x;
        vp.Height = (DWORD)ImGui::GetIO().DisplaySize.y;
        vp.MinZ = 0.f;
        vp.MaxZ = 1.f;
        device->SetViewport(&vp);
        return;
    }

    if (!model->SkeletonPath.empty() && model->SkeletonPath != g_preview.skeletonPath) {
        g_preview.skeletonLoaded = false;
        if (auto* skel = mr->LoadSkeleton(model->SkeletonPath, modelPath)) {
            g_preview.skeleton = *skel;
            g_preview.skeletonPath = model->SkeletonPath;
            g_preview.animator.SetSkeleton(&g_preview.skeleton);
            g_preview.skeletonLoaded = true;
        } else {
            Logger::Instance().Warning("Object Viewer: skeleton load failed: " + model->SkeletonPath);
        }
    }

    const BsrResource* bsr = mr->GetBsrResource(modelPath);
    g_preview.clipCount = bsr ? static_cast<int>(bsr->AnimationPaths.size()) : 0;
    g_preview.clipLoaded = false;
    if (bsr && !bsr->AnimationPaths.empty()) {
        if (!ResolveSelectedClip(*bsr, modelPath, ctx.objectViewer)) {
            ctx.objectViewer.animTimeMs = 0.f;
            g_preview.activeClipPath.clear();
        }
        if (ctx.objectViewer.selectedAnimPath != g_preview.activeClipPath) {
            if (auto* clip = mr->LoadBanClip(ctx.objectViewer.selectedAnimPath, modelPath)) {
                g_preview.animator.SetClip(clip);
                g_preview.activeClipPath = ctx.objectViewer.selectedAnimPath;
                g_preview.clipLoaded = true;
            } else {
                Logger::Instance().Warning("Object Viewer: clip load failed: " + ctx.objectViewer.selectedAnimPath);
            }
        } else {
            g_preview.clipLoaded = g_preview.animator.DurationMs() > 0.f;
        }
    }

    if (ctx.objectViewer.animPlaying && g_preview.animator.DurationMs() > 0.f) {
        ctx.objectViewer.animTimeMs += dt * 1000.f * ctx.objectViewer.animSpeed;
        if (ctx.objectViewer.animTimeMs > g_preview.animator.DurationMs())
            ctx.objectViewer.animTimeMs = 0.f;
    }
    g_preview.animator.SetClipPath(ctx.objectViewer.selectedAnimPath);
    g_preview.animator.SetBindOnlyFk(ctx.objectViewer.bindOnlyFk);
    {
        const int mode = (std::max)(0, (std::min)(3, ctx.objectViewer.banLocalMode));
        g_preview.animator.SetBanLocalMode(static_cast<SkeletonAnimator::BanLocalMode>(mode));
    }
    g_preview.animator.SetSkinningDebugMode(SkeletonAnimator::SkinningDebugMode::BindWorldInvAnim);
    g_preview.animator.SetTimeMs(ctx.objectViewer.animTimeMs);

    Vector3 modelCenter = (model->MinBounds + model->MaxBounds) * 0.5f;
    const float yaw = ctx.objectViewer.previewYaw;
    Vector3 panRight(cosf(yaw), 0.f, -sinf(yaw));
    Vector3 center = modelCenter + panRight * ctx.objectViewer.previewPanX
        + Vector3(0.f, ctx.objectViewer.previewPanY, 0.f);
    Vector3 size = model->MaxBounds - model->MinBounds;
    float radius = (std::max)({size.x, size.y, size.z, 1.f}) * 0.6f;
    float dist = ctx.objectViewer.previewDistance * radius;

    float cy = cosf(ctx.objectViewer.previewPitch);
    Vector3 eye(
        center.x + sinf(ctx.objectViewer.previewYaw) * cy * dist,
        center.y + sinf(ctx.objectViewer.previewPitch) * dist + radius * 0.3f,
        center.z + cosf(ctx.objectViewer.previewYaw) * cy * dist);

    Matrix4 view = MatrixLookAtLH(eye, center, Vector3(0, 1, 0));
    float aspect = w / h;
    Matrix4 proj = MatrixPerspectiveFovLH(0.785398f, aspect, 0.1f, dist * 10.f);

    const bool useAnim = g_preview.clipLoaded && g_preview.skeletonLoaded
        && g_preview.activeClipPath == ctx.objectViewer.selectedAnimPath
        && g_preview.animator.DurationMs() > 0.f;

    mr->BeginBatch(view, proj, 0, ctx.objectViewer.previewWireframe, true);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    if (useAnim) {
        mr->DrawModelAnimated(modelPath, Vector3(0, 0, 0), 0.f, ctx.objectViewer.previewWireframe,
            &g_preview.animator, &g_preview.skeleton, view, proj);
    } else {
        mr->DrawModel(modelPath, Vector3(0, 0, 0), 0.f, false, false, view, proj);
    }
    mr->EndBatch();

    if (ctx.sroEffectRenderer && ctx.objectViewer.previewEffects && (!model->EffectPaths.empty() || !model->ParticleAttachments.empty())) {
        std::vector<EffectRenderer::EffectDrawRequest> effectRequests;
        if (!model->ParticleAttachments.empty()) {
            effectRequests.reserve(model->ParticleAttachments.size());
            for (const auto& attachment : model->ParticleAttachments) {
                EffectRenderer::EffectDrawRequest req;
                req.Pos = Vector3(0, 0, 0);
                req.Path = attachment.Path;
                req.LocalAnchorOffset = attachment.Position;
                req.LocalAnchorRotation = attachment.Rotation;
                effectRequests.push_back(req);
            }
        } else {
            Vector3 effectAnchor(0.0f, 0.0f, 0.0f);
            if (model->MaxBounds.y > model->MinBounds.y && model->MinBounds.x < model->MaxBounds.x) {
                const float height = model->MaxBounds.y - model->MinBounds.y;
                effectAnchor.x = (model->MinBounds.x + model->MaxBounds.x) * 0.5f;
                effectAnchor.y = model->MinBounds.y + height * 0.72f;
                effectAnchor.z = (model->MinBounds.z + model->MaxBounds.z) * 0.5f;
            }
            effectRequests.reserve(model->EffectPaths.size());
            for (const std::string& effectPath : model->EffectPaths) {
                effectRequests.push_back({Vector3(0, 0, 0), effectPath, 0.f, false, effectAnchor});
            }
        }
        ctx.sroEffectRenderer->DrawPlacementEffects(effectRequests, eye, view, proj);
    }

    g_preview.status = "Loaded (" + std::to_string(model->TotalVertices) + " verts, "
        + std::to_string(g_preview.clipCount) + " clips";
    if (!model->EffectPaths.empty()) {
        g_preview.status += ", " + std::to_string(model->EffectPaths.size()) + " effects";
    }
    if (!model->SkeletonPath.empty())
        g_preview.status += g_preview.skeletonLoaded ? ", skeleton OK" : ", skeleton FAILED";
    if (!ctx.objectViewer.selectedAnimPath.empty())
        g_preview.status += g_preview.clipLoaded ? ", clip OK" : ", clip FAILED";
    if (g_preview.animator.DurationMs() > 0.f)
        g_preview.status += ", " + std::to_string(static_cast<int>(g_preview.animator.DurationMs())) + " ms";
    g_preview.status += ")";

    g_preview.framebuffer.EndRender();
    g_preview.frameValid = true;
    g_preview.lastFrameW = targetW;
    g_preview.lastFrameH = targetH;
    g_preview.lastRenderTick = now;
    g_preview.lastRenderModelPath = modelPath;
    g_preview.lastRenderClipPath = ctx.objectViewer.selectedAnimPath;
    g_preview.lastRenderAnimTimeMs = ctx.objectViewer.animTimeMs;
    g_preview.lastPreviewYaw = ctx.objectViewer.previewYaw;
    g_preview.lastPreviewPitch = ctx.objectViewer.previewPitch;
    g_preview.lastPreviewDistance = ctx.objectViewer.previewDistance;
    g_preview.lastPreviewPanX = ctx.objectViewer.previewPanX;
    g_preview.lastPreviewPanY = ctx.objectViewer.previewPanY;
    g_preview.lastPreviewWireframe = ctx.objectViewer.previewWireframe;
    g_preview.lastPreviewEffects = ctx.objectViewer.previewEffects;
    g_preview.lastBindOnlyFk = ctx.objectViewer.bindOnlyFk;
    g_preview.lastBanLocalMode = ctx.objectViewer.banLocalMode;

    D3DVIEWPORT9 vp{};
    vp.Width = (DWORD)ImGui::GetIO().DisplaySize.x;
    vp.Height = (DWORD)ImGui::GetIO().DisplaySize.y;
    vp.MinZ = 0.f;
    vp.MaxZ = 1.f;
    device->SetViewport(&vp);
}

static bool DragVSplitter(const char* id, float* ratio, float totalHeight) {
    ImGui::InvisibleButton(id, ImVec2(-1.f, 4.f));
    if (ImGui::IsItemActive() && totalHeight > 1.f) {
        *ratio += ImGui::GetIO().MouseDelta.y / totalHeight;
        *ratio = std::clamp(*ratio, 0.15f, 0.85f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    return ImGui::IsItemActive();
}

static bool DragHSplitter(const char* id, float* ratio, float totalWidth, float height) {
    ImGui::SameLine(0.f, 0.f);
    ImGui::InvisibleButton(id, ImVec2(4.f, height));
    if (ImGui::IsItemActive() && totalWidth > 1.f) {
        *ratio += ImGui::GetIO().MouseDelta.x / totalWidth;
        *ratio = std::clamp(*ratio, 0.20f, 0.80f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    return ImGui::IsItemActive();
}

static const char* ClientStatusLabel(const EditorContext& ctx, const Editor& editor) {
    if (editor.IsClientLoading()) return "Loading...";
    if (ctx.sroClientLoaded && ctx.sroTextData) return "Ready";
    if (!ctx.project.clientPath.empty()) return "Not loaded";
    return "No client";
}

static void DrawCatalogList(EditorContext& ctx, Editor& editor, int filterTab, const char* search) {
    if (editor.IsClientLoading()) {
        ImGui::TextDisabled("Loading client data...");
        return;
    }

    if (filterTab == 5) {
        ImGui::BeginChild("MapObjList", ImVec2(0, 0), true);
        if (!ctx.sroMeshRenderer) {
            ImGui::TextDisabled("Import client data first.");
        } else {
            const auto& mapping = ctx.sroMeshRenderer->GetObjectMapping();
            if (g_catalogCache.meshRenderer != ctx.sroMeshRenderer ||
                g_catalogCache.mapEntryCount != mapping.size()) {
                g_catalogCache.meshRenderer = ctx.sroMeshRenderer;
                g_catalogCache.mapEntryCount = mapping.size();
                g_catalogCache.mapEntries.assign(mapping.begin(), mapping.end());
                std::sort(g_catalogCache.mapEntries.begin(), g_catalogCache.mapEntries.end(),
                    [](const auto& a, const auto& b) { return a.first < b.first; });
            }

            int shown = 0;
            for (const auto& [objId, path] : g_catalogCache.mapEntries) {
                if (search[0]) {
                    std::string q = search;
                    std::string idStr = std::to_string(objId);
                    if (path.find(q) == std::string::npos && idStr.find(q) == std::string::npos) continue;
                }
                char label[256];
                std::snprintf(label, sizeof(label), "%u  %s", objId, path.c_str());
                if (ImGui::Selectable(label, ctx.objectViewer.inspectedObjId == objId)) {
                    ctx.InspectObjId(objId);
                }
                ++shown;
            }
            ImGui::TextDisabled("Showing %d map objects", shown);
        }
        ImGui::EndChild();
        return;
    }

    if (!ctx.sroTextData) {
        ImGui::TextDisabled("Import client data to browse game objects.");
        return;
    }

    const sro::StringTableCatalog* strings = ctx.sroAssets ? &ctx.sroAssets->Strings() : nullptr;
    if (g_catalogCache.textData != ctx.sroTextData ||
        g_catalogCache.strings != strings ||
        g_catalogCache.filterTab != filterTab ||
        g_catalogCache.search != search) {
        g_catalogCache.textData = ctx.sroTextData;
        g_catalogCache.strings = strings;
        g_catalogCache.filterTab = filterTab;
        g_catalogCache.search = search;
        g_catalogCache.filtered.clear();
        ctx.sroTextData->CollectForFilter(TabToFilter(filterTab), search, g_catalogCache.filtered, strings);
    }
    const auto& filtered = g_catalogCache.filtered;

    if (filtered.empty()) {
        ImGui::TextDisabled(search[0] ? "No matching objects." : "No objects in this category.");
        return;
    }

    ImGui::TextDisabled("Showing %d %s", (int)filtered.size(), FilterLabel(filterTab));
    ImGui::BeginChild("CatalogList", ImVec2(0, 0), true);

    ImGuiListClipper clipper;
    clipper.Begin((int)filtered.size());
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const auto* ref = filtered[row];
            const bool selected = ctx.objectViewer.source == ObjectInspectSource::GameObject
                && ctx.objectViewer.inspectedCodeName == ref->codeName;
            const std::string display = ResolveDisplayName(ctx, *ref);
            char label[320];
            if (!display.empty() && display != ref->codeName) {
                std::snprintf(label, sizeof(label), "%s  (%s)##%d",
                    ref->codeName.c_str(), display.c_str(), ref->serviceId);
            } else {
                std::snprintf(label, sizeof(label), "%s##%d", ref->codeName.c_str(), ref->serviceId);
            }
            if (ImGui::Selectable(label, selected)) {
                ctx.InspectGameObject(ref->codeName);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", ctx.sroTextData->ResolvePrimaryModelPath(*ref).c_str());
            }
        }
    }
    ImGui::EndChild();
}

} // namespace

void Ui::DrawObjectViewerPanel(EditorContext& ctx, Editor& editor) {
    if (!ImGui::Begin("Object Viewer", &ctx.panels.objectViewer)) { ImGui::End(); return; }

    ImGui::PushItemWidth(-1.f);
    char searchBuf[128] = {};
    if (!ctx.objectViewer.catalogSearch.empty()) {
        std::snprintf(searchBuf, sizeof(searchBuf), "%s", ctx.objectViewer.catalogSearch.c_str());
    }
    if (ImGui::InputTextWithHint("##ov_search", "Search CodeName, model path, ObjID...", searchBuf, sizeof(searchBuf))) {
        ctx.objectViewer.catalogSearch = searchBuf;
    }
    ImGui::PopItemWidth();

    ImGui::Checkbox("Follow Selection", &ctx.objectViewer.followSelection);
    ImGui::SameLine();
    const char* status = ClientStatusLabel(ctx, editor);
    if (editor.IsClientLoading())
        ImGui::TextColored(ImVec4(1.f, 0.85f, 0.3f, 1.f), "[%s]", status);
    else if (ctx.sroClientLoaded && ctx.sroTextData)
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.f), "[%s]", status);
    else
        ImGui::TextDisabled("[%s]", status);

    const char* tabs[] = {"All", "Characters", "Items", "NPCs", "Monsters", "Map (object.ifo)"};
    if (ImGui::BeginTabBar("ObjectViewerTabs")) {
        for (int i = 0; i < 6; ++i) {
            ImGuiTabItemFlags flags = 0;
            if (ctx.objectViewer.filterTabRestorePending && ctx.objectViewer.filterTab == i)
                flags = ImGuiTabItemFlags_SetSelected;
            if (ImGui::BeginTabItem(tabs[i], nullptr, flags)) {
                ctx.objectViewer.filterTab = i;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
        ctx.objectViewer.filterTabRestorePending = false;
    }

    const ImVec2 mainAvail = ImGui::GetContentRegionAvail();
    const float mainW = (std::max)(mainAvail.x, 1.f);
    const float mainH = (std::max)(mainAvail.y, 120.f);
    const float leftW = mainW * ctx.objectViewer.splitCatalogPreview - 2.f;
    const float previewW = mainW - leftW - 4.f;

    ImGui::BeginChild("OV_LeftColumn", ImVec2(leftW, mainH), false, ImGuiWindowFlags_NoScrollbar);
    const float catalogH = (mainH - 4.f) * ctx.objectViewer.splitCatalogInspect;
    ImGui::BeginChild("CatalogPane", ImVec2(0, catalogH), true);
    DrawCatalogList(ctx, editor, ctx.objectViewer.filterTab, ctx.objectViewer.catalogSearch.c_str());
    ImGui::EndChild();

    DragVSplitter("OV_VSplit", &ctx.objectViewer.splitCatalogInspect, mainH);

    ImGui::BeginChild("InspectPane", ImVec2(0, 0), true);
    ObjectInspectReport report;
    if (ctx.objectViewer.source == ObjectInspectSource::GameObject && ctx.sroTextData) {
        if (const auto* ref = ctx.sroTextData->FindByCodeName(ctx.objectViewer.inspectedCodeName))
            report = ObjectInspector::BuildFromGameObject(*ref, ctx.sroMeshRenderer, ctx.sroTextData);
    } else if (ctx.objectViewer.source == ObjectInspectSource::BsrPath) {
        report = ObjectInspector::BuildFromBsrPath(ctx.objectViewer.inspectedBsrPath, ctx.sroMeshRenderer,
            ctx.sroMeshRenderer ? ctx.sroMeshRenderer->GetClientPath() : L"");
    } else if (ctx.objectViewer.source == ObjectInspectSource::ObjId && ctx.sroAssets) {
        report = ObjectInspector::BuildFromObjId(ctx.objectViewer.inspectedObjId, ctx.sroAssets->Objects(),
            ctx.sroMeshRenderer, ctx.sroMeshRenderer ? ctx.sroMeshRenderer->GetClientPath() : L"");
    }

    if (report.title.empty()) {
        ImGui::TextDisabled("Select an object to inspect.");
    } else if (ImGui::CollapsingHeader("Details", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("%s", report.title.c_str());
        if (!report.primaryModelPath.empty()) {
            if (!report.modelPathSource.empty())
                DrawTruncatedLabelValue("Model",
                    report.primaryModelPath + " (" + report.modelPathSource + ")");
            else
                DrawTruncatedLabelValue("Model", report.primaryModelPath);
        }
        if (report.missingTextureCount > 0)
            ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "%d submesh(es) missing textures", report.missingTextureCount);
        for (const auto& section : report.sections)
            DrawInspectTree(section);
    }

    if (report.bsrLoaded && !report.bsr.AnimationPaths.empty()) {
        if (ImGui::CollapsingHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Clips: %d", (int)report.bsr.AnimationPaths.size());
            if (g_preview.animator.DurationMs() > 0.f)
                ImGui::SameLine();
            if (g_preview.animator.DurationMs() > 0.f)
                ImGui::TextDisabled("| %.0f ms", g_preview.animator.DurationMs());
            if (!report.primaryModelPath.empty() && !g_preview.skeletonLoaded && !report.bsr.SkeletonPath.empty())
                ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Skeleton load failed");
            if (!ctx.objectViewer.selectedAnimPath.empty() && !g_preview.clipLoaded)
                ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Clip load failed");
            std::string selectedLabel = ctx.objectViewer.selectedAnimPath;
            if (ctx.objectViewer.selectedAnimIndex >= 0) {
                selectedLabel = "[" + std::to_string(ctx.objectViewer.selectedAnimIndex) + "] "
                    + selectedLabel;
            }
            const std::string clipPreview = EllipsizeToWidth(
                selectedLabel,
                (std::max)(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Clip").x - 40.f, 40.f));
            if (ImGui::BeginCombo("Clip", clipPreview.c_str())) {
                for (size_t i = 0; i < report.bsr.AnimationPaths.size(); ++i) {
                    const auto& p = report.bsr.AnimationPaths[i];
                    const bool selected = ctx.objectViewer.selectedAnimIndex == static_cast<int>(i)
                        && ctx.objectViewer.selectedAnimPath == p;
                    const std::string label = "[" + std::to_string(i) + "] " + p;
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        ctx.objectViewer.selectedAnimPath = p;
                        ctx.objectViewer.selectedAnimIndex = static_cast<int>(i);
                        ctx.objectViewer.animTimeMs = 0.f;
                        g_preview.activeClipPath.clear();
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", p.c_str());
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            } else if (!ctx.objectViewer.selectedAnimPath.empty() && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", ctx.objectViewer.selectedAnimPath.c_str());
            }

            const char* playLabel = ctx.objectViewer.animPlaying ? "Pause" : "Play";
            if (ImGui::Button(playLabel)) ctx.objectViewer.animPlaying = !ctx.objectViewer.animPlaying;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.f);
            ImGui::SliderFloat("Speed", &ctx.objectViewer.animSpeed, 0.1f, 3.f, "%.1fx");
            const float dur = g_preview.animator.DurationMs();
            if (dur > 0.f) {
                ImGui::SetNextItemWidth(-1.f);
                ImGui::SliderFloat("Timeline", &ctx.objectViewer.animTimeMs, 0.f, dur, "%.0f ms");
            }

            const char* banLocalLabels[] = {
                "Auto",
                "Replace (absolute)",
                "Bind * BAN",
                "BAN * Bind",
            };
            ImGui::SetNextItemWidth(-1.f);
            ImGui::Combo("BAN local", &ctx.objectViewer.banLocalMode, banLocalLabels, 4);
            ImGui::Checkbox("Bind-only FK", &ctx.objectViewer.bindOnlyFk);
        }
    }
    ImGui::EndChild();
    ImGui::EndChild();

    DragHSplitter("OV_HSplit", &ctx.objectViewer.splitCatalogPreview, mainW, mainH);

    ImGui::SameLine(0.f, 0.f);
    ImGui::BeginChild("OV_PreviewColumn", ImVec2(previewW, mainH), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::Checkbox("Wireframe", &ctx.objectViewer.previewWireframe);
    ImGui::SameLine();
    ImGui::Checkbox("Effects", &ctx.objectViewer.previewEffects);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.f);
    ImGui::SliderFloat("Zoom", &ctx.objectViewer.previewDistance, 0.5f, 20.f, "%.1f");

    ImVec2 previewSize = ImGui::GetContentRegionAvail();
    if (previewSize.y < 120.f) previewSize.y = 120.f;

    ImVec2 previewPos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("PreviewNav", previewSize);
    const bool previewHovered = ImGui::IsItemHovered();
    HandlePreviewCameraInput(ctx.objectViewer, previewHovered);

    const float previewDt = ImGui::GetIO().DeltaTime;
    RenderObjectPreview(ctx, editor, previewSize.x, previewSize.y, previewDt);

    ImGui::SetCursorScreenPos(previewPos);
    if (auto* tex = g_preview.framebuffer.ColorTexture()) {
        ImGui::Image((ImTextureID)(intptr_t)tex, previewSize, ImVec2(0, 0), ImVec2(1, 1));
    } else {
        ImGui::Dummy(previewSize);
    }

    if (!g_preview.status.empty()) {
        ImGui::SetCursorScreenPos(ImVec2(previewPos.x + 6.f, previewPos.y + previewSize.y - 20.f));
        ImGui::TextDisabled("%s", g_preview.status.c_str());
    }
    ImGui::EndChild();

    ImGui::End();
}

void Ui::DrawAssetBrowserPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Asset Browser", &ctx.panels.assetBrowser)) { ImGui::End(); return; }
    ImGui::TextWrapped("Quick catalog browser. Use Object Viewer for full inspection and 3D preview.");
    if (ImGui::Button("Open Object Viewer"))
        ctx.panels.objectViewer = true;
    ImGui::Separator();
    static char search[128] = "";
    ImGui::InputTextWithHint("##ab_search", "Search...", search, sizeof(search));
    static int tab = 0;
    const char* tabs[] = {"All", "NPCs", "Monsters", "Items"};
    if (ImGui::BeginTabBar("AssetTabs")) {
        for (int i = 0; i < 4; ++i) {
            if (ImGui::BeginTabItem(tabs[i])) {
                tab = i;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    if (!ctx.sroTextData) {
        ImGui::TextDisabled("Import client data first.");
        ImGui::End();
        return;
    }

    sro::CatalogFilter filter = sro::CatalogFilter::All;
    if (tab == 1) filter = sro::CatalogFilter::Npcs;
    else if (tab == 2) filter = sro::CatalogFilter::Monsters;
    else if (tab == 3) filter = sro::CatalogFilter::Items;

    std::vector<const sro::GameObjectRef*> items;
    const sro::StringTableCatalog* strings = ctx.sroAssets ? &ctx.sroAssets->Strings() : nullptr;
    ctx.sroTextData->CollectForFilter(filter, search, items, strings);

    ImGui::TextDisabled("Showing %d", (int)items.size());
    ImGui::BeginChild("AssetList");
    ImGuiListClipper clipper;
    clipper.Begin((int)items.size());
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const auto* ref = items[row];
            const std::string display = ResolveDisplayName(ctx, *ref);
            char label[320];
            if (!display.empty() && display != ref->codeName) {
                std::snprintf(label, sizeof(label), "%s  (%s)", ref->codeName.c_str(), display.c_str());
            } else {
                std::snprintf(label, sizeof(label), "%s", ref->codeName.c_str());
            }
            if (ImGui::Selectable(label)) {
                ctx.InspectGameObject(ref->codeName);
            }
        }
    }
    ImGui::EndChild();
    ImGui::End();
}
