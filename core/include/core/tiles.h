#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mm {

enum class Layer {
    Ground,
    Objects,
};

enum class TileType : uint16_t {
    Empty = 0,

    // Ground layer
    Grass,
    Road,
    Dirt,
    Sand,
    Ice,
    Water,

    // Object layer
    Wall,
    Barrier,
    Ramp,
    Boost,
    Oil,
    Start,
    Checkpoint,
    Finish,

    Count
};

struct TileInfo {
    TileType type;
    const char* name;
    uint8_t r, g, b;
};

const std::vector<TileInfo>& allTileInfos();
const TileInfo* tileInfo(TileType type);

} // namespace mm
