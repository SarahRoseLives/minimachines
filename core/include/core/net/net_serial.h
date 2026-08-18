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
    uint16_t connectedMask;
    uint16_t totalLaps;
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
        uint8_t playerIndex;
        uint8_t pad[3];
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

inline StatePacket packState(const std::vector<CarState>& cars, const RaceData& race, const std::vector<int>& indices = {}) {
    StatePacket p;
    p.raceTime100 = static_cast<uint16_t>(race.raceTime * 100.0f);
    p.raceState = static_cast<uint8_t>(race.state);
    p.numCars = static_cast<uint8_t>(cars.size());
    p.totalLaps = race.totalLaps;

    uint16_t mask = 0;
    for (int idx : indices) {
        if (idx >= 0 && idx < MAX_PLAYERS) mask |= (1 << idx);
    }
    p.connectedMask = mask;

    for (int i = 0; i < static_cast<int>(cars.size()) && i < MAX_PLAYERS; ++i) {
        auto& cd = p.cars[i];
        cd.x = static_cast<int16_t>(cars[i].x);
        cd.y = static_cast<int16_t>(cars[i].y);
        cd.heading100 = static_cast<int16_t>(cars[i].heading * 100.0f);
        cd.vx = static_cast<int16_t>(cars[i].vx);
        cd.vy = static_cast<int16_t>(cars[i].vy);
        cd.playerIndex = (i < static_cast<int>(indices.size())) ? static_cast<uint8_t>(indices[i]) : static_cast<uint8_t>(i);
        cd.pad[0] = cd.pad[1] = cd.pad[2] = 0;
        if (i < static_cast<int>(race.racers.size())) {
            int ri = cd.playerIndex;
            if (ri < static_cast<int>(race.racers.size())) {
                cd.checkpoint = static_cast<uint8_t>(race.racers[ri].currentCheckpoint);
                cd.lap = static_cast<uint8_t>(race.racers[ri].currentLap);
                cd.position = static_cast<uint8_t>(race.racers[ri].position);
                cd.flags = race.racers[ri].finished ? 1 : 0;
            }
        }
    }
    return p;
}

inline void unpackState(const StatePacket& p, std::vector<CarState>& cars, RaceData& race, uint16_t& connectedMask) {
    race.state = static_cast<RaceState>(p.raceState);
    race.raceTime = p.raceTime100 / 100.0f;
    race.totalLaps = p.totalLaps;
    connectedMask = p.connectedMask;

    cars.clear();
    race.racers.clear();
    for (int i = 0; i < p.numCars; ++i) {
        auto& cd = p.cars[i];
        CarState car;
        car.x = cd.x;
        car.y = cd.y;
        car.heading = cd.heading100 / 100.0f;
        car.vx = cd.vx;
        car.vy = cd.vy;
        car.speed = std::sqrt(static_cast<float>(cd.vx) * cd.vx + static_cast<float>(cd.vy) * cd.vy);
        car.forwardSpeed = car.speed;
        cars.push_back(car);

        RacerState racer;
        racer.currentCheckpoint = cd.checkpoint;
        racer.currentLap = cd.lap;
        racer.position = cd.position;
        racer.finished = (cd.flags & 1) != 0;
        race.racers.push_back(racer);
    }
}

} // namespace net
} // namespace mm
