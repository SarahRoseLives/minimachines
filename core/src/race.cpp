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

void raceInit(RaceData& race, const MapData& map) {
    race = RaceData();
    race.totalLaps = map.laps();
    if (map.checkpoints().empty()) {
        race.state = RaceState::Racing;
    } else {
        race.state = RaceState::Countdown;
        race.countdown = COUNTDOWN_TIME;
    }
}

void raceUpdate(RaceData& race, float carX, float carY, const MapData& map, float dt,
                float prevX, float prevY) {
    (void)prevX;
    (void)prevY;

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
    race.currentLapTime += dt;

    auto& cps = map.checkpoints();
    if (cps.empty()) return;

    int ts = map.tileSize();
    int carTileX = static_cast<int>(std::floor(carX / ts));
    int carTileY = static_cast<int>(std::floor(carY / ts));

    int next = race.currentCheckpoint;
    auto& cp = cps[next];

    auto tiles = checkpointTiles(cp.x1, cp.y1, cp.x2, cp.y2);

    for (auto& t : tiles) {
        if (t.first == carTileX && t.second == carTileY) {
            race.currentCheckpoint++;

            if (race.currentCheckpoint >= static_cast<int>(cps.size())) {
                race.currentCheckpoint = 0;
                race.currentLap++;

                if (race.currentLapTime < race.bestLap)
                    race.bestLap = race.currentLapTime;
                race.currentLapTime = 0.0f;

                if (race.currentLap >= race.totalLaps)
                    race.state = RaceState::Finished;
            }
            break;
        }
    }
}

} // namespace mm
