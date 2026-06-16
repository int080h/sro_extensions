#include "core/EditorSession.h"
#include "EditorContext.h"
#include "EditorViewport.h"
#include "EditorCamera.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <shlobj.h>

using json = nlohmann::json;

std::wstring EditorSession::SessionFilePath() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
        return L"";
    std::wstring base(path);
    CoTaskMemFree(path);
    std::filesystem::path dir = std::filesystem::path(base) / L"SroWorldEditor";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return (dir / L"session.json").wstring();
}

bool EditorSession::Load(EditorSession& out) {
    const std::wstring path = SessionFilePath();
    if (path.empty() || !FileExists(path))
        return false;

    std::ifstream in(path);
    if (!in) return false;

    json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }

    out.clientPath = j.value("clientPath", "");
    out.sroRegionRx = j.value("sroRegionRx", 97);
    out.sroRegionRy = j.value("sroRegionRy", 167);
    out.cameraYaw = j.value("cameraYaw", -90.0f);
    out.cameraPitch = j.value("cameraPitch", -30.0f);
    out.cameraSpeed = j.value("cameraSpeed", 250.0f);
    out.valid = j.value("valid", false);
    if (j.contains("cameraPosition")) {
        out.cameraPosition.x = j["cameraPosition"].value("x", 960.0f);
        out.cameraPosition.y = j["cameraPosition"].value("y", 150.0f);
        out.cameraPosition.z = j["cameraPosition"].value("z", 960.0f);
    }
    return true;
}

bool EditorSession::Save(const EditorSession& session) {
    const std::wstring path = SessionFilePath();
    if (path.empty()) return false;

    json j;
    j["clientPath"] = session.clientPath;
    j["sroRegionRx"] = session.sroRegionRx;
    j["sroRegionRy"] = session.sroRegionRy;
    j["cameraYaw"] = session.cameraYaw;
    j["cameraPitch"] = session.cameraPitch;
    j["cameraSpeed"] = session.cameraSpeed;
    j["valid"] = session.valid;
    j["cameraPosition"] = {{"x", session.cameraPosition.x}, {"y", session.cameraPosition.y}, {"z", session.cameraPosition.z}};

    std::ofstream out(path);
    if (!out) return false;
    out << j.dump(2);
    return true;
}

void EditorSession::Capture(const EditorSession& src, EditorSession& dst) {
    dst = src;
}

void EditorSession::CaptureFrom(EditorContext& ctx, EditorViewport& viewport, EditorSession& out) {
    out.clientPath = ctx.project.clientPath;
    out.sroRegionRx = ctx.sroRegionRx;
    out.sroRegionRy = ctx.sroRegionRy;
    out.cameraPosition = viewport.GetCamera().Position;
    out.cameraYaw = viewport.GetCamera().Yaw;
    out.cameraPitch = viewport.GetCamera().Pitch;
    out.cameraSpeed = ctx.viewport.cameraSpeed;
    out.valid = ctx.sroClientLoaded && !out.clientPath.empty();
}
