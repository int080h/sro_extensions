#include "ui/UiCommon.h"
#include "imgui.h"

void Ui::DrawValidationPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Validation", &ctx.panels.validation)) { ImGui::End(); return; }

    static int severityFilter = 0;
    ImGui::Combo("Severity", &severityFilter, "All\0Errors\0Warnings\0");
    static int regionFilter = -1;
    ImGui::InputInt("Region Filter (-1=all)", &regionFilter);

    if (ImGui::Button("Run Validation")) ctx.RefreshValidation();
    ImGui::Separator();

    ImGui::BeginChild("ValidationList");
    for (const auto& m : ctx.validationMessages) {
        if (severityFilter == 1 && m.severity != ValidationSeverity::Error) continue;
        if (severityFilter == 2 && m.severity != ValidationSeverity::Warning) continue;
        if (regionFilter >= 0 && m.regionId != regionFilter) continue;
        ImVec4 col = ImVec4(0.7f, 0.7f, 0.7f, 1);
        if (m.severity == ValidationSeverity::Error) col = ImVec4(1, 0.35f, 0.35f, 1);
        if (m.severity == ValidationSeverity::Warning) col = ImVec4(1, 0.8f, 0.2f, 1);
        const char* tag = m.severity == ValidationSeverity::Error ? "ERROR" : "WARNING";
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        if (ImGui::Selectable((std::string(tag) + ": " + m.message + "##" + m.objectId).c_str())) {
            if (!m.objectId.empty()) {
                ctx.Select({ctx.sroClientLoaded ? EntityKind::MapPlacement : EntityKind::WorldObject,
                    m.regionId, m.objectId});
            }
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::End();
}
