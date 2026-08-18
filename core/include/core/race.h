#pragma once

#include <vector>
#include <limits>

namespace mm {

struct Spawn;
class MapData;

enum class RaceState {
    Waiting,
    Countdown,
    Racing,
    Finished,
};

struct RacerState {
    int currentCheckpoint = 0;
    int currentLap = 0;
    float bestLap = std::numeric_limits<float>::max();
    float currentLapTime = 0.0f;
    bool finished = false;
    float finishTime = 0.0f;
    int position = 0;
};

struct CarPosition {
    float x;
    float y;
    float prevX;
    float prevY;
};

struct RaceData {
    RaceState state = RaceState::Waiting;
    float countdown = 0.0f;
    float raceTime = 0.0f;
    int totalLaps = 3;
    std::vector<RacerState> racers;
    int finishedCount = 0;
};

void raceInit(RaceData& race, const MapData& map, int numRacers);
void raceUpdate(RaceData& race, const std::vector<CarPosition>& cars, const MapData& map, float dt);
int raceGetPosition(const RaceData& race, int racerIdx);

} // namespace mm
