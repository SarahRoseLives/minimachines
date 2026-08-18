#include "game_app.h"
#include "core/map_serializer.h"
#include "core/collision.h"
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <random>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace mm {

static const TileInfo* findTileInfo(TileType type) {
    for (auto& info : allTileInfos()) {
        if (info.type == type) return &info;
    }
    return nullptr;
}

static const int NUM_BOTS = 3;

static const Uint8 CAR_COLORS[][3] = {
    {51, 153, 255},
    {255, 80, 80},
    {80, 255, 80},
    {255, 255, 80},
};

static std::string getExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string path(buf);
    auto pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : ".";
#elif defined(__ANDROID__)
    char* pref = SDL_GetPrefPath("minimachines", "game");
    std::string result(pref);
    SDL_free(pref);
    return result;
#else
    return ".";
#endif
}

void GameApp::generateFallbackMap() {
    const int W = 40, H = 30, TS = 32;
    m_map = MapData(W, H, TS);
    m_map.name() = "Fallback Track";

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            bool edge = (x == 0 || x == W - 1 || y == 0 || y == H - 1);
            bool inner = (x >= 10 && x <= 29 && y >= 8 && y <= 21);
            bool innerEdge = (x == 10 || x == 29 || y == 8 || y == 21);

            if (edge) {
                m_map.setGround(x, y, TileType::Wall);
            } else if (inner) {
                if (innerEdge)
                    m_map.setGround(x, y, TileType::Wall);
                else
                    m_map.setGround(x, y, TileType::Grass);
            } else {
                m_map.setGround(x, y, TileType::Road);
            }
        }
    }

    m_map.setObject(20, 2, TileType::Start);

    m_map.checkpoints().push_back({38, 3, 38, 26});
    m_map.checkpoints().push_back({38, 26, 3, 26});
    m_map.checkpoints().push_back({3, 26, 3, 3});
    m_map.checkpoints().push_back({3, 3, 38, 3});

    m_map.setLaps(3);
}
void GameApp::loadRandomMap() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    const int W = 1000, H = 1000, TS = 32;
    const int ROAD_HW = 3;
    const int PAD = 60;
    const float PI = 3.14159265358979f;

    m_map = MapData(W, H, TS);
    m_map.name() = "Random Track";
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            m_map.setGround(x, y, TileType::Grass);

    auto segDist = [](float ax, float ay, float bx, float by,
                      float cx, float cy, float dx, float dy) -> float {
        auto dot = [](float ax, float ay, float bx, float by) { return ax*bx + ay*by; };
        float abx = bx-ax, aby = by-ay, len2 = abx*abx+aby*aby;
        if (len2 < 0.001f) { float dxx=cx-ax,dyy=cy-ay; return std::sqrt(dxx*dxx+dyy*dyy); }
        float t = std::clamp(dot(cx-ax,cy-ay,abx,aby)/len2, 0.0f, 1.0f);
        float px = ax+t*abx, py = ay+t*aby;
        float cdx = dx-cx, cdy = dy-cy, len2cd = cdx*cdx+cdy*cdy;
        if (len2cd < 0.001f) { float dxx=px-cx,dyy=py-cy; return std::sqrt(dxx*dxx+dyy*dyy); }
        float s = std::clamp(dot(px-cx,py-cy,cdx,cdy)/len2cd, 0.0f, 1.0f);
        float qx = cx+s*cdx, qy = cy+s*cdy;
        float dxx = px-qx, dyy = py-qy;
        return std::sqrt(dxx*dxx+dyy*dyy);
    };

    auto overlaps = [&](int ax, int ay, int bx, int by,
                        const std::vector<std::pair<int,int>>& wp, int skipEnds) -> bool {
        int n = static_cast<int>(wp.size());
        for (int j = 0; j < n - 1; ++j) {
            if (j < skipEnds && j > 0) continue;
            if (j >= n - 1 - skipEnds) continue;
            auto& a = wp[j]; auto& b = wp[j+1];
            if (segDist(ax,ay,bx,by, a.first,a.second,b.first,b.second) < ROAD_HW*2+6)
                return true;
        }
        return false;
    };

    auto drawLine = [&](int x1, int y1, int x2, int y2) {
        int dx = std::abs(x2-x1), dy = std::abs(y2-y1);
        int sx = (x1<x2)?1:-1, sy = (y1<y2)?1:-1, err = dx-dy;
        int cx = x1, cy = y1;
        while (true) {
            for (int oy=-ROAD_HW; oy<=ROAD_HW; ++oy)
                for (int ox=-ROAD_HW; ox<=ROAD_HW; ++ox) {
                    int px=cx+ox, py=cy+oy;
                    if (px>=1 && px<W-1 && py>=1 && py<H-1)
                        m_map.setGround(px, py, TileType::Road);
                }
            if (cx==x2 && cy==y2) break;
            int e2 = 2*err;
            if (e2 > -dy) { err -= dy; cx += sx; }
            if (e2 < dx)  { err += dx; cy += sy; }
        }
    };

    float centerX = W/2.0f, centerY = H/2.0f;

    for (int attempt = 0; attempt < 100; ++attempt) {
        std::vector<std::pair<int,int>> wp;

        float sa = std::uniform_real_distribution<>(0.0f, 2.0f*PI)(gen);
        float sd = std::uniform_real_distribution<>(150.0f, 250.0f)(gen);
        wp.push_back({std::clamp((int)(centerX + sd*std::cos(sa)), PAD, W-PAD-1),
                      std::clamp((int)(centerY + sd*std::sin(sa)), PAD, H-PAD-1)});

        float angle = sa + PI + std::uniform_real_distribution<>(-0.5f, 0.5f)(gen);

        for (int i = 0; i < 30; ++i) {
            angle += std::uniform_real_distribution<>(-1.2f, 1.2f)(gen);
            float step = std::uniform_real_distribution<>(30.0f, 70.0f)(gen);
            int nx = std::clamp((int)(wp.back().first + step*std::cos(angle)), PAD, W-PAD-1);
            int ny = std::clamp((int)(wp.back().second + step*std::sin(angle)), PAD, H-PAD-1);

            if (overlaps(wp.back().first, wp.back().second, nx, ny, wp, 0)) {
                angle += PI*0.5f;
                continue;
            }
            wp.push_back({nx, ny});
        }

        if (wp.size() < 12) continue;

        auto& last = wp.back();
        auto& first = wp.front();
        float gap = std::sqrt((float)((first.first-last.first)*(first.first-last.first) +
                                      (first.second-last.second)*(first.second-last.second)));

        int closureSteps = std::max(2, (int)(gap / 40.0f));
        bool closureOk = true;
        std::vector<std::pair<int,int>> closure;

        for (int step = 1; step <= closureSteps; ++step) {
            float t = (float)step / closureSteps;
            int mx = (int)(last.first + (first.first - last.first) * t)
                     + std::uniform_int_distribution<>(-3,3)(gen);
            int my = (int)(last.second + (first.second - last.second) * t)
                     + std::uniform_int_distribution<>(-3,3)(gen);
            mx = std::clamp(mx, PAD, W-PAD-1);
            my = std::clamp(my, PAD, H-PAD-1);

            auto& prev = closure.empty() ? last : closure.back();
            if (overlaps(prev.first, prev.second, mx, my, wp, 0) ||
                overlaps(prev.first, prev.second, mx, my, closure, 0)) {
                closureOk = false;
                break;
            }
            closure.push_back({mx, my});
        }

        if (!closureOk) continue;

        auto& closureLast = closure.back();
        if (overlaps(closureLast.first, closureLast.second, first.first, first.second, wp, 0) ||
            overlaps(closureLast.first, closureLast.second, first.first, first.second, closure, 0))
            continue;

        for (auto& c : closure) wp.push_back(c);

        int numWP = (int)wp.size();
        for (int i = 0; i < numWP; ++i)
            drawLine(wp[i].first, wp[i].second, wp[(i+1)%numWP].first, wp[(i+1)%numWP].second);

        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (x==0||x==W-1||y==0||y==H-1) m_map.setGround(x, y, TileType::Wall);

        for (int y = 1; y < H-1; ++y)
            for (int x = 1; x < W-1; ++x)
                if (m_map.getGround(x,y) == TileType::Grass) {
                    bool nr = false;
                    for (int dy=-1; dy<=1 && !nr; ++dy)
                        for (int dx=-1; dx<=1 && !nr; ++dx)
                            if (m_map.getGround(x+dx,y+dy) == TileType::Road) nr = true;
                    if (nr) m_map.setGround(x, y, TileType::Wall);
                }

        int cpStep = std::max(1, numWP / 4);
        for (int i = 0; i < numWP; i += cpStep) {
            auto& p = wp[i];
            auto& pn = wp[(i+1)%numWP];
            float dirX = (float)(pn.first-p.first), dirY = (float)(pn.second-p.second);
            float dirLen = std::sqrt(dirX*dirX+dirY*dirY);
            if (dirLen < 0.001f) continue;
            float perpX = -dirY/dirLen, perpY = dirX/dirLen;
            int hs = ROAD_HW + 2;
            m_map.checkpoints().push_back({
                p.first + (int)(perpX*hs), p.second + (int)(perpY*hs),
                p.first - (int)(perpX*hs), p.second - (int)(perpY*hs)
            });
        }

        float spawnAng = std::atan2((float)(wp[1].second-wp[0].second), (float)(wp[1].first-wp[0].first));
        Spawn sp; sp.x = wp[0].first; sp.y = wp[0].second; sp.angle = spawnAng;
        m_map.spawns().push_back(sp);
        m_map.setLaps(3);
        break;
    }

    m_cars.clear();
    m_botConfigs.clear();
    m_botStates.clear();
    m_carVisuals.clear();
    m_playerIndex = 0;

    CarState playerCar;
    m_cars.push_back(playerCar);
    CarVisual playerVis;
    playerVis.r = CAR_COLORS[0][0];
    playerVis.g = CAR_COLORS[0][1];
    playerVis.b = CAR_COLORS[0][2];
    m_carVisuals.push_back(playerVis);

    spawnBots();
    resetAllCars();
    raceInit(m_race, m_map, static_cast<int>(m_cars.size()));
    m_camera.x = m_cars[0].x;
    m_camera.y = m_cars[0].y;
    m_state = AppState::Playing;
}

