#pragma once

#include "core/tiles.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mm {

struct Spawn {
    int x, y;
    float angle;
};

struct Checkpoint {
    int x1, y1;
    int x2, y2;
};

class MapData {
public:
    MapData();
    MapData(int width, int height, int tileSize);

    int width() const { return m_width; }
    int height() const { return m_height; }
    int tileSize() const { return m_tileSize; }

    void resize(int w, int h);
    void setTileSize(int s);

    TileType getGround(int x, int y) const;
    TileType getObject(int x, int y) const;
    void setGround(int x, int y, TileType t);
    void setObject(int x, int y, TileType t);

    std::vector<Spawn>& spawns() { return m_spawns; }
    const std::vector<Spawn>& spawns() const { return m_spawns; }

    std::vector<Checkpoint>& checkpoints() { return m_checkpoints; }
    const std::vector<Checkpoint>& checkpoints() const { return m_checkpoints; }

    int laps() const { return m_laps; }
    void setLaps(int l) { m_laps = l; }

    std::string& name() { return m_name; }
    const std::string& name() const { return m_name; }

    std::string& author() { return m_author; }
    const std::string& author() const { return m_author; }

    bool inBounds(int x, int y) const;

private:
    int m_width;
    int m_height;
    int m_tileSize;
    int m_laps = 3;
    std::string m_name = "Untitled";
    std::string m_author;
    std::vector<TileType> m_ground;
    std::vector<TileType> m_objects;
    std::vector<Spawn> m_spawns;
    std::vector<Checkpoint> m_checkpoints;
};

} // namespace mm
