#include "core/race.h"
#include "core/map_data.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace mm {

static constexpr float COUNTDOWN_TIME = 3.0f;

static std::vector<std::pair<int,int>> checkpointTiles(int x1, int y1, int x2, int y2) {
    std::vector<std::pair<int,int>> tiles;

    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int cx = x1, cy = y1;
    while (true) {
        tiles.push_back({cx, cy});
        if (cx == x2 && cy == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; cx += sx; }
        if (e2 < dx)  { err += dx; cy += sy; }
    }

    return tiles;
}

void raceInit(RaceData& race, const MapData& map, int numRacers) {
    race = RaceData();
    race.totalLaps = map.laps();
    race.racers.resize(numRacers);
    if (map.checkpoints().empty()) {
        race.state = RaceState::Racing;
    } else {
        race.state = RaceState::Countdown;
        race.countdown = COUNTDOWN_TIME;
    }
}

static int racerProgress(const RacerState& r, int numCheckpoints) {
    return r.currentLap * numCheckpoints + r.currentCheckpoint;
}

static void updatePositions(RaceData& race, const MapData& map) {
    int numCheckpoints = static_cast<int>(map.checkpoints().size());
    if (numCheckpoints == 0) return;

    std::vector<std::pair<int,int>> indexed;
    for (int i = 0; i < static_cast<int>(race.racers.size()); ++i) {
        int prog = racerProgress(race.racers[i], numCheckpoints);
        indexed.push_back({prog, i});
    }

    std::sort(indexed.begin(), indexed.end(), [](auto& a, auto& b) {
        return a.first > b.first;
    });

    for (int pos = 0; pos < static_cast<int>(indexed.size()); ++pos) {
        race.racers[indexed[pos].second].position = pos + 1;
    }
}

void raceUpdate(RaceData& race, const std::vector<CarPosition>& cars, const MapData& map, float dt) {
    if (race.state == RaceState::Countdown) {
        race.countdown -= dt;
        if (race.countdown <= 0.0f) {
            race.countdown = 0.0f;
            race.state = RaceState::Racing;
        }
        return;
    }

    if (race.state == RaceState::Finished) return;

    race.raceTime += dt;

    auto& cps = map.checkpoints();
    if (cps.empty()) return;

    int ts = map.tileSize();

    for (int i = 0; i < static_cast<int>(cars.size()); ++i) {
        if (i >= static_cast<int>(race.racers.size())) break;
        auto& racer = race.racers[i];
        if (racer.finished) continue;

        racer.currentLapTime += dt;

        int carTileX = static_cast<int>(std::floor(cars[i].x / ts));
        int carTileY = static_cast<int>(std::floor(cars[i].y / ts));

        int next = racer.currentCheckpoint;
        auto& cp = cps[next];

        auto tiles = checkpointTiles(cp.x1, cp.y1, cp.x2, cp.y2);

        for (auto& t : tiles) {
            if (t.first == carTileX && t.second == carTileY) {
                racer.currentCheckpoint++;

                if (racer.currentCheckpoint >= static_cast<int>(cps.size())) {
                    racer.currentCheckpoint = 0;
                    racer.currentLap++;

                    if (racer.currentLapTime < racer.bestLap)
                        racer.bestLap = racer.currentLapTime;
                    racer.currentLapTime = 0.0f;

                    if (racer.currentLap >= race.totalLaps) {
                        racer.finished = true;
                        racer.finishTime = race.raceTime;
                        race.finishedCount++;

                        if (race.finishedCount >= static_cast<int>(race.racers.size()))
                            race.state = RaceState::Finished;
                    }
                }
                break;
            }
        }
    }

    updatePositions(race, map);
}

int raceGetPosition(const RaceData& race, int racerIdx) {
    if (racerIdx >= 0 && racerIdx < static_cast<int>(race.racers.size()))
        return race.racers[racerIdx].position;
    return 0;
}

} // namespace mm
