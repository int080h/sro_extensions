#include "ui/UiCommon.h"
#include "core/Logger.h"
#include "imgui.h"

void Ui::DrawAssetBrowserPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Asset Browser", &ctx.panels.assetBrowser)) { ImGui::End(); return; }

    static int tab = 0;
    const char* tabs[] = {"Static", "Buildings", "Trees", "Rocks", "Props", "NPCs", "Monsters", "Teleports", "Effects", "Prefabs"};
    if (ImGui::BeginTabBar("AssetTabs")) {
        for (int i = 0; i < 10; ++i) {
            if (ImGui::BeginTabItem(tabs[i], nullptr, tab == i ? ImGuiTabItemFlags_SetSelected : 0)) {
                tab = i;
                static char search[128] = "";
                ImGui::InputTextWithHint("##search", "Search name or CodeName128...", search, sizeof(search));
                ImGui::Separator();
                const char* samples[] = {"jangan_gate.bsr", "tree_oak.bsr", "rock_large.bsr", "NPC_CH_SMITH", "MOB_CH_MANGNYANG"};
                for (const char* s : samples) {
                    if (search[0] && strstr(s, search) == nullptr) continue;
                    if (ImGui::Selectable(s)) Logger::Instance().Info(std::string("Selected asset: ") + s);
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}