void GameApp::spawnBots() {
    int ts = m_map.tileSize();

    for (int i = 0; i < NUM_BOTS; ++i) {
        CarState car;
        int spawnIdx = i + 1;

        if (spawnIdx < static_cast<int>(m_map.spawns().size())) {
            auto& s = m_map.spawns()[spawnIdx];
            car.x = (s.x + 0.5f) * ts;
            car.y = (s.y + 0.5f) * ts;
            car.heading = s.angle;
        } else if (!m_map.spawns().empty()) {
            auto& s = m_map.spawns()[0];
            float cosA = std::cos(s.angle);
            float sinA = std::sin(s.angle);
            float offset = spawnIdx * 20.0f;
            float sideOffset = ((i % 2) == 0 ? -1.0f : 1.0f) * 15.0f;
            car.x = (s.x + 0.5f) * ts - cosA * offset + (-sinA) * sideOffset;
            car.y = (s.y + 0.5f) * ts - sinA * offset + cosA * sideOffset;
            car.heading = s.angle;
        } else {
            car.x = (m_map.width() / 2.0f + i * 2) * ts;
            car.y = m_map.height() / 2.0f * ts;
        }

        m_cars.push_back(car);

        BotConfig cfg;
        cfg.aggression = 0.6f + (i * 0.1f);
        cfg.accuracy = 0.7f + (i * 0.05f);
        m_botConfigs.push_back(cfg);
        m_botStates.push_back(BotState{});

        CarVisual vis;
        vis.r = CAR_COLORS[i + 1][0];
        vis.g = CAR_COLORS[i + 1][1];
        vis.b = CAR_COLORS[i + 1][2];
        m_carVisuals.push_back(vis);
    }
}

