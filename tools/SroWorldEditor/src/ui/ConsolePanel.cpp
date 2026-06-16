#include "ui/UiCommon.h"
#include "core/Logger.h"
#include "imgui.h"

void Ui::DrawConsolePanel(EditorContext& ctx) {
    if (!ImGui::Begin("Console", &ctx.panels.console)) { ImGui::End(); return; }

    if (ImGui::Button("Clear")) Logger::Instance().Clear();
    ImGui::SameLine();
    if (ImGui::Button("Copy")) ImGui::SetClipboardText(Logger::Instance().ExportText().c_str());
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        FILE* f = fopen("console_export.log", "w");
        if (f) { fputs(Logger::Instance().ExportText().c_str(), f); fclose(f); }
    }
    ImGui::Separator();

    ImGui::BeginChild("LogScroll");
    for (const auto& e : Logger::Instance().Entries()) {
        ImVec4 col = ImVec4(0.85f, 0.85f, 0.85f, 1);
        if (e.level == LogLevel::Warning) col = ImVec4(1.0f, 0.8f, 0.2f, 1);
        if (e.level == LogLevel::Error) col = ImVec4(1.0f, 0.35f, 0.35f, 1);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(e.message.c_str());
        ImGui::PopStyleColor();
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::End();
}
