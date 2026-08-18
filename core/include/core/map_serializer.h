#pragma once

#include "core/map_data.h"
#include <string>

namespace mm {

class MapSerializer {
public:
    static bool saveToFile(const MapData& map, const std::string& path);
    static bool loadFromFile(MapData& map, const std::string& path);
};

} // namespace mm
