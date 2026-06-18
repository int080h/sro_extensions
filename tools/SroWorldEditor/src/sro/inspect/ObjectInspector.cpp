#include "ObjectInspector.h"

#include "assets/AssetPathResolver.h"

#include "core/FileSystem.h"

#include "core/TextDecode.h"

#include <cstdio>

#include <filesystem>



namespace {



ObjectInspectNode Leaf(const std::string& label, const std::string& value) {

    return {label, value, {}};

}



void AppendBsrSection(ObjectInspectReport& report, const BsrResource& bsr, MeshRenderer* mr) {

    ObjectInspectNode bsrNode;

    bsrNode.label = "BSR Resource";

    bsrNode.value = bsr.Signature;



    if (!bsr.SkeletonPath.empty())

        bsrNode.children.push_back(Leaf("Skeleton", bsr.SkeletonPath));

    if (!bsr.CollisionPath.empty())

        bsrNode.children.push_back(Leaf("Collision", bsr.CollisionPath));



    ObjectInspectNode meshes{"Meshes", std::to_string(bsr.MeshPaths.size()), {}};

    for (size_t i = 0; i < bsr.MeshPaths.size(); ++i)

        meshes.children.push_back(Leaf("Mesh " + std::to_string(i), bsr.MeshPaths[i]));

    bsrNode.children.push_back(std::move(meshes));



    ObjectInspectNode materials{"Materials", std::to_string(bsr.MaterialPaths.size()), {}};

    for (size_t i = 0; i < bsr.MaterialPaths.size(); ++i)

        materials.children.push_back(Leaf("Material " + std::to_string(i), bsr.MaterialPaths[i]));

    bsrNode.children.push_back(std::move(materials));



    ObjectInspectNode anims{"Animations", std::to_string(bsr.AnimationPaths.size()), {}};

    for (size_t i = 0; i < bsr.AnimationPaths.size(); ++i)

        anims.children.push_back(Leaf("Clip " + std::to_string(i), bsr.AnimationPaths[i]));

    bsrNode.children.push_back(std::move(anims));



    if (mr && !report.primaryModelPath.empty()) {

        if (auto* model = mr->PreloadModel(report.primaryModelPath)) {

            report.missingTextureCount = model->MissingTextureCount;

            char sizeBuf[128];

            const Vector3 size = model->MaxBounds - model->MinBounds;

            std::snprintf(sizeBuf, sizeof(sizeBuf), "%.1f x %.1f x %.1f", size.x, size.y, size.z);

            bsrNode.children.push_back(Leaf("AABB Size", sizeBuf));

            bsrNode.children.push_back(Leaf("Vertices", std::to_string(model->TotalVertices)));

            bsrNode.children.push_back(Leaf("Indices", std::to_string(model->TotalIndices)));

            bsrNode.children.push_back(Leaf("Submeshes", std::to_string(model->SubMeshCount)));

            if (model->MissingTextureCount > 0)

                bsrNode.children.push_back(Leaf("Missing Textures", std::to_string(model->MissingTextureCount)));



            int vertexSkinnedCount = 0;
            int rigidBoneCount = 0;
            for (const auto& sub : model->SubMeshes) {
                if (sub.HasVertexSkin) ++vertexSkinnedCount;
                if (sub.HasRigidBone) ++rigidBoneCount;
            }
            if (vertexSkinnedCount > 0)
                bsrNode.children.push_back(Leaf("Vertex-skinned Submeshes", std::to_string(vertexSkinnedCount)));
            if (rigidBoneCount > 0)
                bsrNode.children.push_back(Leaf("Rigid Bone Submeshes", std::to_string(rigidBoneCount)));
            if (!bsr.SkeletonPath.empty() && !bsr.AnimationPaths.empty() && vertexSkinnedCount == 0
                && rigidBoneCount == 0) {
                bsrNode.children.push_back(Leaf("Animation Warning",
                    "Skeleton and clips present but no skinned submeshes detected"));
            }

        }

    }



    report.sections.push_back(std::move(bsrNode));

}



} // namespace



