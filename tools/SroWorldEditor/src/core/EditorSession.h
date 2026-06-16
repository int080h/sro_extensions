#pragma once
#include "core/MathTypes.h"
#include <string>

class EditorContext;
class EditorViewport;

struct EditorSession {
    std::string clientPath;
    int sroRegionRx = 97;
    int sroRegionRy = 167;
    Vector3 cameraPosition{960.0f, 150.0f, 960.0f};
    float cameraYaw = -90.0f;
    float cameraPitch = -30.0f;
    float cameraSpeed = 250.0f;
    bool valid = false;

    static std::wstring SessionFilePath();
    static bool Load(EditorSession& out);
    static bool Save(const EditorSession& session);

    static void Capture(const EditorSession& src, EditorSession& dst);
    static void CaptureFrom(EditorContext& ctx, EditorViewport& viewport, EditorSession& out);
};
