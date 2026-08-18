#include "properties.h"
#include "imgui.h"
#include "core/map_serializer.h"

namespace mm {

void Properties::draw(MapData& map, bool& showNewMapDialog) {
    if (!ImGui::Begin("Properties")) {
        ImGui::End();
        return;
    }

    char nameBuf[128];
    char authorBuf[128];
    snprintf(nameBuf, sizeof(nameBuf), "%s", map.name().c_str());
    snprintf(authorBuf, sizeof(authorBuf), "%s", map.author().c_str());

    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
        map.name() = nameBuf;
    if (ImGui::InputText("Author", authorBuf, sizeof(authorBuf)))
        map.author() = authorBuf;

    int w = map.width(), h = map.height();
    if (ImGui::InputInt("Width", &w) && w > 0 && w <= 256)
        map.resize(w, h);
    if (ImGui::InputInt("Height", &h) && h > 0 && h <= 256)
        map.resize(w, h);

    int ts = map.tileSize();
    if (ImGui::InputInt("Tile Size", &ts) && ts > 0)
        map.setTileSize(ts);

    int laps = map.laps();
    if (ImGui::InputInt("Laps", &laps) && laps > 0)
        map.setLaps(laps);

    ImGui::Separator();
    ImGui::Text("Spawns: %d", (int)map.spawns().size());
    ImGui::Text("Checkpoints: %d", (int)map.checkpoints().size());

    if (ImGui::Button("New Map"))
        showNewMapDialog = true;

    if (ImGui::Button("Clear Spawns"))
        map.spawns().clear();
    if (ImGui::Button("Clear Checkpoints"))
        map.checkpoints().clear();

    ImGui::End();
}

void Properties::drawNewMapDialog(MapData& map, bool& open) {
    if (!open) return;

    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("New Map", &open)) {
        static int nw = 32, nh = 32, nts = 32;
        ImGui::InputInt("Width", &nw);
        ImGui::InputInt("Height", &nh);
        ImGui::InputInt("Tile Size", &nts);

        if (nw < 1) nw = 1;
        if (nh < 1) nh = 1;
        if (nts < 1) nts = 1;

        if (ImGui::Button("Create")) {
            map = MapData(nw, nh, nts);
            open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            open = false;
    }
    ImGui::End();
}

} // namespace mm
