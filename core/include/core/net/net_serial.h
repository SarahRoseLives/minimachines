#pragma once

#include "core/net/packet.h"
#include "core/game_input.h"
#include "core/car.h"
#include "core/race.h"
#include <vector>
#include <string>
#include <cstring>
#include <cmath>

namespace mm {
namespace net {

struct StatePacket {
    uint16_t raceTime100;
    uint8_t raceState;
    uint8_t numCars;
    struct CarData {
        int16_t x;
        int16_t y;
        int16_t heading100;
        int16_t vx;
        int16_t vy;
        uint8_t checkpoint;
        uint8_t lap;
        uint8_t position;
        uint8_t flags;
    } cars[MAX_PLAYERS];
};

struct InputPacket {
    int8_t throttle100;
    int8_t steer100;
    uint8_t handbrake;
};

struct JoinPacket {
    char name[32];
};

struct PlayerInfo {
    uint8_t index;
    char name[32];
    bool connected;
};

inline InputPacket packInput(const PlayerInput& in) {
    InputPacket p;
    p.throttle100 = static_cast<int8_t>(in.throttle * 100.0f);
    p.steer100 = static_cast<int8_t>(in.steer * 100.0f);
    p.handbrake = in.handbrake ? 1 : 0;
    return p;
}

inline PlayerInput unpackInput(const InputPacket& p) {
    PlayerInput in;
    in.throttle = p.throttle100 / 100.0f;
    in.steer = p.steer100 / 100.0f;
    in.handbrake = p.handbrake != 0;
    return in;
}

inline StatePacket packState(const std::vector<CarState>& cars, const RaceData& race) {
    StatePacket p;
    p.raceTime100 = static_cast<uint16_t>(race.raceTime * 100.0f);
    p.raceState = static_cast<uint8_t>(race.state);
    p.numCars = static_cast<uint8_t>(cars.size());
    for (int i = 0; i < static_cast<int>(cars.size()) && i < MAX_PLAYERS; ++i) {
        auto& cd = p.cars[i];
        cd.x = static_cast<int16_t>(cars[i].x);
        cd.y = static_cast<int16_t>(cars[i].y);
        cd.heading100 = static_cast<int16_t>(cars[i].heading * 100.0f);
        cd.vx = static_cast<int16_t>(cars[i].vx);
        cd.vy = static_cast<int16_t>(cars[i].vy);
        if (i < static_cast<int>(race.racers.size())) {
            cd.checkpoint = static_cast<uint8_t>(race.racers[i].currentCheckpoint);
            cd.lap = static_cast<uint8_t>(race.racers[i].currentLap);
            cd.position = static_cast<uint8_t>(race.racers[i].position);
            cd.flags = race.racers[i].finished ? 1 : 0;
        }
    }
    return p;
}

inline void unpackState(const StatePacket& p, std::vector<CarState>& cars, RaceData& race) {
    race.state = static_cast<RaceState>(p.raceState);
    race.raceTime = p.raceTime100 / 100.0f;
    cars.resize(p.numCars);
    race.racers.resize(p.numCars);
    for (int i = 0; i < p.numCars; ++i) {
        auto& cd = p.cars[i];
        cars[i].x = cd.x;
        cars[i].y = cd.y;
        cars[i].heading = cd.heading100 / 100.0f;
        cars[i].vx = cd.vx;
        cars[i].vy = cd.vy;
        cars[i].speed = std::sqrt(static_cast<float>(cd.vx) * cd.vx + static_cast<float>(cd.vy) * cd.vy);
        cars[i].forwardSpeed = cars[i].speed;
        if (i < static_cast<int>(race.racers.size())) {
            race.racers[i].currentCheckpoint = cd.checkpoint;
            race.racers[i].currentLap = cd.lap;
            race.racers[i].position = cd.position;
            race.racers[i].finished = (cd.flags & 1) != 0;
        }
    }
}

} // namespace net
} // namespace mm
