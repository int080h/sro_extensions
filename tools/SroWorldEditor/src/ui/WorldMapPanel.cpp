#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "Rendering/WorldMapControl.h"
#include "Rendering/MinimapWidget.h"
#include "Rendering/RenderManager.h"
#include "core/SceneSpace.h"
#include "core/Logger.h"
#include "imgui.h"
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <algorithm>

namespace {

int MinimapHudContextKey(const WorldMapControl* wmc) {
    return wmc->MapStyle * 1000 + (wmc->IsInLocalMapView() ? 100 : 0) +
           (wmc->IsInDungeonMapView() ? 10 : 0);
}

bool MinimapHudRecommendedDefault(const WorldMapControl* wmc) {
    if (wmc->IsInLocalMapView()) return false;
    return wmc->MapStyle == 1 || wmc->MapStyle == 2 || wmc->IsInDungeonMapView();
}

} // namespace

void Ui::DrawWorldMapPanel(EditorContext& ctx, Editor& editor) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    if (!ImGui::Begin("World Map", &ctx.panels.worldMap, ImGuiWindowFlags_NoBackground)) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    if (!ctx.sroClientLoaded || !ctx.sroRenderManager) {
        ImGui::TextDisabled("Import client data to use the world map.");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    WorldMapControl* wmc = ctx.sroRenderManager->GetWorldMapControl();
    if (!wmc) {
        ImGui::TextDisabled("World map not initialized.");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    wmc->CurrentRx = ctx.sroRegionRx;
    wmc->CurrentRy = ctx.sroRegionRy;

    const EditorCamera& cam = editor.Viewport().GetCamera();
    const Vector3& camPos = cam.Position;
    sro::LocalPos lp = sro::SceneSpace::SceneToLocal(camPos.x, camPos.z, ctx.sroRegionRx, ctx.sroRegionRy);
    const float heading = sro::map::CameraHeadingToMapIconRadians(cam.Front.x, cam.Front.z);
    wmc->SetPlayerPosition(lp.rx, lp.ry, lp.localX, lp.localZ, heading);

    MinimapWidget* minimapHud = ctx.sroRenderManager->GetMinimapWidget();
    if (minimapHud) {
        minimapHud->SetCenterRegion(lp.rx, lp.ry);
        minimapHud->SetPlayerPosition(lp.localX, lp.localZ, heading);
    }

    float totalW = ImGui::GetContentRegionAvail().x;
    float totalH = ImGui::GetContentRegionAvail().y;
    float panelW = 200.0f;
    float mapW = totalW - panelW - 10.0f;
    if (mapW < 200.0f) mapW = 200.0f;

    static float canvasW = 400.0f;
    static float canvasH = 300.0f;
    static DWORD lastRegionLoadTick = 0;
    static bool showMinimapHud = false;
    static int lastHudContextKey = -1;

    const int hudContextKey = MinimapHudContextKey(wmc);
    if (hudContextKey != lastHudContextKey) {
        lastHudContextKey = hudContextKey;
        showMinimapHud = MinimapHudRecommendedDefault(wmc);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::BeginChild("MapViewPort", ImVec2(mapW, totalH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
    {
        const bool mapLoading = !wmc->IsMinimapIndexComplete() || wmc->PendingTextureLoads() > 0
            || (wmc->MapStyle == 0 && wmc->SelectedLocalMapId == 0 && !wmc->IsInDungeonMapView() &&
                !wmc->IsWorldPreloadComplete());
        ImVec2 vMin = ImGui::GetWindowContentRegionMin();
        ImVec2 vMax = ImGui::GetWindowContentRegionMax();
        ImVec2 pos = ImGui::GetWindowPos();

        float actualW = vMax.x - vMin.x;
        float actualH = vMax.y - vMin.y;

        canvasW = actualW;
        canvasH = actualH;

        const bool hudActive = showMinimapHud && minimapHud != nullptr;
        wmc->SetMinimapHudActive(hudActive, lp.rx, lp.ry);
        wmc->Tick(actualW, actualH);

        if (actualW > 0 && actualH > 0) {
            RenderManager::MapRenderParams mapParams;
            const float hudSize = (std::min)(160.0f, (std::max)(144.0f, actualW * 0.12f));
            float hudX = 0.0f;
            float hudY = 0.0f;
            if (hudActive) {
                mapParams.drawMinimapHud = true;
                mapParams.hudSize = hudSize;
                mapParams.hudX = actualW - hudSize - 8.0f;
                mapParams.hudY = 8.0f;
                hudX = mapParams.hudX;
                hudY = mapParams.hudY;
            }

            LPDIRECT3DTEXTURE9 mapTex = ctx.sroRenderManager->RenderMapToTexture(actualW, actualH, mapParams);
            if (mapTex) {
                ImGui::Image((ImTextureID)(intptr_t)mapTex, ImVec2(actualW, actualH));
            } else {
                ImGui::Dummy(ImVec2(actualW, actualH));
            }

            ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("MapInteractionArea", ImVec2(actualW, actualH));
            bool isHovered = ImGui::IsItemHovered();

            const float mapStartX = ImGui::GetItemRectMin().x;
            const float mapStartY = ImGui::GetItemRectMin().y;
            static bool mapLeftPressed = false;
            static bool mapLeftDragging = false;
            static ImVec2 mapPressPos = ImVec2(0.0f, 0.0f);
            static Vector2 mapPanStart(0.0f, 0.0f);

            auto isOverDungeonControls = [&](const ImVec2& mousePos) -> bool {
                if (!wmc->IsInDungeonMapView()) return false;
                const float panelX = mapStartX + actualW - 92.0f;
                const float panelY = mapStartY + 30.0f;
                return mousePos.x >= panelX && mousePos.x <= panelX + 84.0f &&
                       mousePos.y >= panelY && mousePos.y <= panelY + 72.0f;
            };

            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !isOverDungeonControls(ImGui::GetMousePos())) {
                mapLeftPressed = true;
                mapLeftDragging = false;
                mapPressPos = ImGui::GetMousePos();
                mapPanStart = wmc->m_panOffset;
                wmc->SetDragging(false);
            }

            if (mapLeftPressed && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                float dx = mousePos.x - mapPressPos.x;
                float dy = mousePos.y - mapPressPos.y;
                if (!mapLeftDragging && (fabsf(dx) > 3.0f || fabsf(dy) > 3.0f)) {
                    mapLeftDragging = true;
                    wmc->SetDragging(true);
                }
                if (mapLeftDragging) {
                    wmc->m_panOffset.x = mapPanStart.x + dx;
                    wmc->m_panOffset.y = mapPanStart.y + dy;
                    wmc->NotifyPanOrZoom();
                }
            }

            if (mapLeftPressed && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (!mapLeftDragging && wmc->SelectedLocalMapId == 0 && !wmc->IsInDungeonMapView() &&
                    (wmc->MapStyle == 0 || wmc->MapStyle == 1)) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    Vector2 mapPos(mousePos.x - mapStartX, mousePos.y - mapStartY);
                    if (auto mapId = wmc->HitTestWorldPoi(mapPos, actualW, actualH)) {
                        wmc->OpenLocalMap(*mapId, actualW, actualH);
                    } else if (auto floorId = wmc->HitTestDungeonEntrance(mapPos, actualW, actualH)) {
                        if (wmc->OpenDungeonMap(*floorId, actualW, actualH)) {
                            std::string floorLabel = wmc->GetDungeonFloorLabel(*floorId);
                            std::string msg = "Opened dungeon floor " + std::to_string(*floorId);
                            if (!floorLabel.empty()) msg += " (" + floorLabel + ")";
                            Logger::Instance().Info(msg);
                        } else {
                            Logger::Instance().Warning(
                                "Dungeon map data not loaded for floor id " + std::to_string(*floorId) +
                                " — check dungeonmap.txt and restart SroWorldEditor after rebuild.");
                        }
                    }
                }
                mapLeftPressed = false;
                wmc->SetDragging(false);
            }

            if (isHovered) {
                float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    Vector2 mapMousePos(mousePos.x - mapStartX, mousePos.y - mapStartY);
                    float oldZoom = wmc->m_zoom;
                    float factor = (wheel > 0.0f) ? 1.18f : 0.84f;
                    float newZoom = std::clamp(oldZoom * factor, 0.12f, 7.0f);
                    wmc->m_zoom = newZoom;
                    wmc->m_panOffset = mapMousePos - (mapMousePos - wmc->m_panOffset) * (newZoom / oldZoom);
                    wmc->NotifyPanOrZoom();
                }
            }

            auto travelToRegion = [&](int rx, int ry) {
                DWORD now = GetTickCount();
                if (now - lastRegionLoadTick < 250) return;
                wmc->CurrentRx = rx;
                wmc->CurrentRy = ry;
                lastRegionLoadTick = now;
                editor.Viewport().SetSroRegion(rx, ry);
                Logger::Instance().Info("Traveled to region " + std::to_string(rx) + "," + std::to_string(ry));
            };

            if (isHovered && !mapLeftDragging && wmc->SelectedLocalMapId == 0 &&
                !wmc->IsInDungeonMapView() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                int rx = 0, ry = 0;
                if (wmc->TryTranslateScreenToActiveRegion(Vector2(mousePos.x - mapStartX, mousePos.y - mapStartY), rx, ry, actualW, actualH)) {
                    travelToRegion(rx, ry);
                }
            }

            if (isHovered && !mapLeftDragging && wmc->SelectedLocalMapId == 0 &&
                !wmc->IsInDungeonMapView() &&
                ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::GetIO().KeyCtrl) {
                ImVec2 mousePos = ImGui::GetMousePos();
                int rx = 0, ry = 0;
                if (wmc->TryTranslateScreenToActiveRegion(Vector2(mousePos.x - mapStartX, mousePos.y - mapStartY), rx, ry, actualW, actualH)) {
                    travelToRegion(rx, ry);
                }
            }

            if (isHovered && wmc->SelectedLocalMapId == 0 && !wmc->IsInDungeonMapView() &&
                (wmc->MapStyle == 0 || wmc->MapStyle == 1)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                if (wmc->HitTestAnyWorldMapPoi(
                        Vector2(mousePos.x - mapStartX, mousePos.y - mapStartY), actualW, actualH)) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
            }

            if (wmc->IsInLocalMapView() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                wmc->CloseLocalMap();
            }
            if (wmc->IsInDungeonMapView()) {
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    wmc->CloseDungeonMap();
                }
                if (ImGui::IsKeyPressed(ImGuiKey_PageUp) || ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                    wmc->CycleDungeonFloor(-1, actualW, actualH);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_PageDown) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                    wmc->CycleDungeonFloor(1, actualW, actualH);
                }
            }

            wmc->RenderImGuiOverlays(mapStartX, mapStartY, actualW, actualH, isHovered);

            if (wmc->IsInDungeonMapView()) {
                const float panelX = mapStartX + actualW - 92.0f;
                const float panelY = mapStartY + 30.0f;
                ImGui::SetCursorScreenPos(ImVec2(panelX + 30.0f, panelY + 4.0f));
                ImGui::PushID("MapDungeonFloorUp");
                if (ImGui::Button("^", ImVec2(24.0f, 18.0f))) {
                    wmc->CycleDungeonFloor(-1, actualW, actualH);
                }
                ImGui::PopID();

                ImGui::SetCursorScreenPos(ImVec2(panelX + 30.0f, panelY + 48.0f));
                ImGui::PushID("MapDungeonFloorDown");
                if (ImGui::Button("v", ImVec2(24.0f, 18.0f))) {
                    wmc->CycleDungeonFloor(1, actualW, actualH);
                }
                ImGui::PopID();

                ImGui::SetCursorScreenPos(ImVec2(panelX + 4.0f, panelY + 22.0f));
                ImGui::PushID("MapDungeonBackWorld");
                if (ImGui::Button("W", ImVec2(22.0f, 28.0f))) {
                    wmc->CloseDungeonMap();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Back to world map");
                }
                ImGui::PopID();
            }

            if (mapLoading) {
                const char* statusText = !wmc->IsMinimapIndexComplete()
                    ? "Indexing minimap tiles..."
                    : (wmc->MapStyle == 0 && !wmc->IsWorldPreloadComplete())
                        ? "Preloading world map tiles..."
                        : "Loading map tiles...";
                ImVec2 textSize = ImGui::CalcTextSize(statusText);
                ImVec2 center(mapStartX + actualW * 0.5f - textSize.x * 0.5f,
                              mapStartY + actualH * 0.5f - textSize.y * 0.5f);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(ImVec2(mapStartX, mapStartY),
                                  ImVec2(mapStartX + actualW, mapStartY + actualH),
                                  IM_COL32(0, 0, 0, 120));
                dl->AddText(center, IM_COL32(230, 230, 230, 255), statusText);
                if (!wmc->IsMinimapIndexComplete()) {
                    char pct[32];
                    snprintf(pct, sizeof(pct), "%.0f%%", wmc->MinimapIndexProgress() * 100.0f);
                    ImVec2 pctSize = ImGui::CalcTextSize(pct);
                    dl->AddText(ImVec2(center.x + textSize.x * 0.5f - pctSize.x * 0.5f, center.y + textSize.y + 4.0f),
                                IM_COL32(180, 200, 255, 255), pct);
                } else if (wmc->MapStyle == 0 && !wmc->IsWorldPreloadComplete()) {
                    char pct[32];
                    snprintf(pct, sizeof(pct), "%.0f%%", wmc->WorldPreloadProgress() * 100.0f);
                    ImVec2 pctSize = ImGui::CalcTextSize(pct);
                    dl->AddText(ImVec2(center.x + textSize.x * 0.5f - pctSize.x * 0.5f, center.y + textSize.y + 4.0f),
                                IM_COL32(180, 200, 255, 255), pct);
                }
            }

            if (hudActive) {
                const float hudScreenX = mapStartX + hudX;
                const float hudScreenY = mapStartY + hudY;

                ImGui::SetCursorScreenPos(ImVec2(hudScreenX, hudScreenY));
                ImGui::SetNextItemAllowOverlap();
                ImGui::InvisibleButton("MinimapHudClick", ImVec2(hudSize, hudSize));
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    wmc->CenterOnRegion(lp.rx, lp.ry, canvasW, canvasH);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Click to center map on player");
                }

                const int tilesDrawn =
                    minimapHud->RenderImGuiTiles(hudScreenX, hudScreenY, hudSize);
                minimapHud->MarkRendered(tilesDrawn);
                minimapHud->RenderImGuiOverlay(hudScreenX, hudScreenY, hudSize,
                    wmc->IsMinimapIndexComplete(), wmc->MinimapIndexProgress());

                char hudCaption[80];
                snprintf(hudCaption, sizeof(hudCaption), "(%d,%d) %.0f,%.0f",
                         lp.ry, lp.rx, lp.localX, lp.localZ);
                ImDrawList* hudDl = ImGui::GetForegroundDrawList();
                ImVec2 captionSize = ImGui::CalcTextSize(hudCaption);
                hudDl->AddText(
                    ImVec2(hudScreenX + hudSize * 0.5f - captionSize.x * 0.5f, hudScreenY + hudSize + 2.0f),
                    IM_COL32(220, 210, 180, 230), hudCaption);
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.07f, 0.10f, 0.94f));
    ImGui::BeginChild("MapControls", ImVec2(panelW, totalH), true);
    {
        ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.15f, 1.0f), "Map Settings");
        ImGui::Separator();

        ImGui::Text("Map Style:");
        const char* styles[] = {"World Map", "Region Minimaps", "Instance / Dungeon"};
        if (ImGui::Combo("##Style", &wmc->MapStyle, styles, IM_ARRAYSIZE(styles))) {
            wmc->CloseLocalMap();
            wmc->CloseDungeonMap();
            wmc->RefreshStyleBounds();
            wmc->CenterOnRegion(wmc->CurrentRx, wmc->CurrentRy, canvasW, canvasH);
        }

        if (wmc->IsInLocalMapView()) {
            const WorldMapControl::LocalMapDefinition* local = wmc->FindLocalMapById(wmc->SelectedLocalMapId);
            std::string cityLabel = local && !local->NameUtf8.empty()
                ? local->NameUtf8
                : ("City map #" + std::to_string(wmc->SelectedLocalMapId));
            ImGui::TextColored(ImVec4(0.55f, 0.85f, 1.0f, 1.0f), "Viewing: %s", cityLabel.c_str());
            if (ImGui::Button("<- Back to World Map", ImVec2(-FLT_MIN, 0))) {
                wmc->CloseLocalMap();
            }
            ImGui::TextDisabled("Escape also returns to world map");
            ImGui::Spacing();
            ImGui::Separator();
        }

        if (wmc->IsInDungeonMapView()) {
            ImGui::TextColored(ImVec4(0.55f, 0.85f, 1.0f, 1.0f), "Viewing: %s",
                               wmc->GetDungeonViewTitle().c_str());
            const std::string floorLabel = wmc->GetDungeonFloorLabel(wmc->SelectedDungeonFloorId);
            if (!floorLabel.empty()) {
                ImGui::Text("Floor: %s", floorLabel.c_str());
            }
            if (ImGui::Button("Previous floor (^)", ImVec2(-FLT_MIN, 0))) {
                wmc->CycleDungeonFloor(-1, canvasW, canvasH);
            }
            if (ImGui::Button("Next floor (v)", ImVec2(-FLT_MIN, 0))) {
                wmc->CycleDungeonFloor(1, canvasW, canvasH);
            }
            if (ImGui::Button("<- Back to World Map", ImVec2(-FLT_MIN, 0))) {
                wmc->CloseDungeonMap();
            }
            ImGui::TextDisabled("PageUp/Down or ^/v on map; Escape returns to world map");
            ImGui::Spacing();
            ImGui::Separator();
        }

        if (wmc->MapStyle == 2 && !wmc->GetInstanceMapDefinitions().empty()) {
            ImGui::Text("Instance Map:");
            std::string instancePreview = "Auto (by region)";
            if (wmc->SelectedInstanceId > 0) {
                if (const auto* sel = wmc->FindInstanceMapById(wmc->SelectedInstanceId)) {
                    instancePreview = wmc->GetInstanceComboLabel(*sel);
                } else {
                    instancePreview = "Selected";
                }
            }
            if (ImGui::BeginCombo("##Instance", instancePreview.c_str())) {
                if (ImGui::Selectable("Auto (by region)", wmc->SelectedInstanceId == 0)) {
                    wmc->SelectedInstanceId = 0;
                    wmc->RefreshStyleBounds();
                }
                for (const auto& inst : wmc->GetInstanceMapDefinitions()) {
                    const std::string label = wmc->GetInstanceComboLabel(inst);
                    if (ImGui::Selectable(label.c_str(), wmc->SelectedInstanceId == inst.Id)) {
                        wmc->SelectedInstanceId = inst.Id;
                        wmc->RefreshStyleBounds();
                        wmc->CenterOnRegion(wmc->CurrentRx, wmc->CurrentRy, canvasW, canvasH);
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Checkbox("Corner Minimap HUD", &showMinimapHud);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("3x3 field minimap synced to 3D viewport.\n"
                              "Auto-on for Region Minimaps and dungeons;\n"
                              "off by default on World Map.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.15f, 1.0f), "Quick Travel");

        auto travelBtn = [&](const char* name, int rx, int ry) {
            if (ImGui::Button(name, ImVec2(-FLT_MIN, 0))) {
                wmc->CurrentRx = rx;
                wmc->CurrentRy = ry;
                wmc->CenterOnRegion(rx, ry, canvasW, canvasH);
                editor.Viewport().SetSroRegion(rx, ry);
            }
        };

        travelBtn("Jangan", 97, 167);
        travelBtn("Jangan Cave", 97, 160);
        travelBtn("Qin-Shi Tomb", 102, 172);
        travelBtn("Dunhuang", 87, 117);
        travelBtn("Hotan", 77, 87);
        travelBtn("Constantinople", 64, 96);
        travelBtn("Samarkand", 64, 107);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Region: (%d, %d)", wmc->CurrentRx, wmc->CurrentRy);
        ImGui::Text("Player: (%d, %d)", lp.rx, lp.ry);
        ImGui::Text("Local: (%.0f, %.0f)", lp.localX, lp.localZ);
        if (wmc->IsInLocalMapView()) {
            ImGui::TextDisabled("Pan/zoom inner city map");
        } else if (wmc->IsInDungeonMapView()) {
            ImGui::TextDisabled("Pan/zoom dungeon map");
            ImGui::TextDisabled("Sidebar or map ^/v; PageUp/Down for floors");
        } else {
            ImGui::TextDisabled("Click city icon for inner map");
            ImGui::TextDisabled("Click cave icon for dungeon map");
            ImGui::TextDisabled("Double-click map to travel");
            ImGui::TextDisabled("Ctrl+click also travels");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::PopStyleColor();
    ImGui::End();
}