ObjectInspectReport ObjectInspector::BuildFromBsrPath(const std::string& bsrPath, MeshRenderer* mr,

                                                      const std::wstring& clientPath) {

    ObjectInspectReport report;

    report.primaryModelPath = bsrPath;

    report.title = std::filesystem::path(bsrPath).filename().string();



    sro::AssetPathResolver resolver;

    resolver.SetClientPath(clientPath);

    BsrResource bsr;

    if (bsr.Read(resolver.ResolveRelativePath(bsrPath))) {

        report.bsr = bsr;

        report.bsrLoaded = true;

        AppendBsrSection(report, bsr, mr);

    }

    return report;

}



ObjectInspectReport ObjectInspector::BuildFromGameObject(const sro::GameObjectRef& ref, MeshRenderer* mr,

                                                         const sro::TextDataCatalog* textData) {

    ObjectInspectReport report;

    report.title = ref.codeName;

    report.primaryModelPath = textData ? textData->ResolvePrimaryModelPath(ref) : ref.PrimaryModelPath();
    if (!ref.parentCodeName.empty() && ref.PrimaryModelPath().empty() && !report.primaryModelPath.empty())
        report.modelPathSource = "via " + ref.parentCodeName;



    ObjectInspectNode identity{"Identity", ref.codeName, {}};

    identity.children.push_back(Leaf("CodeName", ref.codeName));

    identity.children.push_back(Leaf("Service ID", std::to_string(ref.serviceId)));

    if (!ref.stringKey.empty())

        identity.children.push_back(Leaf("String Key", ref.stringKey));

    if (!ref.parentCodeName.empty())

        identity.children.push_back(Leaf("Parent CodeName", ref.parentCodeName));

    if (TextDecode::IsPrintableDisplayText(ref.displayName))

        identity.children.push_back(Leaf("Display Name", ref.displayName));

    identity.children.push_back(Leaf("Source", ref.sourceFile));

    report.sections.push_back(std::move(identity));



    ObjectInspectNode models{"Model Paths", std::to_string(ref.modelPaths.size()), {}};

    for (const auto& p : ref.modelPaths)

        if (!p.empty() && p != "xxx") models.children.push_back(Leaf("Path", p));

    if (!report.primaryModelPath.empty()) {

        std::string resolved = report.primaryModelPath;

        if (!report.modelPathSource.empty()) resolved += " (" + report.modelPathSource + ")";

        models.children.push_back(Leaf("Resolved", resolved));

    }

    report.sections.push_back(std::move(models));



    if (!report.primaryModelPath.empty() && mr) {

        BsrResource bsr;

        sro::AssetPathResolver resolver;

        resolver.SetClientPath(mr->GetClientPath());

        if (bsr.Read(resolver.ResolveRelativePath(report.primaryModelPath))) {

            report.bsr = bsr;

            report.bsrLoaded = true;

            AppendBsrSection(report, bsr, mr);

        }

    }



    ObjectInspectNode raw{"TextData Columns", std::to_string(ref.rawColumns.size()), {}};

    for (size_t i = 0; i < ref.rawColumns.size(); ++i) {

        if (ref.rawColumns[i].empty() || ref.rawColumns[i] == "xxx") continue;

        raw.children.push_back(Leaf("Col " + std::to_string(i), ref.rawColumns[i]));

    }

    report.sections.push_back(std::move(raw));

    return report;

}



ObjectInspectReport ObjectInspector::BuildFromObjId(uint32_t objId, const sro::ObjectIndex& objectIndex,

                                                    MeshRenderer* mr, const std::wstring& clientPath) {

    ObjectInspectReport report;

    const std::string bsrPath = objectIndex.ResolveBsrPath(objId);

    report.title = "ObjID " + std::to_string(objId);

    report.primaryModelPath = bsrPath;

    report.sections.push_back(Leaf("ObjID", std::to_string(objId)));

    report.sections.push_back(Leaf("BSR Path", bsrPath));

    if (!bsrPath.empty()) {

        auto sub = BuildFromBsrPath(bsrPath, mr, clientPath);

        report.bsr = sub.bsr;

        report.bsrLoaded = sub.bsrLoaded;

        report.missingTextureCount = sub.missingTextureCount;

        for (auto& s : sub.sections) report.sections.push_back(std::move(s));

    }

    return report;

}

