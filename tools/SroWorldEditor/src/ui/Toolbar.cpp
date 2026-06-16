#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "imgui.h"

void Ui::DrawToolbar(EditorContext& ctx, Editor& editor) {
    ImGui::Begin("##Toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);

    ImGui::SetWindowPos(ImVec2(0, ImGui::GetFrameHeight()), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 36), ImGuiCond_Always);

    if (ImGui::Button("Save")) editor.SaveProject();
    ImGui::SameLine();
    ImGui::BeginDisabled(!ctx.commandHistory.CanUndo());
    if (ImGui::Button("Undo")) ctx.commandHistory.Undo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!ctx.commandHistory.CanRedo());
    if (ImGui::Button("Redo")) ctx.commandHistory.Redo();
    ImGui::EndDisabled();
    ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();

    const char* toolNames[] = {"Sel", "Move", "Rot", "Scale"};
    for (int i = 0; i < 4; ++i) {
        bool on = static_cast<int>(ctx.activeTool) == i;
        if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.45f, 0.65f, 1));
        if (ImGui::Button(toolNames[i])) ctx.activeTool = static_cast<EditorToolType>(i);
        if (on) ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    ImGui::TextDisabled("|"); ImGui::SameLine();
    ImGui::Checkbox("Grid", &ctx.viewport.showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Bounds", &ctx.viewport.showRegionBounds);
    ImGui::SameLine();
    ImGui::Checkbox("Collision", &ctx.viewport.showCollision);
    if (ctx.sroClientLoaded) {
        ImGui::SameLine();
        ImGui::Checkbox("Event Decors", &ctx.viewport.showEventDecors);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Spawns", &ctx.viewport.showSpawns);
    ImGui::SameLine();
    ImGui::Checkbox("NPCs", &ctx.viewport.showNpcs);
    ImGui::SameLine();
    ImGui::Checkbox("TP", &ctx.viewport.showTeleports);
    ImGui::SameLine();
    if (ImGui::Button("Validate")) editor.RunValidation();
    ImGui::SameLine();
    ImGui::Checkbox("Snap Grid", &ctx.viewport.snapToGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Snap Ground", &ctx.viewport.snapToGround);

    ImGui::End();
}

void Ui::DrawStatusBar(const EditorContext& ctx) {
    ImGui::Begin("##StatusBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
    float h = 22.0f;
    ImGui::SetWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - h), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, h), ImGuiCond_Always);

    ImGui::Text("Project: %s | Region: %d | Mouse: X %.0f Y %.0f Z %.0f | Selected: %s | FPS: %.0f | %s",
        ctx.project.projectName.c_str(), ctx.activeRegionId,
        ctx.mouseWorldPos.x, ctx.mouseWorldPos.y, ctx.mouseWorldPos.z,
        ctx.selectedObjectName.empty() ? "-" : ctx.selectedObjectName.c_str(),
        ctx.fps, ctx.project.modified ? "Modified" : "Saved");
    ImGui::End();
}
