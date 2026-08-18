#pragma once

#include <vector>
#include <limits>

namespace mm {

struct Spawn;
struct Checkpoint;
class MapData;

enum class RaceState {
    Waiting,
    Countdown,
    Racing,
    Finished,
};

struct RaceData {
    RaceState state = RaceState::Waiting;
    float countdown = 0.0f;
    float raceTime = 0.0f;
    float bestLap = std::numeric_limits<float>::max();
    float currentLapTime = 0.0f;
    int currentCheckpoint = 0;
    int currentLap = 0;
    int totalLaps = 3;
};

void raceInit(RaceData& race, const MapData& map);
void raceUpdate(RaceData& race, float carX, float carY, const MapData& map, float dt,
                float prevX, float prevY);

} // namespace mm
