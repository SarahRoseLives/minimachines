#include "core/map_data.h"
#include <algorithm>
#include <stdexcept>

namespace mm {

MapData::MapData()
    : MapData(32, 32, 32) {}

MapData::MapData(int width, int height, int tileSize)
    : m_width(width), m_height(height), m_tileSize(tileSize),
      m_ground(width * height, TileType::Grass),
      m_objects(width * height, TileType::Empty) {}

bool MapData::inBounds(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

void MapData::resize(int w, int h) {
    std::vector<TileType> newGround(w * h, TileType::Grass);
    std::vector<TileType> newObjects(w * h, TileType::Empty);
    int copyW = std::min(w, m_width);
    int copyH = std::min(h, m_height);
    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            newGround[y * w + x] = m_ground[y * m_width + x];
            newObjects[y * w + x] = m_objects[y * m_width + x];
        }
    }
    m_ground = std::move(newGround);
    m_objects = std::move(newObjects);
    m_width = w;
    m_height = h;
}

void MapData::setTileSize(int s) {
    m_tileSize = s;
}

TileType MapData::getGround(int x, int y) const {
    if (!inBounds(x, y)) return TileType::Empty;
    return m_ground[y * m_width + x];
}

TileType MapData::getObject(int x, int y) const {
    if (!inBounds(x, y)) return TileType::Empty;
    return m_objects[y * m_width + x];
}

void MapData::setGround(int x, int y, TileType t) {
    if (!inBounds(x, y)) return;
    m_ground[y * m_width + x] = t;
}

void MapData::setObject(int x, int y, TileType t) {
    if (!inBounds(x, y)) return;
    m_objects[y * m_width + x] = t;
}

} // namespace mm
