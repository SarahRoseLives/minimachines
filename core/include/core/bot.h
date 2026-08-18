#pragma once

#include "core/game_input.h"
#include "core/car.h"

namespace mm {

class MapData;
struct RacerState;

struct BotConfig {
    float aggression = 0.8f;
    float accuracy = 0.9f;
    float awareness = 1.0f;
    float turnSlowdown = 0.5f;
    float rayDistance = 5.0f;
};

PlayerInput botComputeInput(const CarState& car, const RacerState& racer,
                           const MapData& map, const BotConfig& cfg);

} // namespace mm
