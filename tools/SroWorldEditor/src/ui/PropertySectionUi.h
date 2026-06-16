#pragma once
#include "core/EntityPropertyModel.h"
#include "imgui.h"
#include <string>

struct EntityPropertyEditState {
    SelectionId lastSelection{};
    bool hasPendingApply = false;
    Vector3 initialPosition{};
    float initialYaw = 0.0f;
};

bool DrawPropertySection(const char* title, const std::vector<PropertyField>& fields, EntityPropertySheet& sheet, bool& changed);
bool DrawMaterialSection(const char* title, const MaterialProperties& material, bool& changed);
bool DrawEntityPropertySheet(EntityPropertySheet& sheet, EntityPropertyEditState& state, const SelectionId& sel);
