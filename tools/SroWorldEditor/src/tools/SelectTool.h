#pragma once
#include "tools/EditorTool.h"

class SelectTool : public EditorTool {
public:
    EditorToolType Type() const override { return EditorToolType::Select; }
    const char* Name() const override { return "Select"; }
};

class MoveTool : public EditorTool {
public:
    EditorToolType Type() const override { return EditorToolType::Move; }
    const char* Name() const override { return "Move"; }
};

class ObjectPlacementTool : public EditorTool {
public:
    EditorToolType Type() const override { return EditorToolType::ObjectPlacement; }
    const char* Name() const override { return "Object Placement"; }
};

class SpawnPlacementTool : public EditorTool {
public:
    EditorToolType Type() const override { return EditorToolType::SpawnPlacement; }
    const char* Name() const override { return "Spawn Placement"; }
};

class ZonePaintTool : public EditorTool {
public:
    EditorToolType Type() const override { return EditorToolType::ZonePaint; }
    const char* Name() const override { return "Zone Paint"; }
};
