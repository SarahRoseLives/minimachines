#pragma once

#include "core/tiles.h"

namespace mm {

class MapData;

struct SurfaceInfo {
    float grip = 1.0f;
    float accel = 1.0f;
    float maxSpeed = 1.0f;
};

SurfaceInfo getSurface(TileType ground, TileType object);
bool isSolid(TileType type);

struct CollisionResult {
    bool hit = false;
    float pushX = 0.0f;
    float pushY = 0.0f;
};

CollisionResult resolveCircleVsGrid(float cx, float cy, float radius, const MapData& map);
TileType getGroundTile(float cx, float cy, const MapData& map);
TileType getObjectTile(float cx, float cy, const MapData& map);

} // namespace mm
