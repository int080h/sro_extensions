#pragma once
#include "core/EntityPropertyModel.h"
#include "EditorContext.h"

class EntityPropertyBuilder {
public:
    static EntityPropertySheet Build(const EditorContext& ctx, const SelectionId& sel);
};
