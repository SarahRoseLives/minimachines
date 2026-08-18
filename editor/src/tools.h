#pragma once

#include "core/tiles.h"
#include "core/map_data.h"
#include "camera.h"
#include <SDL.h>

namespace mm {

enum class ToolType {
    Paint,
    Erase,
    StampSpawn,
    StampCheckpoint,
    DeleteEntity,
};

struct ToolState {
    ToolType currentTool = ToolType::Paint;
    TileType selectedTile = TileType::Grass;
    Layer activeLayer = Layer::Ground;
    bool painting = false;
    int spawnTileX = -1;
    int spawnTileY = -1;
    int aimX = 0;
    int aimY = 0;
    int checkTileX = -1;
    int checkTileY = -1;
};

class Tools {
public:
    void onMouseButton(const Camera& cam, MapData& map, ToolState& state,
                       int mx, int my, bool down, bool isLeft);
    void onMouseMove(const Camera& cam, MapData& map, ToolState& state,
                     int mx, int my);
};

} // namespace mm
