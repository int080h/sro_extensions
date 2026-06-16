#pragma once
#include "EditorAppTypes.h"
#include "core/MathTypes.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

enum class PropertyFieldType {
    ReadOnlyText,
    Text,
    Float,
    Int,
    Bool,
    Float3,
    YawRadians,
};

struct PropertyField {
    std::string label;
    PropertyFieldType type = PropertyFieldType::ReadOnlyText;
    bool editable = false;

    std::string textValue;
    float floatValue = 0.0f;
    int intValue = 0;
    bool boolValue = false;
    Vector3 float3Value{};
};

struct MaterialEntry {
    std::string materialName;
    std::string texturePath;
    int vertexCount = 0;
    int indexCount = 0;
};

struct GeometricProperties {
    std::vector<PropertyField> fields;
    std::vector<MaterialEntry> subMeshes; // unused here; kept for builder convenience
};

struct TransformProperties {
    std::vector<PropertyField> fields;
};

struct MaterialProperties {
    std::vector<MaterialEntry> entries;
    std::vector<PropertyField> fields;
};

struct MetadataProperties {
    std::vector<PropertyField> fields;
};

struct EntityPropertySheet {
    EntityKind kind = EntityKind::WorldObject;
    std::string title;
    bool requiresApply = false;

    GeometricProperties geometry;
    TransformProperties transform;
    MaterialProperties material;
    MetadataProperties metadata;

    // Editable backing values (mutated by UI, applied on Apply)
    std::optional<Vector3> editPosition;
    std::optional<float> editYaw;
    std::optional<Vector3> editRotation;
    std::optional<Vector3> editScale;
    std::optional<Vector3> editDestPosition;
    std::optional<bool> editHidden;
    std::optional<bool> editVisible;
    std::optional<bool> editLocked;
    std::optional<bool> editCollision;
    std::optional<std::string> editName;
    std::optional<std::string> editNotes;
    std::optional<std::string> editModelPath;
};

struct EntityPropertyBuildContext {
    class MeshRenderer* meshRenderer = nullptr;
    class RenderManager* renderManager = nullptr;
    const std::vector<struct PlacementVM>* placements = nullptr;
};
