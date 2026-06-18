#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "core/Logger.h"
#include "imgui.h"
#include <cstring>

static void MenuItemStub(const char* label, const char* shortcut = nullptr) {
    ImGui::BeginDisabled();
    ImGui::MenuItem(label, shortcut, false, false);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Coming soon");
}

void Ui::DrawMainMenuBar(EditorContext& ctx, Editor& editor) {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project", "Ctrl+N")) editor.NewProject();
        if (ImGui::MenuItem("Open Project", "Ctrl+O")) editor.OpenProject();
        if (ImGui::MenuItem("Save Project", "Ctrl+S")) editor.SaveProject();
        if (ImGui::MenuItem("Save Project As...")) editor.SaveProjectAs();
        ImGui::Separator();
        if (ImGui::MenuItem("Import Client Data")) editor.ImportClientData();
        if (ImGui::MenuItem("Clear Client Cache")) editor.ClearClientCache();
        MenuItemStub("Import Server Data");
        ImGui::BeginDisabled(!ctx.sroClientLoaded);
        if (ImGui::MenuItem("Save Changed Client Files")) editor.SaveChangedClientFiles();
        if (ImGui::MenuItem("Save Navmesh Files")) editor.SaveChangedClientFiles();
        if (ImGui::MenuItem("Export Changed Client Files")) editor.ExportChangedClientFiles();
        if (ImGui::MenuItem("Export Navmesh Folder")) editor.ExportChangedClientFiles();
        ImGui::EndDisabled();
        MenuItemStub("Export SQL Script");
        MenuItemStub("Export Patch Folder");
        ImGui::Separator();
        MenuItemStub("Recent Projects");
        if (ImGui::MenuItem("Exit", "Alt+F4")) PostQuitMessage(0);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        bool canUndo = editor.CanUndo();
        bool canRedo = editor.CanRedo();
        ImGui::BeginDisabled(!canUndo);
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) editor.Undo();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!canRedo);
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) editor.Redo();
        ImGui::EndDisabled();
        ImGui::Separator();
        MenuItemStub("Cut", "Ctrl+X");
        MenuItemStub("Copy", "Ctrl+C");
        MenuItemStub("Paste", "Ctrl+V");
        ImGui::BeginDisabled(!ctx.selection || ctx.selection->kind != EntityKind::MapPlacement);
        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) editor.DuplicateSelection();
        if (ImGui::MenuItem("Delete", "Del")) editor.DeleteSelection();
        ImGui::EndDisabled();
        MenuItemStub("Select All", "Ctrl+A");
        MenuItemStub("Deselect");
        MenuItemStub("Preferences");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Viewport", nullptr, &ctx.panels.viewport);
        ImGui::MenuItem("World Outliner", nullptr, &ctx.panels.worldOutliner);
        ImGui::MenuItem("Properties", nullptr, &ctx.panels.properties);
        ImGui::MenuItem("Object Viewer", nullptr, &ctx.panels.objectViewer);
        ImGui::MenuItem("Asset Browser", nullptr, &ctx.panels.assetBrowser);
        ImGui::MenuItem("Region Manager", nullptr, &ctx.panels.regionManager);
        ImGui::MenuItem("World Map", "M", &ctx.panels.worldMap);
        MenuItemStub("Spawn Editor");
        ImGui::MenuItem("NPC Editor", nullptr, &ctx.panels.npcEditor);
        MenuItemStub("Teleport Editor");
        MenuItemStub("Zone Editor");
        MenuItemStub("Collision Viewer");
        ImGui::MenuItem("Terrain Nav", nullptr, &ctx.panels.terrainNavPanel);
        ImGui::MenuItem("Nav Layers", nullptr, &ctx.panels.navLayersPanel);
        ImGui::MenuItem("AI Nav Data", nullptr, &ctx.panels.aiNavDataPanel);
        ImGui::MenuItem("Dungeon Nav", nullptr, &ctx.panels.dungeonNavPanel);
        ImGui::MenuItem("NavMesh Browser", nullptr, &ctx.panels.navMeshBrowser);
        ImGui::MenuItem("Nav Layers", nullptr, &ctx.panels.navLayersPanel);
        ImGui::MenuItem("Object Nav Inspector", nullptr, &ctx.panels.collisionEditor);
        ImGui::MenuItem("Validation Panel", nullptr, &ctx.panels.validation);
        ImGui::MenuItem("Console", nullptr, &ctx.panels.console);
        ImGui::MenuItem("Performance Monitor", nullptr, &ctx.panels.performance);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Navigation")) {
        if (ImGui::MenuItem("Nav Layers")) ctx.panels.navLayersPanel = true;
        if (ImGui::MenuItem("Terrain Nav Editor")) ctx.panels.terrainNavPanel = true;
        if (ImGui::MenuItem("AI Nav Data Editor")) ctx.panels.aiNavDataPanel = true;
        if (ImGui::MenuItem("Object Nav Inspector")) ctx.panels.collisionEditor = true;
        if (ImGui::MenuItem("Dungeon Nav")) ctx.panels.dungeonNavPanel = true;
        if (ImGui::MenuItem("NavMesh Browser")) ctx.panels.navMeshBrowser = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Collision Paint", nullptr, ctx.activeTool == EditorToolType::CollisionPaint))
            ctx.activeTool = EditorToolType::CollisionPaint;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Tools")) {
        const char* tools[] = {"Select", "Move", "Rotate", "Scale", "Terrain", "Object Placement", "Spawn Placement", "NPC Placement", "Teleport Placement", "Zone Paint", "Collision Paint", "Measure"};
        for (int i = 0; i < 12; ++i) {
            bool sel = static_cast<int>(ctx.activeTool) == i;
            if (ImGui::MenuItem(tools[i], nullptr, sel)) ctx.activeTool = static_cast<EditorToolType>(i);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("World")) {
        if (ImGui::MenuItem("Region Manager")) ctx.panels.regionManager = true;
        if (ImGui::MenuItem("World Map")) ctx.panels.worldMap = true;
        MenuItemStub("Object Manager");
        MenuItemStub("NPC Manager");
        MenuItemStub("Monster Spawn Manager");
        MenuItemStub("Teleport Manager");
        MenuItemStub("Zone Manager");
        MenuItemStub("Dungeon Manager");
        MenuItemStub("Fortress Manager");
        MenuItemStub("Event Zone Manager");
        MenuItemStub("Minimap Manager");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Database")) {
        MenuItemStub("Connect to Database");
        MenuItemStub("RefObjCommon Browser");
        MenuItemStub("RefObjChar Browser");
        MenuItemStub("RefObjItem Browser");
        MenuItemStub("RefShop Browser");
        MenuItemStub("RefTeleport Browser");
        MenuItemStub("RefNest / RefHive / RefTactics Browser");
        MenuItemStub("Generate SQL");
        MenuItemStub("Compare Database");
        MenuItemStub("Validate Database Links");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Build")) {
        if (ImGui::MenuItem("Validate Project")) editor.RunValidation();
        MenuItemStub("Validate Current Region");
        MenuItemStub("Generate SQL");
        ImGui::BeginDisabled(!ctx.sroClientLoaded);
        if (ImGui::MenuItem("Generate Client Patch")) editor.ExportChangedClientFiles();
        ImGui::EndDisabled();
        MenuItemStub("Export Selected Region");
        ImGui::BeginDisabled(!ctx.sroClientLoaded);
        if (ImGui::MenuItem("Export Changed Regions")) editor.ExportChangedClientFiles();
        ImGui::EndDisabled();
        MenuItemStub("Run Full Error Check");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window")) {
        if (ImGui::MenuItem("Reset Layout")) editor.ResetLayout();
        MenuItemStub("Save Layout");
        MenuItemStub("Load Layout");
        MenuItemStub("Close All Panels");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        MenuItemStub("Documentation");
        MenuItemStub("Shortcuts");
        if (ImGui::MenuItem("About")) Logger::Instance().Info("Silkroad Online Game World Editor v0.1");
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}