void GameApp::resetAllCars() {
    int ts = m_map.tileSize();

    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        if (i < static_cast<int>(m_map.spawns().size())) {
            auto& s = m_map.spawns()[i];
            m_cars[i].x = (s.x + 0.5f) * ts;
            m_cars[i].y = (s.y + 0.5f) * ts;
            m_cars[i].heading = s.angle;
        } else if (!m_map.spawns().empty()) {
            auto& s = m_map.spawns()[0];
            float cosA = std::cos(s.angle);
            float sinA = std::sin(s.angle);
            float offset = i * 20.0f;
            float sideOffset = ((i % 2) == 0 ? -1.0f : 1.0f) * 15.0f;
            m_cars[i].x = (s.x + 0.5f) * ts - cosA * offset + (-sinA) * sideOffset;
            m_cars[i].y = (s.y + 0.5f) * ts - sinA * offset + cosA * sideOffset;
            m_cars[i].heading = s.angle;
        } else {
            m_cars[i].x = (m_map.width() / 2.0f + i * 2) * ts;
            m_cars[i].y = m_map.height() / 2.0f * ts;
            m_cars[i].heading = 0.0f;
        }

        m_cars[i].vx = 0.0f;
        m_cars[i].vy = 0.0f;
        m_cars[i].speed = 0.0f;
        m_cars[i].forwardSpeed = 0.0f;
    }

    for (auto& vis : m_carVisuals)
        vis.trails.clear();

    m_lastPlayerCheckpoint = 0;
    m_checkpointFlash = 0.0f;
}

void GameApp::loadMap(const std::string& path) {
    MapData loaded;
    if (MapSerializer::loadFromFile(loaded, path)) {
        m_map = std::move(loaded);
    } else {
        fprintf(stderr, "Failed to load map: %s\n", path.c_str());
        generateFallbackMap();
    }

    m_cars.clear();
    m_botConfigs.clear();
    m_botStates.clear();
    m_carVisuals.clear();
    m_playerIndex = 0;

    CarState playerCar;
    m_cars.push_back(playerCar);
    CarVisual playerVis;
    playerVis.r = CAR_COLORS[0][0];
    playerVis.g = CAR_COLORS[0][1];
    playerVis.b = CAR_COLORS[0][2];
    m_carVisuals.push_back(playerVis);

    spawnBots();
    resetAllCars();
    raceInit(m_race, m_map, static_cast<int>(m_cars.size()));
    m_camera.x = m_cars[0].x;
    m_camera.y = m_cars[0].y;
    m_state = AppState::Playing;
}

void GameApp::loadFallback() {
    generateFallbackMap();

    m_cars.clear();
    m_botConfigs.clear();
    m_botStates.clear();
    m_carVisuals.clear();
    m_playerIndex = 0;

    CarState playerCar;
    m_cars.push_back(playerCar);
    CarVisual playerVis;
    playerVis.r = CAR_COLORS[0][0];
    playerVis.g = CAR_COLORS[0][1];
    playerVis.b = CAR_COLORS[0][2];
    m_carVisuals.push_back(playerVis);

    spawnBots();
    resetAllCars();
    raceInit(m_race, m_map, static_cast<int>(m_cars.size()));
    m_camera.x = m_cars[0].x;
    m_camera.y = m_cars[0].y;
    m_state = AppState::Playing;
}

void GameApp::scanMaps() {
    m_maps.clear();

    std::vector<std::string> searchDirs = {
        getExeDir(),
    };

    for (auto& dir : searchDirs) {
        try {
            if (!fs::exists(dir)) continue;
            for (auto& entry : fs::directory_iterator(dir)) {
                if (entry.path().extension() == ".json") {
                    MapEntry me;
                    me.name = entry.path().stem().string();
                    me.path = entry.path().string();
                    m_maps.push_back(me);
                }
            }
        } catch (const std::exception&) {}
    }
}

bool GameApp::init(SDL_Renderer* renderer, const char* mapPath) {
    m_renderer = renderer;
    m_input.init();

    if (enet_initialize() == 0) {
        m_net.init();
        m_net.setOnMapReceived([this](const std::string& data) { onMapReceived(data); });
        m_net.setOnState([this]() { onStateReceived(); });
        m_netInitialized = true;
    }

    m_masterUrl = mm::net::DEFAULT_MASTER;
    m_mapDir = getExeDir();
    scanMaps();

    if (mapPath) {
        loadMap(mapPath);
    } else {
        m_state = AppState::Menu;
    }

    return true;
}

