#pragma once
#include "core/EntityPropertyModel.h"
#include "EditorContext.h"

class EntityPropertyApplier {
public:
    static bool ApplyPlacement(EditorContext& ctx, const EntityPropertySheet& sheet, const SelectionId& sel);
    static bool ApplyProjectEntity(EditorContext& ctx, const EntityPropertySheet& sheet, const SelectionId& sel);
};
