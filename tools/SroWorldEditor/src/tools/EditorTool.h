#pragma once
#include "EditorAppTypes.h"

class EditorTool {
public:
    virtual ~EditorTool() = default;
    virtual EditorToolType Type() const = 0;
    virtual const char* Name() const = 0;
    virtual void OnActivate(class EditorContext&) {}
    virtual void OnDeactivate(class EditorContext&) {}
};
