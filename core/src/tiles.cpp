#include "core/tiles.h"

namespace mm {

static const std::vector<TileInfo> s_infos = {
    { TileType::Empty,     "Empty",     0,   0,   0   },
    { TileType::Grass,     "Grass",     76,  153, 0   },
    { TileType::Road,      "Road",      96,  96,  96  },
    { TileType::Dirt,      "Dirt",      139, 90,  43  },
    { TileType::Sand,      "Sand",      210, 180, 140 },
    { TileType::Ice,       "Ice",       173, 216, 230 },
    { TileType::Water,     "Water",     30,  90,  180 },
    { TileType::Wall,      "Wall",      50,  50,  50  },
    { TileType::Barrier,   "Barrier",   180, 180, 0   },
    { TileType::Ramp,      "Ramp",      160, 120, 60  },
    { TileType::Boost,     "Boost",     255, 165, 0   },
    { TileType::Oil,       "Oil",       30,  30,  30  },
    { TileType::Start,     "Start",     0,   100, 255 },
    { TileType::Checkpoint,"Checkpoint",255, 140, 0   },
    { TileType::Finish,    "Finish",    255, 0,   0   },
};

const std::vector<TileInfo>& allTileInfos() {
    return s_infos;
}

const TileInfo* tileInfo(TileType type) {
    auto idx = static_cast<int>(type);
    if (idx >= 0 && idx < static_cast<int>(s_infos.size()))
        return &s_infos[idx];
    return nullptr;
}

} // namespace mm
