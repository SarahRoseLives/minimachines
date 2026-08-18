#include "properties.h"
#include "imgui.h"

namespace mm {

bool Properties::draw(MapData& map) {
    bool newMapRequested = false;
    if (!ImGui::Begin("Properties")) {
        ImGui::End();
        return false;
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

    if (ImGui::Button("New Map")) {
        map = MapData(32, 32, 32);
        newMapRequested = true;
    }

    if (ImGui::Button("Clear Spawns"))
        map.spawns().clear();
    if (ImGui::Button("Clear Checkpoints"))
        map.checkpoints().clear();

    ImGui::End();
    return newMapRequested;
}

} // namespace mm
