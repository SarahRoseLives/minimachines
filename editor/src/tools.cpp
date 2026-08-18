#include "tools.h"
#include <cmath>
#include <climits>

namespace mm {

static void paintTile(MapData& map, ToolState& state, int tx, int ty) {
    if (!map.inBounds(tx, ty)) return;

    if (state.roadWallPreset && state.selectedTile == TileType::Road) {
        int half = state.brushSize / 2;
        for (int dx = -half - 1; dx <= half + 1; ++dx) {
            if (!map.inBounds(tx + dx, ty)) continue;
            if (dx < -half || dx > half)
                map.setObject(tx + dx, ty, TileType::Wall);
            else
                map.setGround(tx + dx, ty, TileType::Road);
        }
        return;
    }

    if (state.currentTool == ToolType::Erase) {
        if (state.activeLayer == Layer::Ground)
            map.setGround(tx, ty, TileType::Grass);
        else
            map.setObject(tx, ty, TileType::Empty);
    } else {
        if (state.activeLayer == Layer::Ground)
            map.setGround(tx, ty, state.selectedTile);
        else
            map.setObject(tx, ty, state.selectedTile);
    }
}

static void applyTool(const Camera& cam, MapData& map, ToolState& state, int mx, int my) {
    SDL_Point tile = cam.screenToWorld(mx, my, map.tileSize());
    if (!map.inBounds(tile.x, tile.y)) return;

    int half = state.brushSize / 2;
    for (int dy = -half; dy < state.brushSize - half; ++dy) {
        for (int dx = -half; dx < state.brushSize - half; ++dx) {
            paintTile(map, state, tile.x + dx, tile.y + dy);
        }
    }
}

void Tools::applyRectFill(MapData& map, ToolState& state, int x1, int y1, int x2, int y2) {
    int minX = std::min(x1, x2);
    int maxX = std::max(x1, x2);
    int minY = std::min(y1, y2);
    int maxY = std::max(y1, y2);

    for (int y = minY; y <= maxY; ++y)
        for (int x = minX; x <= maxX; ++x)
            paintTile(map, state, x, y);
}

void Tools::onMouseButton(const Camera& cam, MapData& map, ToolState& state,
                          int mx, int my, bool down, bool isLeft, bool shiftHeld) {
    if (!isLeft) return;

    int ts = map.tileSize();

    if (down) {
        if (shiftHeld && (state.currentTool == ToolType::Paint || state.currentTool == ToolType::Erase)) {
            SDL_Point tile = cam.screenToWorld(mx, my, ts);
            if (!map.inBounds(tile.x, tile.y)) return;
            state.rectFilling = true;
            state.rectStartX = tile.x;
            state.rectStartY = tile.y;
            state.rectEndX = tile.x;
            state.rectEndY = tile.y;
            return;
        }
        if (state.currentTool == ToolType::StampSpawn) {
            SDL_Point tile = cam.screenToWorld(mx, my, ts);
            if (!map.inBounds(tile.x, tile.y)) return;
            state.spawnTileX = tile.x;
            state.spawnTileY = tile.y;
            state.aimX = mx;
            state.aimY = my;
            state.painting = true;
            return;
        }

        if (state.currentTool == ToolType::StampCheckpoint) {
            SDL_Point tile = cam.screenToWorld(mx, my, ts);
            if (!map.inBounds(tile.x, tile.y)) return;
            state.checkTileX = tile.x;
            state.checkTileY = tile.y;
            state.aimX = mx;
            state.aimY = my;
            state.painting = true;
            return;
        }

        if (state.currentTool == ToolType::DeleteEntity) {
            SDL_Point tile = cam.screenToWorld(mx, my, ts);
            int bestDist = INT_MAX;
            int bestIdx = -1;
            bool isSpawn = true;

            for (int i = 0; i < static_cast<int>(map.spawns().size()); ++i) {
                int dx = map.spawns()[i].x - tile.x;
                int dy = map.spawns()[i].y - tile.y;
                int d = dx * dx + dy * dy;
                if (d < bestDist) {
                    bestDist = d;
                    bestIdx = i;
                    isSpawn = true;
                }
            }

            for (int i = 0; i < static_cast<int>(map.checkpoints().size()); ++i) {
                auto& cp = map.checkpoints()[i];
                int midX = (cp.x1 + cp.x2) / 2;
                int midY = (cp.y1 + cp.y2) / 2;
                int dx = midX - tile.x;
                int dy = midY - tile.y;
                int d = dx * dx + dy * dy;
                if (d < bestDist) {
                    bestDist = d;
                    bestIdx = i;
                    isSpawn = false;
                }
            }

            if (bestIdx >= 0 && bestDist <= 4) {
                if (isSpawn)
                    map.spawns().erase(map.spawns().begin() + bestIdx);
                else
                    map.checkpoints().erase(map.checkpoints().begin() + bestIdx);
            }
            return;
        }

        state.painting = true;
        applyTool(cam, map, state, mx, my);
    } else {
        if (state.rectFilling) {
            applyRectFill(map, state, state.rectStartX, state.rectStartY, state.rectEndX, state.rectEndY);
            state.rectFilling = false;
            state.rectStartX = -1;
            state.rectStartY = -1;
            state.rectEndX = -1;
            state.rectEndY = -1;
            return;
        }

        if (state.currentTool == ToolType::StampSpawn && state.painting) {
            float centerX = (state.spawnTileX + 0.5f) * ts;
            float centerY = (state.spawnTileY + 0.5f) * ts;
            float aimWorldX = mx / cam.zoom + cam.offsetX;
            float aimWorldY = my / cam.zoom + cam.offsetY;
            float angle = std::atan2(aimWorldY - centerY, aimWorldX - centerX);

            Spawn s;
            s.x = state.spawnTileX;
            s.y = state.spawnTileY;
            s.angle = angle;
            map.spawns().push_back(s);
        }

        if (state.currentTool == ToolType::StampCheckpoint && state.painting) {
            SDL_Point endTile = cam.screenToWorld(mx, my, ts);
            if (!map.inBounds(endTile.x, endTile.y)) {
                endTile.x = state.checkTileX;
                endTile.y = state.checkTileY;
            }

            Checkpoint c;
            c.x1 = state.checkTileX;
            c.y1 = state.checkTileY;
            c.x2 = endTile.x;
            c.y2 = endTile.y;
            map.checkpoints().push_back(c);
        }

        state.painting = false;
        state.spawnTileX = -1;
        state.spawnTileY = -1;
        state.checkTileX = -1;
        state.checkTileY = -1;
    }
}

void Tools::onMouseMove(const Camera& cam, MapData& map, ToolState& state,
                        int mx, int my) {
    if (state.rectFilling) {
        SDL_Point tile = cam.screenToWorld(mx, my, map.tileSize());
        state.rectEndX = tile.x;
        state.rectEndY = tile.y;
        return;
    }

    if (state.currentTool == ToolType::StampSpawn && state.painting) {
        state.aimX = mx;
        state.aimY = my;
        return;
    }

    if (state.currentTool == ToolType::StampCheckpoint && state.painting) {
        state.aimX = mx;
        state.aimY = my;
        return;
    }

    if (!state.painting) return;
    applyTool(cam, map, state, mx, my);
}

} // namespace mm
