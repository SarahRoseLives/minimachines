#pragma once

#include "core/game_input.h"

namespace mm {

class MapData;

struct CarState {
    float x = 0.0f;
    float y = 0.0f;
    float heading = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float speed = 0.0f;
    float forwardSpeed = 0.0f;
};

struct CarConfig {
    float accel = 300.0f;
    float brakeForce = 400.0f;
    float maxSpeed = 300.0f;
    float steerRate = 3.0f;
    float drag = 0.985f;
    float grip = 0.85f;
    float handbrakeGrip = 0.45f;
    float radius = 12.0f;
    float minSteerSpeed = 20.0f;
};

void carUpdate(CarState& car, const PlayerInput& input, const CarConfig& cfg,
               const MapData& map, float dt);

} // namespace mm
