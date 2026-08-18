#include "core/bot.h"
#include "core/race.h"
#include "core/collision.h"
#include "core/map_data.h"
#include <cmath>
#include <algorithm>

namespace mm {

static constexpr float PI = 3.14159265358979f;

static float normalizeAngle(float a) {
    while (a > PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

static float castRay(float startX, float startY, float angle, float maxDist, const MapData& map) {
    int ts = map.tileSize();
    float dx = std::cos(angle);
    float dy = std::sin(angle);
    float step = ts * 0.5f;

    for (float d = step; d < maxDist; d += step) {
        float x = startX + dx * d;
        float y = startY + dy * d;
        int tx = static_cast<int>(std::floor(x / ts));
        int ty = static_cast<int>(std::floor(y / ts));

        if (!map.inBounds(tx, ty)) return d;

        if (isSolid(map.getGround(tx, ty)) || isSolid(map.getObject(tx, ty))) {
            return d;
        }
    }
    return maxDist;
}

PlayerInput botComputeInput(const CarState& car, const RacerState& racer,
                           const MapData& map, const BotConfig& cfg,
                           BotState& state, float dt) {
    PlayerInput in;
    int ts = map.tileSize();

    if (car.speed < 10.0f && std::abs(in.throttle) > 0.1f) {
        state.stuckTimer += dt;
    } else {
        state.stuckTimer = std::max(0.0f, state.stuckTimer - dt * 2.0f);
    }

    if (state.reverseTimer > 0.0f) {
        state.reverseTimer -= dt;
        float angleToTarget = 0.0f;
        auto& cps = map.checkpoints();
        if (!cps.empty()) {
            int next = racer.currentCheckpoint;
            auto& cp = cps[next];
            float targetX = (cp.x1 + cp.x2 + 1.0f) * 0.5f * ts;
            float targetY = (cp.y1 + cp.y2 + 1.0f) * 0.5f * ts;
            angleToTarget = std::atan2(targetY - car.y, targetX - car.x);
        }
        float angleDiff = normalizeAngle(angleToTarget - car.heading);
        in.steer = (angleDiff > 0) ? -1.0f : 1.0f;
        in.throttle = -0.8f;
        in.handbrake = false;
        return in;
    }

    if (state.stuckTimer > 0.8f) {
        state.stuckTimer = 0.0f;
        state.reverseTimer = 0.5f;
        in.throttle = -0.8f;
        in.steer = 0.0f;
        return in;
    }

    auto& cps = map.checkpoints();
    if (cps.empty()) {
        in.throttle = cfg.aggression;
        return in;
    }

    int next = racer.currentCheckpoint;
    auto& cp = cps[next];
    float targetX = (cp.x1 + cp.x2 + 1.0f) * 0.5f * ts;
    float targetY = (cp.y1 + cp.y2 + 1.0f) * 0.5f * ts;

    float angleToTarget = std::atan2(targetY - car.y, targetX - car.x);
    float angleDiff = normalizeAngle(angleToTarget - car.heading);

    float steerAmount = angleDiff / (PI * 0.5f);
    steerAmount = std::clamp(steerAmount, -1.0f, 1.0f);

    float inaccuracy = 1.0f - cfg.accuracy;
    steerAmount *= (1.0f - inaccuracy * 0.3f);

    float rayDist = cfg.rayDistance * ts;
    float rayAngles[] = { 0.0f, -0.4f, 0.4f, -0.8f, 0.8f };
    float rayWeights[] = { 2.0f, 1.0f, 1.0f, 0.5f, 0.5f };

    float avoidSteer = 0.0f;
    float closestHit = rayDist;

    for (int i = 0; i < 5; ++i) {
        float rayAngle = car.heading + rayAngles[i];
        float hitDist = castRay(car.x, car.y, rayAngle, rayDist, map);

        if (hitDist < closestHit) {
            closestHit = hitDist;
        }

        if (hitDist < rayDist * 0.6f) {
            float urgency = 1.0f - (hitDist / (rayDist * 0.6f));
            float side = (rayAngles[i] < 0) ? 1.0f : -1.0f;
            if (rayAngles[i] == 0.0f) {
                side = (steerAmount > 0) ? 1.0f : -1.0f;
            }
            avoidSteer += side * urgency * rayWeights[i] * cfg.awareness;
        }
    }

    float turnFactor = std::abs(angleDiff) / PI;
    float throttle = cfg.aggression * (1.0f - turnFactor * cfg.turnSlowdown);

    if (closestHit < rayDist * 0.3f) {
        throttle *= 0.5f;
    }

    in.steer = std::clamp(steerAmount + avoidSteer * 0.5f, -1.0f, 1.0f);
    in.throttle = std::clamp(throttle, 0.0f, 1.0f);
    in.handbrake = false;

    return in;
}

} // namespace mm
