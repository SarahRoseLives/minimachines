#include "core/collision.h"
#include "core/map_data.h"
#include <cmath>

namespace mm {

SurfaceInfo getSurface(TileType ground, TileType object) {
    SurfaceInfo s;

    switch (ground) {
    case TileType::Road:  s.grip = 1.0f; s.accel = 1.0f; s.maxSpeed = 1.0f; break;
    case TileType::Grass: s.grip = 0.7f; s.accel = 0.6f; s.maxSpeed = 0.6f; break;
    case TileType::Dirt:  s.grip = 0.8f; s.accel = 0.7f; s.maxSpeed = 0.7f; break;
    case TileType::Sand:  s.grip = 0.5f; s.accel = 0.4f; s.maxSpeed = 0.5f; break;
    case TileType::Ice:   s.grip = 0.05f; s.accel = 0.8f; s.maxSpeed = 0.9f; break;
    case TileType::Water: s.grip = 0.2f; s.accel = 0.2f; s.maxSpeed = 0.3f; break;
    default:              s.grip = 1.0f; s.accel = 1.0f; s.maxSpeed = 1.0f; break;
    }

    switch (object) {
    case TileType::Boost: s.accel *= 2.0f; s.maxSpeed *= 1.5f; break;
    case TileType::Oil:   s.grip = 0.1f; break;
    default: break;
    }

    return s;
}

bool isSolid(TileType type) {
    return type == TileType::Wall || type == TileType::Barrier;
}

CollisionResult resolveCircleVsGrid(float cx, float cy, float radius, const MapData& map) {
    CollisionResult result;
    int ts = map.tileSize();
    float r2 = radius * radius;

    int minTX = static_cast<int>(std::floor((cx - radius) / ts));
    int maxTX = static_cast<int>(std::floor((cx + radius) / ts));
    int minTY = static_cast<int>(std::floor((cy - radius) / ts));
    int maxTY = static_cast<int>(std::floor((cy + radius) / ts));

    for (int ty = minTY; ty <= maxTY; ++ty) {
        for (int tx = minTX; tx <= maxTX; ++tx) {
            TileType obj = map.getObject(tx, ty);
            TileType ground = map.getGround(tx, ty);
            if (!isSolid(obj) && !isSolid(ground)) continue;

            float nearX = cx;
            float nearY = cy;
            if (nearX < tx * ts) nearX = static_cast<float>(tx * ts);
            if (nearX > (tx + 1) * ts) nearX = static_cast<float>((tx + 1) * ts);
            if (nearY < ty * ts) nearY = static_cast<float>(ty * ts);
            if (nearY > (ty + 1) * ts) nearY = static_cast<float>((ty + 1) * ts);

            float dx = cx - nearX;
            float dy = cy - nearY;
            float dist2 = dx * dx + dy * dy;

            if (dist2 < r2) {
                float dist = std::sqrt(dist2);
                if (dist < 0.001f) {
                    result.pushX = radius;
                    result.hit = true;
                } else {
                    float pen = radius - dist;
                    result.pushX += (dx / dist) * pen;
                    result.pushY += (dy / dist) * pen;
                    result.hit = true;
                }
            }
        }
    }

    return result;
}

TileType getGroundTile(float cx, float cy, const MapData& map) {
    int ts = map.tileSize();
    int tx = static_cast<int>(std::floor(cx / ts));
    int ty = static_cast<int>(std::floor(cy / ts));
    return map.getGround(tx, ty);
}

TileType getObjectTile(float cx, float cy, const MapData& map) {
    int ts = map.tileSize();
    int tx = static_cast<int>(std::floor(cx / ts));
    int ty = static_cast<int>(std::floor(cy / ts));
    return map.getObject(tx, ty);
}

} // namespace mm