void GameApp::shutdown() {
    m_net.shutdown();
    if (m_netInitialized) enet_deinitialize();
    m_input.shutdown();
}

void GameApp::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        m_quit = true;
        return;
    }

    m_input.handleEvent(e);

    if (m_state == AppState::Playing && e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_r) {
            resetAllCars();
            raceInit(m_race, m_map, static_cast<int>(m_cars.size()));
        }
        if (e.key.keysym.sym == SDLK_ESCAPE)
            m_state = AppState::Menu;
    }

    if (m_state == AppState::MultiplayerPlaying && e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
            m_net.disconnect();
            m_mpMapLoaded = false;
            m_state = AppState::Menu;
        }
    }

    if (m_state == AppState::MultiplayerLobby && e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE)
            m_state = AppState::Menu;
    }
}

void GameApp::updateTrails(float dt) {
    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        auto& car = m_cars[i];
        auto& vis = m_carVisuals[i];

        if (car.speed > 50.0f) {
            TrailPoint tp;
            tp.x = car.x;
            tp.y = car.y;
            tp.age = 0.0f;
            vis.trails.push_back(tp);
        }

        for (auto it = vis.trails.begin(); it != vis.trails.end();) {
            it->age += dt;
            if (it->age > 2.0f)
                it = vis.trails.erase(it);
            else
                ++it;
        }
    }
}

void GameApp::update(float dt) {
    if (m_state == AppState::Menu || m_state == AppState::MultiplayerLobby) return;

    if (m_state == AppState::MultiplayerPlaying) {
        m_net.update();
        PlayerInput playerIn = m_input.poll();
        m_net.sendInput(playerIn);

        updateTrails(dt);

        if (m_mpMapLoaded && m_playerIndex < static_cast<int>(m_cars.size())) {
            int sw, sh;
            SDL_GetRendererOutputSize(m_renderer, &sw, &sh);
            m_camera.zoom = std::min(sw, sh) / 350.0f;
            m_camera.update(m_cars[m_playerIndex].x, m_cars[m_playerIndex].y, dt);

            int ts = m_map.tileSize();
            for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
                int carTX = static_cast<int>(std::floor(m_cars[i].x / ts));
                int carTY = static_cast<int>(std::floor(m_cars[i].y / ts));
                TileType obj = m_map.getObject(carTX, carTY);
                TileType ground = m_map.getGround(carTX, carTY);
                if (obj == TileType::Ramp || ground == TileType::Ramp)
                    m_carVisuals[i].hopScale = 1.6f;
                else
                    m_carVisuals[i].hopScale += (1.0f - m_carVisuals[i].hopScale) * 8.0f * dt;
            }
        }
        return;
    }
    PlayerInput playerIn = m_input.poll();

    int prevCheckpoint = m_race.racers[m_playerIndex].currentCheckpoint;

    std::vector<CarPosition> carPositions;
    for (auto& car : m_cars) {
        CarPosition cp;
        cp.x = car.x;
        cp.y = car.y;
        cp.prevX = car.x;
        cp.prevY = car.y;
        carPositions.push_back(cp);
    }

    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        if (m_race.state == RaceState::Racing) {
            if (i == m_playerIndex) {
                carUpdate(m_cars[i], playerIn, m_carCfg, m_map, dt);
            } else {
                PlayerInput botIn = botComputeInput(m_cars[i], m_race.racers[i], m_map, m_botConfigs[i - 1], m_botStates[i - 1], dt);
                carUpdate(m_cars[i], botIn, m_carCfg, m_map, dt);
            }
        } else if (m_race.state == RaceState::Countdown) {
            carUpdate(m_cars[i], PlayerInput{}, m_carCfg, m_map, dt);
        }
        carPositions[i].x = m_cars[i].x;
        carPositions[i].y = m_cars[i].y;
    }

    raceUpdate(m_race, carPositions, m_map, dt);

    int newCheckpoint = m_race.racers[m_playerIndex].currentCheckpoint;
    if (newCheckpoint != prevCheckpoint && m_race.state == RaceState::Racing) {
        m_checkpointFlash = 0.3f;
    }
    m_lastPlayerCheckpoint = newCheckpoint;

    if (m_checkpointFlash > 0.0f)
        m_checkpointFlash -= dt;

    updateTrails(dt);

    {
        int sw, sh;
        SDL_GetRendererOutputSize(m_renderer, &sw, &sh);
        m_camera.zoom = std::min(sw, sh) / 350.0f;
    }

    int ts = m_map.tileSize();
    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        int carTX = static_cast<int>(std::floor(m_cars[i].x / ts));
        int carTY = static_cast<int>(std::floor(m_cars[i].y / ts));
        TileType obj = m_map.getObject(carTX, carTY);
        TileType ground = m_map.getGround(carTX, carTY);
        if (obj == TileType::Ramp || ground == TileType::Ramp) {
            m_carVisuals[i].hopScale = 1.6f;
        } else {
            m_carVisuals[i].hopScale += (1.0f - m_carVisuals[i].hopScale) * 8.0f * dt;
        }
    }

    m_camera.update(m_cars[m_playerIndex].x, m_cars[m_playerIndex].y, dt);
}

