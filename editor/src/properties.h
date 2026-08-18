#pragma once

#include "core/map_data.h"
#include <SDL.h>
#include <string>

namespace mm {

class Properties {
public:
    void draw(MapData& map, bool& showNewMapDialog);
    void drawNewMapDialog(MapData& map, bool& open);
};

} // namespace mm
