#include "core/car.h"
#include "core/collision.h"
#include "core/map_data.h"
#include <cmath>
#include <algorithm>

namespace mm {

void carUpdate(CarState& car, const PlayerInput& input, const CarConfig& cfg,
               const MapData& map, float dt) {
    float cosH = std::cos(car.heading);
    float sinH = std::sin(car.heading);

    float fwdX = cosH;
    float fwdY = sinH;
    float rightX = -sinH;
    float rightY = cosH;

    car.forwardSpeed = car.vx * fwdX + car.vy * fwdY;
    float lateralSpeed = car.vx * rightX + car.vy * rightY;

    TileType ground = getGroundTile(car.x, car.y, map);
    TileType obj = getObjectTile(car.x, car.y, map);
    SurfaceInfo surf = getSurface(ground, obj);

    float throttleForce = input.throttle * cfg.accel * surf.accel;
    if (input.throttle < 0.0f)
        throttleForce = input.throttle * cfg.brakeForce;

    car.forwardSpeed += throttleForce * dt;

    float speedCap = cfg.maxSpeed * surf.maxSpeed;
    if (car.forwardSpeed > speedCap) car.forwardSpeed = speedCap;
    if (car.forwardSpeed < -speedCap * 0.4f) car.forwardSpeed = -speedCap * 0.4f;

    car.forwardSpeed *= std::pow(cfg.drag, dt * 60.0f);

    float grip = cfg.grip * surf.grip;
    if (input.handbrake)
        grip = cfg.handbrakeGrip * surf.grip;

    float lateralDecay = std::pow(grip, dt * 60.0f);
    lateralSpeed *= lateralDecay;

    float absSpeed = std::abs(car.forwardSpeed);
    float steerFactor = 0.0f;
    if (absSpeed > cfg.minSteerSpeed)
        steerFactor = std::min(absSpeed / 100.0f, 1.0f);

    float steerDir = (car.forwardSpeed >= 0.0f) ? 1.0f : -1.0f;
    car.heading += input.steer * cfg.steerRate * steerFactor * steerDir * dt;

    cosH = std::cos(car.heading);
    sinH = std::sin(car.heading);
    car.vx = cosH * car.forwardSpeed + (-sinH) * lateralSpeed;
    car.vy = sinH * car.forwardSpeed + cosH * lateralSpeed;

    car.x += car.vx * dt;
    car.y += car.vy * dt;

    CollisionResult col = resolveCircleVsGrid(car.x, car.y, cfg.radius, map);
    if (col.hit) {
        car.x += col.pushX;
        car.y += col.pushY;

        float pushLen = std::sqrt(col.pushX * col.pushX + col.pushY * col.pushY);
        if (pushLen > 0.001f) {
            float nx = col.pushX / pushLen;
            float ny = col.pushY / pushLen;
            float dot = car.vx * nx + car.vy * ny;
            if (dot < 0.0f) {
                car.vx -= dot * nx;
                car.vy -= dot * ny;
            }
        }
    }

    car.speed = std::sqrt(car.vx * car.vx + car.vy * car.vy);
    car.forwardSpeed = car.vx * cosH + car.vy * sinH;
}

} // namespace mm