void GameApp::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
    SDL_RenderClear(renderer);

    ImGui::NewFrame();

    if (m_state == AppState::Menu) {
        renderMenu(renderer);
    } else if (m_state == AppState::MultiplayerLobby) {
        renderServerBrowser(renderer);
    } else {
        renderTiles(renderer);
        renderTrails(renderer);
        renderCars(renderer);
        renderHUD(renderer);
        renderMinimap(renderer);

        if (m_race.state == RaceState::Finished) {
            renderFinishScreen(renderer);
        }

        if (m_checkpointFlash > 0.0f) {
            int sw, sh;
            SDL_GetRendererOutputSize(renderer, &sw, &sh);
            Uint8 alpha = static_cast<Uint8>(m_checkpointFlash / 0.3f * 60);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 200, 0, alpha);
            SDL_Rect full = {0, 0, sw, sh};
            SDL_RenderFillRect(renderer, &full);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void GameApp::renderMenu(SDL_Renderer* renderer) {
    int sw, sh;
    SDL_GetRendererOutputSize(renderer, &sw, &sh);

    float menuW = 500.0f;
    ImGui::SetNextWindowPos(ImVec2(sw / 2.0f, sh / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(menuW, 0), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::Begin("##menu", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);

    ImGui::SetWindowFontScale(2.5f);
    float titleW = ImGui::CalcTextSize("MiniMachines").x;
    ImGui::SetCursorPosX((menuW - titleW) / 2.0f);
    ImGui::Text("MiniMachines");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Select a map:");
    ImGui::Spacing();

    ImGui::BeginChild("map_list", ImVec2(0, 200), true);
    for (int i = 0; i < static_cast<int>(m_maps.size()); ++i) {
        bool selected = (i == m_selectedMap);
        if (ImGui::Selectable(m_maps[i].name.c_str(), selected,
                              ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 30))) {
            m_selectedMap = i;
            if (ImGui::IsMouseDoubleClicked(0))
                loadMap(m_maps[m_selectedMap].path);
        }
        if (selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndChild();

    ImGui::Spacing();

    if (!m_maps.empty()) {
        if (ImGui::Button("Play", ImVec2(-1, 50)))
            loadMap(m_maps[m_selectedMap].path);
        ImGui::Spacing();
    }

    if (ImGui::Button("Fallback Track", ImVec2(-1, 50)))
        loadFallback();
    ImGui::Spacing();

    if (ImGui::Button("Random Track", ImVec2(-1, 50)))
        loadRandomMap();
    ImGui::Spacing();

    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Multiplayer", ImVec2(-1, 50)))
        m_state = AppState::MultiplayerLobby;
    ImGui::Spacing();

    if (ImGui::Button("Quit", ImVec2(-1, 40)))
        m_quit = true;

    ImGui::Separator();
    ImGui::Text("D-Pad: Navigate | A: Select | B: Back");

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void GameApp::connectToServer(const std::string& ip, int port) {
    m_connectStatus = "Connecting...";
    if (m_net.connect(ip, port)) {
        m_state = AppState::MultiplayerPlaying;
        m_mpMapLoaded = false;
        m_connectStatus = "Connected!";
    } else {
        m_connectStatus = "Connection failed.";
    }
}

void GameApp::onMapReceived(const std::string& mapData) {
    MapData loaded;
    std::string tmpPath = getExeDir() + "\\_net_map.json";
    FILE* f = fopen(tmpPath.c_str(), "wb");
    if (f) {
        fwrite(mapData.data(), 1, mapData.size(), f);
        fclose(f);
        if (MapSerializer::loadFromFile(loaded, tmpPath)) {
            m_map = std::move(loaded);
            m_cars.clear();
            m_carVisuals.clear();
            for (int i = 0; i < mm::net::MAX_PLAYERS; ++i) {
                CarState car;
                m_cars.push_back(car);
                CarVisual vis;
                vis.r = CAR_COLORS[i % 4][0];
                vis.g = CAR_COLORS[i % 4][1];
                vis.b = CAR_COLORS[i % 4][2];
                m_carVisuals.push_back(vis);
            }
            m_race = RaceData();
            m_camera.x = m_cars[0].x;
            m_camera.y = m_cars[0].y;
            m_mpMapLoaded = true;
        }
        remove(tmpPath.c_str());
    }
}

void GameApp::onStateReceived() {
    auto& netCars = m_net.getCarStates();
    auto& netRace = m_net.getRaceData();
    m_playerIndex = m_net.getPlayerIndex();

    for (int i = 0; i < static_cast<int>(netCars.size()) && i < static_cast<int>(m_cars.size()); ++i) {
        m_cars[i] = netCars[i];
    }
    m_race.state = netRace.state;
    m_race.raceTime = netRace.raceTime;
    m_race.totalLaps = netRace.totalLaps;
    m_race.racers = netRace.racers;

    if (m_mpMapLoaded && m_playerIndex < static_cast<int>(m_cars.size())) {
        m_camera.update(m_cars[m_playerIndex].x, m_cars[m_playerIndex].y, 1.0f / 30.0f);
    }
}

void GameApp::renderServerBrowser(SDL_Renderer* renderer) {
    int sw, sh;
    SDL_GetRendererOutputSize(renderer, &sw, &sh);

    ImGui::SetNextWindowPos(ImVec2(sw / 2.0f, sh / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Multiplayer", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Server Browser");
    ImGui::Separator();

    ImGui::InputText("Master Server", (char*)m_masterUrl.c_str(), m_masterUrl.size() + 1,
                     ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        m_serverList = m_net.queryMaster(m_masterUrl);
        m_selectedServer = -1;
        m_hasPreview = false;
    }

    ImGui::Separator();
    ImGui::Text("Servers:");

    ImGui::BeginChild("server_browser_area", ImVec2(0, -80), false);

    ImGui::BeginChild("server_list", ImVec2(300, 0), true);
    for (int i = 0; i < static_cast<int>(m_serverList.size()); ++i) {
        auto& s = m_serverList[i];
        char label[256];
        snprintf(label, sizeof(label), "%s:%d\n%s\n%d/%d players",
                 s.ip.c_str(), s.port, s.mapName.c_str(), s.players, s.maxPlayers);
        bool selected = (i == m_selectedServer);
        if (ImGui::Selectable(label, selected)) {
            m_selectedServer = i;
            snprintf(m_directIp, sizeof(m_directIp), "%s", s.ip.c_str());
            snprintf(m_directPort, sizeof(m_directPort), "%d", s.port);

            if (!s.mapData.empty()) {
                std::string tmpPath = getExeDir() + "\\_preview_map.json";
                FILE* f = fopen(tmpPath.c_str(), "wb");
                if (f) {
                    fwrite(s.mapData.data(), 1, s.mapData.size(), f);
                    fclose(f);
                    if (MapSerializer::loadFromFile(m_previewMap, tmpPath))
                        m_hasPreview = true;
                    else
                        m_hasPreview = false;
                    remove(tmpPath.c_str());
                }
            } else {
                m_hasPreview = false;
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("map_preview", ImVec2(0, 0), true);
    if (m_hasPreview) {
        ImGui::Text("Map Preview: %s", m_previewMap.name().c_str());
        ImGui::Text("Size: %dx%d", m_previewMap.width(), m_previewMap.height());

        ImVec2 avail = ImGui::GetContentRegionAvail();
        int previewSize = static_cast<int>(std::min(avail.x, avail.y));
        if (previewSize < 50) previewSize = 50;

        int mapW = m_previewMap.width();
        int mapH = m_previewMap.height();
        float scale = (float)previewSize / std::max(mapW, mapH);
        int drawW = (int)(mapW * scale);
        int drawH = (int)(mapH * scale);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        drawList->AddRectFilled(cursor, ImVec2(cursor.x + drawW, cursor.y + drawH),
                                IM_COL32(0, 0, 0, 255));

        int step = std::max(1, mapW / drawW);
        for (int ty = 0; ty < mapH; ty += step) {
            for (int tx = 0; tx < mapW; tx += step) {
                TileType ground = m_previewMap.getGround(tx, ty);
                TileType obj = m_previewMap.getObject(tx, ty);
                TileType display = (obj != TileType::Empty) ? obj : ground;
                const TileInfo* info = findTileInfo(display);
                if (!info) continue;

                int px = (int)(tx * scale);
                int py = (int)(ty * scale);
                int ps = std::max(1, (int)(step * scale));

                ImU32 col = IM_COL32(info->r, info->g, info->b, 255);
                drawList->AddRectFilled(
                    ImVec2(cursor.x + px, cursor.y + py),
                    ImVec2(cursor.x + px + ps, cursor.y + py + ps),
                    col);
            }
        }

        ImGui::Dummy(ImVec2(drawW, drawH));
    } else {
        ImGui::Text("Select a server to preview its map");
    }
    ImGui::EndChild();

    ImGui::EndChild();

    ImGui::Separator();
    ImGui::PushItemWidth(200);
    ImGui::InputText("IP", m_directIp, sizeof(m_directIp));
    ImGui::SameLine();
    ImGui::PushItemWidth(80);
    ImGui::InputText("Port", m_directPort, sizeof(m_directPort));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Connect")) {
        connectToServer(m_directIp, atoi(m_directPort));
    }
    ImGui::SameLine();
    if (ImGui::Button("Back"))
        m_state = AppState::Menu;
    ImGui::PopItemWidth();

    if (!m_connectStatus.empty()) {
        ImGui::Text("%s", m_connectStatus.c_str());
    }

    ImGui::End();
}

void GameApp::renderTiles(SDL_Renderer* r) {
    int sw, sh;
    SDL_GetRendererOutputSize(r, &sw, &sh);

    int ts = m_map.tileSize();
    float zoom = m_camera.zoom;

    float worldW = sw / zoom;
    float worldH = sh / zoom;

    int startX = static_cast<int>(std::floor((m_camera.x - worldW / 2) / ts)) - 1;
    int startY = static_cast<int>(std::floor((m_camera.y - worldH / 2) / ts)) - 1;
    int endX = static_cast<int>(std::ceil((m_camera.x + worldW / 2) / ts)) + 1;
    int endY = static_cast<int>(std::ceil((m_camera.y + worldH / 2) / ts)) + 1;

    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (endX > m_map.width()) endX = m_map.width();
    if (endY > m_map.height()) endY = m_map.height();

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            TileType ground = m_map.getGround(x, y);
            TileType obj = m_map.getObject(x, y);
            TileType display = (obj != TileType::Empty) ? obj : ground;
            const TileInfo* info = findTileInfo(display);
            if (!info) continue;

            SDL_Point sp = m_camera.worldToScreen(x * ts, y * ts);
            sp.x += sw / 2;
            sp.y += sh / 2;
            int w = static_cast<int>(ts * zoom);
            int h = static_cast<int>(ts * zoom);

            SDL_Rect dst = {sp.x, sp.y, w, h};
            SDL_SetRenderDrawColor(r, info->r, info->g, info->b, 255);
            SDL_RenderFillRect(r, &dst);
        }
    }

    SDL_SetRenderDrawColor(r, 0, 0, 0, 60);
    for (int x = startX; x <= endX; ++x) {
        SDL_Point sp = m_camera.worldToScreen(x * ts, startY * ts);
        sp.x += sw / 2; sp.y += sh / 2;
        SDL_Point ep = m_camera.worldToScreen(x * ts, endY * ts);
        ep.x += sw / 2; ep.y += sh / 2;
        SDL_RenderDrawLine(r, sp.x, sp.y, ep.x, ep.y);
    }
    for (int y = startY; y <= endY; ++y) {
        SDL_Point sp = m_camera.worldToScreen(startX * ts, y * ts);
        sp.x += sw / 2; sp.y += sh / 2;
        SDL_Point ep = m_camera.worldToScreen(endX * ts, y * ts);
        ep.x += sw / 2; ep.y += sh / 2;
        SDL_RenderDrawLine(r, sp.x, sp.y, ep.x, ep.y);
    }

    for (auto& cp : m_map.checkpoints()) {
        int dx = std::abs(cp.x2 - cp.x1);
        int dy = std::abs(cp.y2 - cp.y1);
        int sx = (cp.x1 < cp.x2) ? 1 : -1;
        int sy = (cp.y1 < cp.y2) ? 1 : -1;
        int err = dx - dy;
        int cx = cp.x1, cy = cp.y1;

        while (true) {
            SDL_Point sp = m_camera.worldToScreen(cx * ts, cy * ts);
            sp.x += sw / 2; sp.y += sh / 2;
            int w = static_cast<int>(ts * zoom);
            SDL_Rect dst = {sp.x, sp.y, w, w};
            SDL_SetRenderDrawColor(r, 255, 140, 0, 80);
            SDL_RenderFillRect(r, &dst);
            SDL_SetRenderDrawColor(r, 255, 200, 0, 180);
            SDL_RenderDrawRect(r, &dst);

            if (cx == cp.x2 && cy == cp.y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; cx += sx; }
            if (e2 < dx)  { err += dx; cy += sy; }
        }
    }
}

void GameApp::renderTrails(SDL_Renderer* r) {
    int sw, sh;
    SDL_GetRendererOutputSize(r, &sw, &sh);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        auto& vis = m_carVisuals[i];
        for (auto& tp : vis.trails) {
            SDL_Point sp = m_camera.worldToScreen(tp.x, tp.y);
            sp.x += sw / 2;
            sp.y += sh / 2;
            Uint8 alpha = static_cast<Uint8>((1.0f - tp.age / 2.0f) * 100);
            SDL_SetRenderDrawColor(r, vis.r, vis.g, vis.b, alpha);
            int size = 3;
            SDL_Rect dst = {sp.x - size, sp.y - size, size * 2, size * 2};
            SDL_RenderFillRect(r, &dst);
        }
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void GameApp::renderCars(SDL_Renderer* r) {
    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        renderCar(r, i);
    }
}

void GameApp::renderCar(SDL_Renderer* r, int idx) {
    int sw, sh;
    SDL_GetRendererOutputSize(r, &sw, &sh);

    auto& car = m_cars[idx];
    auto& vis = m_carVisuals[idx];

    SDL_Point sp = m_camera.worldToScreen(car.x, car.y);
    sp.x += sw / 2;
    sp.y += sh / 2;

    float carR = m_carCfg.radius * m_camera.zoom * vis.hopScale;
    float cosH = std::cos(car.heading);
    float sinH = std::sin(car.heading);

    auto rotate = [&](float lx, float ly) -> SDL_Vertex {
        SDL_Vertex v;
        v.position.x = sp.x + (lx * cosH - ly * sinH);
        v.position.y = sp.y + (lx * sinH + ly * cosH);
        v.color = {vis.r, vis.g, vis.b, 255};
        v.tex_coord = {0, 0};
        return v;
    };

    SDL_Vertex verts[3] = {
        rotate(carR, 0),
        rotate(-carR * 0.7f, -carR * 0.5f),
        rotate(-carR * 0.7f, carR * 0.5f),
    };

    SDL_RenderGeometry(r, nullptr, verts, 3, nullptr, 0);
}

void GameApp::renderHUD(SDL_Renderer* renderer) {
    int sw, sh;
    SDL_GetRendererOutputSize(renderer, &sw, &sh);

    int safeMargin = std::max(60, sh / 20);

    ImGui::SetNextWindowPos(ImVec2(safeMargin, safeMargin), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##hud", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);

    ImGui::Text("Map: %s", m_map.name().c_str());
    ImGui::Separator();

    if (m_race.state == RaceState::Countdown) {
        ImGui::Text("Starting in: %.0f", std::ceil(m_race.countdown));
    } else if (m_race.state == RaceState::Racing) {
        auto& playerRacer = m_race.racers[m_playerIndex];
        int pos = raceGetPosition(m_race, m_playerIndex);
        const char* posStr = "th";
        if (pos == 1) posStr = "st";
        else if (pos == 2) posStr = "nd";
        else if (pos == 3) posStr = "rd";

        ImGui::Text("Position: %d%s / %d", pos, posStr, static_cast<int>(m_cars.size()));
        ImGui::Text("Lap: %d / %d", playerRacer.currentLap + 1, m_race.totalLaps);
        ImGui::Text("Checkpoint: %d / %d", playerRacer.currentCheckpoint,
                    static_cast<int>(m_map.checkpoints().size()));
        ImGui::Text("Time: %.2f", m_race.raceTime);
        if (playerRacer.bestLap < 1e6f)
            ImGui::Text("Best Lap: %.2f", playerRacer.bestLap);
        ImGui::Text("Speed: %.0f", m_cars[m_playerIndex].speed);

        ImGui::Separator();
        for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
            auto& r = m_race.racers[i];
            auto& vis = m_carVisuals[i];
            ImVec4 col(vis.r / 255.0f, vis.g / 255.0f, vis.b / 255.0f, 1.0f);
            if (r.finished) {
                ImGui::TextColored(col, "%s: FINISHED (%.2f)", i == 0 ? "You" : "Bot", r.finishTime);
            } else {
                ImGui::TextColored(col, "%s: Lap %d, CP %d", i == 0 ? "You" : "Bot",
                                   r.currentLap + 1, r.currentCheckpoint);
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("R = Reset | ESC = Menu | WASD/Arrows = Drive");

    ImGui::End();

    if (m_race.state == RaceState::Countdown && m_race.countdown > 0.0f) {
        ImGui::SetNextWindowPos(ImVec2(sw / 2.0f, sh / 2.0f - 40), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("##countdown", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav);
        ImGui::PushFont(nullptr);
        ImGui::SetWindowFontScale(4.0f);
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "%d", static_cast<int>(std::ceil(m_race.countdown)));
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

void GameApp::renderMinimap(SDL_Renderer* r) {
    int sw, sh;
    SDL_GetRendererOutputSize(r, &sw, &sh);

    int mapW = m_map.width();
    int mapH = m_map.height();
    int minimapSize = 180;
    float scale = (float)minimapSize / std::max(mapW, mapH);
    int drawW = (int)(mapW * scale);
    int drawH = (int)(mapH * scale);
    int safeMargin = std::max(60, sh / 20);
    int ox = sw - drawW - safeMargin;
    int oy = sh - drawH - safeMargin;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_Rect bg = {ox - 2, oy - 2, drawW + 4, drawH + 4};
    SDL_RenderFillRect(r, &bg);

    int step = std::max(1, mapW / drawW);
    for (int ty = 0; ty < mapH; ty += step) {
        for (int tx = 0; tx < mapW; tx += step) {
            TileType ground = m_map.getGround(tx, ty);
            TileType obj = m_map.getObject(tx, ty);
            TileType display = (obj != TileType::Empty) ? obj : ground;
            const TileInfo* info = findTileInfo(display);
            if (!info) continue;

            SDL_SetRenderDrawColor(r, info->r, info->g, info->b, 200);
            int px = ox + (int)(tx * scale);
            int py = oy + (int)(ty * scale);
            int ps = std::max(1, (int)(step * scale));
            SDL_Rect dst = {px, py, ps, ps};
            SDL_RenderFillRect(r, &dst);
        }
    }

    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        auto& car = m_cars[i];
        auto& vis = m_carVisuals[i];
        int cx = ox + (int)(car.x / m_map.tileSize() * scale);
        int cy = oy + (int)(car.y / m_map.tileSize() * scale);
        int dotSize = (i == m_playerIndex) ? 4 : 3;

        SDL_SetRenderDrawColor(r, vis.r, vis.g, vis.b, 255);
        SDL_Rect dot = {cx - dotSize/2, cy - dotSize/2, dotSize, dotSize};
        SDL_RenderFillRect(r, &dot);

        if (i == m_playerIndex) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
            SDL_RenderDrawRect(r, &dot);
        }
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void GameApp::renderFinishScreen(SDL_Renderer* renderer) {
    int sw, sh;
    SDL_GetRendererOutputSize(renderer, &sw, &sh);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_Rect full = {0, 0, sw, sh};
    SDL_RenderFillRect(renderer, &full);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    ImGui::SetNextWindowPos(ImVec2(sw / 2.0f, sh / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::Begin("##finish", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

    ImGui::SetWindowFontScale(2.0f);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "RACE COMPLETE");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    int playerPos = raceGetPosition(m_race, m_playerIndex);
    const char* posStr = "th";
    if (playerPos == 1) posStr = "st";
    else if (playerPos == 2) posStr = "nd";
    else if (playerPos == 3) posStr = "rd";

    ImGui::Text("You finished: %d%s", playerPos, posStr);
    ImGui::Text("Total time: %.2f", m_race.racers[m_playerIndex].finishTime);
    if (m_race.racers[m_playerIndex].bestLap < 1e6f)
        ImGui::Text("Best lap: %.2f", m_race.racers[m_playerIndex].bestLap);
    ImGui::Separator();

    for (int i = 0; i < static_cast<int>(m_cars.size()); ++i) {
        auto& r = m_race.racers[i];
        auto& vis = m_carVisuals[i];
        ImVec4 col(vis.r / 255.0f, vis.g / 255.0f, vis.b / 255.0f, 1.0f);
        ImGui::TextColored(col, "%d. %s - %.2f", i + 1, i == 0 ? "You" : "Bot", r.finishTime);
    }

    ImGui::Separator();
    if (ImGui::Button("Back to Menu", ImVec2(330, 30)))
        m_state = AppState::Menu;
    if (ImGui::Button("Race Again", ImVec2(330, 30))) {
        resetAllCars();
        raceInit(m_race, m_map, static_cast<int>(m_cars.size()));
    }

    ImGui::End();
}

} // namespace mm
