#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "imgui.h"
#include "imgui_internal.h"

void Ui::DrawPerformancePanel(const EditorContext& ctx) {
    ImGui::Begin("Performance", &const_cast<PanelVisibility&>(ctx.panels).performance);
    ImGui::Text("FPS: %.1f", ctx.fps);
    int objCount = 0, spawnCount = 0, npcCount = 0;
    for (const auto& r : ctx.world.regions) {
        objCount += (int)r.objects.size();
        spawnCount += (int)r.spawns.size();
        npcCount += (int)r.npcs.size();
    }
    ImGui::Text("Object count: %d", objCount);
    ImGui::Text("Spawn count: %d", spawnCount);
    ImGui::Text("NPC count: %d", npcCount);
    ImGui::Text("Region count: %d", (int)ctx.world.regions.size());
    ImGui::TextDisabled("Draw calls: N/A (M2+)");
    ImGui::TextDisabled("Memory usage: N/A (M2+)");
    ImGui::End();
}

void Ui::SetupDefaultDockLayout(unsigned int dockspaceId) {
    ImGuiID id = dockspaceId;
    ImGui::DockBuilderRemoveNode(id);
    ImGui::DockBuilderAddNode(id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(id, ImGui::GetMainViewport()->Size);

    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 0.22f, nullptr, &id);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(id, ImGuiDir_Right, 0.25f, nullptr, &id);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(id, ImGuiDir_Down, 0.25f, nullptr, &id);
    ImGuiID dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.45f, nullptr, &dockLeft);

    ImGui::DockBuilderDockWindow("World Outliner", dockLeft);
    ImGui::DockBuilderDockWindow("Asset Browser", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Viewport", id);
    ImGui::DockBuilderDockWindow("Properties", dockRight);
    ImGui::DockBuilderDockWindow("Console", dockBottom);
    ImGui::DockBuilderDockWindow("Validation", dockBottom);
    ImGui::DockBuilderDockWindow("Performance", dockBottom);
    ImGui::DockBuilderDockWindow("World Map", dockBottom);
    ImGui::DockBuilderFinish(id);
}

void Ui::DrawViewportPanel(EditorContext& ctx, Editor& editor) {
    if (!ImGui::Begin("Viewport", &ctx.panels.viewport)) { ImGui::End(); return; }

    const char* modes[] = {"Lit", "Wireframe", "Collision", "Zone Overlay", "Spawn Overlay", "Region Debug"};
    int mode = static_cast<int>(ctx.viewport.viewMode);
    ImGui::SetNextItemWidth(140);
    if (ImGui::Combo("View Mode", &mode, modes, IM_ARRAYSIZE(modes)))
        ctx.viewport.viewMode = static_cast<ViewportMode>(mode);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Camera Speed", &ctx.viewport.cameraSpeed, 50.0f, 1000.0f);
    ImGui::SameLine();
    ImGui::Text("Region: %d", ctx.activeRegionId);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 1.0f || avail.y < 1.0f) {
        ImGui::End();
        return;
    }

    editor.RenderViewportScene(avail.x, avail.y);

    LPDIRECT3DTEXTURE9 tex = editor.Viewport().GetDisplayTexture();
    if (tex) {
        ImGui::Image((ImTextureID)(intptr_t)tex, avail, ImVec2(0, 0), ImVec2(1, 1));
    } else {
        ImGui::Dummy(avail);
    }

    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
    ImGui::InvisibleButton("viewport_canvas", avail, ImGuiButtonFlags_MouseButtonRight);
    bool focused = ImGui::IsItemFocused() || ImGui::IsItemHovered();
    editor.SetViewportFocused(focused);

    if (focused && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }

    if (!ctx.sroClientLoaded && tex) {
        const char* msg = "Sample world data — Import Client Data to load real SRO regions";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        ImVec2 pmin = ImGui::GetItemRectMin();
        ImVec2 pmax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(pmin.x + (pmax.x - pmin.x - ts.x) * 0.5f, pmax.y - ts.y - 8.0f),
            IM_COL32(200, 200, 200, 200), msg);
    }

    ImGui::Text("Mouse world: %.0f, %.0f, %.0f", ctx.mouseWorldPos.x, ctx.mouseWorldPos.y, ctx.mouseWorldPos.z);
    ImGui::End();
}
