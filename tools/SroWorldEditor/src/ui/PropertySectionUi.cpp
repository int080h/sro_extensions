#include "ui/PropertySectionUi.h"
#include <cstring>

namespace {

bool DrawField(PropertyField& field, EntityPropertySheet& sheet) {
    bool changed = false;
    ImGui::PushID(field.label.c_str());

    switch (field.type) {
    case PropertyFieldType::ReadOnlyText:
        ImGui::TextDisabled("%s", field.label.c_str());
        ImGui::SameLine(160.0f);
        ImGui::TextWrapped("%s", field.textValue.c_str());
        break;
    case PropertyFieldType::Text:
        ImGui::TextDisabled("%s", field.label.c_str());
        ImGui::SameLine(160.0f);
        {
            char buf[512];
            std::strncpy(buf, field.textValue.c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##val", buf, sizeof(buf))) {
                field.textValue = buf;
                changed = true;
                if (field.label == "Name" && sheet.editName) sheet.editName = field.textValue;
                else if (field.label == "Model Path" && sheet.editModelPath) sheet.editModelPath = field.textValue;
                else if (field.label == "Notes" && sheet.editNotes) sheet.editNotes = field.textValue;
            }
        }
        break;
    case PropertyFieldType::Float:
        ImGui::TextDisabled("%s", field.label.c_str());
        ImGui::SameLine(160.0f);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::DragFloat("##val", &field.floatValue, 0.1f)) changed = true;
        break;
    case PropertyFieldType::Int:
        ImGui::TextDisabled("%s", field.label.c_str());
        ImGui::SameLine(160.0f);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::DragInt("##val", &field.intValue, 1)) changed = true;
        break;
    case PropertyFieldType::Bool:
        if (ImGui::Checkbox(field.label.c_str(), &field.boolValue)) {
            changed = true;
            if (field.label == "Hidden" && sheet.editHidden) sheet.editHidden = field.boolValue;
            else if (field.label == "Collision" && sheet.editCollision) sheet.editCollision = field.boolValue;
            else if (field.label == "Visible" && sheet.editVisible) sheet.editVisible = field.boolValue;
            else if (field.label == "Locked" && sheet.editLocked) sheet.editLocked = field.boolValue;
        }
        break;
    case PropertyFieldType::Float3:
        ImGui::TextDisabled("%s", field.label.c_str());
        ImGui::SameLine(160.0f);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::DragFloat3("##val", &field.float3Value.x, field.label.find("Scale") != std::string::npos ? 0.01f : 1.0f)) {
            changed = true;
            if (field.label == "Position" && sheet.editPosition) sheet.editPosition = field.float3Value;
            else if (field.label == "Rotation" && sheet.editRotation) sheet.editRotation = field.float3Value;
            else if (field.label == "Scale" && sheet.editScale) sheet.editScale = field.float3Value;
            else if (field.label == "Source Position" && sheet.editPosition) sheet.editPosition = field.float3Value;
            else if (field.label == "Dest Position" && sheet.editDestPosition) sheet.editDestPosition = field.float3Value;
        }
        break;
    case PropertyFieldType::YawRadians:
        ImGui::TextDisabled("%s", field.label.c_str());
        ImGui::SameLine(160.0f);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat("##val", &field.floatValue, -3.14159265f, 3.14159265f, "%.3f rad")) {
            changed = true;
            if (sheet.editYaw) sheet.editYaw = field.floatValue;
        }
        break;
    }

    ImGui::PopID();
    return changed;
}

} // namespace

bool DrawPropertySection(const char* title, const std::vector<PropertyField>& fields, EntityPropertySheet& sheet, bool& changed) {
    if (fields.empty()) {
        if (ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("N/A");
        }
        return false;
    }

    bool sectionChanged = false;
    if (ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) {
        for (auto& field : const_cast<std::vector<PropertyField>&>(fields)) {
            if (DrawField(field, sheet)) sectionChanged = true;
        }
    }
    if (sectionChanged) changed = true;
    return sectionChanged;
}

bool DrawMaterialSection(const char* title, const MaterialProperties& material, bool& changed) {
    (void)changed;
    if (material.entries.empty() && material.fields.empty()) {
        if (ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("N/A");
        }
        return false;
    }

    if (ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) {
        int idx = 0;
        for (const auto& entry : material.entries) {
            ImGui::PushID(idx++);
            if (ImGui::TreeNode("Submesh")) {
                ImGui::TextDisabled("Material"); ImGui::SameLine(120); ImGui::Text("%s", entry.materialName.c_str());
                ImGui::TextDisabled("Texture"); ImGui::SameLine(120); ImGui::TextWrapped("%s", entry.texturePath.c_str());
                ImGui::TextDisabled("Vertices"); ImGui::SameLine(120); ImGui::Text("%d", entry.vertexCount);
                ImGui::TextDisabled("Indices"); ImGui::SameLine(120); ImGui::Text("%d", entry.indexCount);
                ImGui::TreePop();
            }
        }
        for (const auto& f : material.fields) {
            PropertyField mut = f;
            EntityPropertySheet dummy;
            DrawField(mut, dummy);
        }
    }
    return false;
}

bool DrawEntityPropertySheet(EntityPropertySheet& sheet, EntityPropertyEditState& state, const SelectionId& sel) {
    bool changed = false;

    if (!(state.lastSelection == sel)) {
        state.lastSelection = sel;
        state.hasPendingApply = false;
        if (sheet.editPosition) state.initialPosition = *sheet.editPosition;
        if (sheet.editYaw) state.initialYaw = *sheet.editYaw;
    }

    ImGui::Text("%s", sheet.title.c_str());
    ImGui::Separator();

    DrawPropertySection("Geometrical Properties", sheet.geometry.fields, sheet, changed);
    DrawPropertySection("Transformational Properties", sheet.transform.fields, sheet, changed);
    DrawMaterialSection("Material & Texture Properties", sheet.material, changed);
    DrawPropertySection("Metadata & Technical Attributes", sheet.metadata.fields, sheet, changed);

    if (changed && sheet.requiresApply)
        state.hasPendingApply = true;

    return changed;
}
