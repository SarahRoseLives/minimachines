#include "palette.h"
#include "imgui.h"

namespace mm {

void Palette::draw(ToolState& state) {
    if (!ImGui::Begin("Palette")) {
        ImGui::End();
        return;
    }

    ImGui::Text("Tiles");
    ImGui::Separator();

    for (auto& info : allTileInfos()) {
        if (info.type == TileType::Empty) continue;

        ImGui::PushID(static_cast<int>(info.type));

        ImVec4 color(info.r / 255.0f, info.g / 255.0f, info.b / 255.0f, 1.0f);
        bool isSelected = (state.currentTool == ToolType::Paint || state.currentTool == ToolType::Erase)
                          && state.selectedTile == info.type;

        if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 0, 1));

        if (ImGui::ColorButton(info.name, color, 0, ImVec2(24, 24))) {
            state.currentTool = ToolType::Paint;
            state.selectedTile = info.type;
        }

        if (isSelected) ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextUnformatted(info.name);

        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Text("Tools");
    ImGui::Separator();

    auto toolButton = [&](const char* label, ToolType tool, ImVec4 col) {
        bool isSelected = (state.currentTool == tool);
        if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_Button, col);
        if (ImGui::Button(label, ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            state.currentTool = tool;
        }
        ImGui::PopStyleColor();
        if (isSelected) ImGui::PopStyleColor();
    };

    toolButton("Spawn", ToolType::StampSpawn, ImVec4(0.0f, 0.4f, 1.0f, 1.0f));
    toolButton("Checkpoint", ToolType::StampCheckpoint, ImVec4(1.0f, 0.55f, 0.0f, 1.0f));
    toolButton("Delete Entity", ToolType::DeleteEntity, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

    ImGui::End();
}

} // namespace mm
