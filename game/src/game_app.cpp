#include "game_app.h"
#include "core/map_serializer.h"
#include "core/collision.h"
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <filesystem>

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
        getExeDir() + "\\..\\editor",
        getExeDir() + "\\..\\..\\assets\\maps",
    };

    for (auto& dir : searchDirs) {
        if (!fs::exists(dir)) continue;
        for (auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() == ".json") {
                MapEntry me;
                me.name = entry.path().stem().string();
                me.path = entry.path().string();
                m_maps.push_back(me);
            }
        }
    }
}

bool GameApp::init(SDL_Renderer* renderer, const char* mapPath) {
    m_renderer = renderer;
    m_input.init();

    m_mapDir = getExeDir();
    scanMaps();

    if (mapPath) {
        loadMap(mapPath);
    } else if (!m_maps.empty()) {
        m_state = AppState::Menu;
    } else {
        loadFallback();
    }

    return true;
}

void GameApp::shutdown() {
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
    if (m_state == AppState::Menu) return;

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
                PlayerInput botIn = botComputeInput(m_cars[i], m_race.racers[i], m_map, m_botConfigs[i - 1]);
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

    m_camera.zoom = 2.0f;
    m_camera.update(m_cars[m_playerIndex].x, m_cars[m_playerIndex].y, dt);
}

void GameApp::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
    SDL_RenderClear(renderer);

    ImGui::NewFrame();

    if (m_state == AppState::Menu) {
        renderMenu(renderer);
    } else {
        renderTiles(renderer);
        renderTrails(renderer);
        renderCars(renderer);
        renderHUD(renderer);

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

    ImGui::SetNextWindowPos(ImVec2(sw / 2.0f, sh / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Always);
    ImGui::Begin("##menu", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("MiniMachines");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    ImGui::Text("Select a map:");
    ImGui::Separator();

    for (int i = 0; i < static_cast<int>(m_maps.size()); ++i) {
        bool selected = (i == m_selectedMap);
        if (ImGui::Selectable(m_maps[i].name.c_str(), selected))
            m_selectedMap = i;
    }

    if (!m_maps.empty()) {
        ImGui::Separator();
        if (ImGui::Button("Play", ImVec2(380, 40)))
            loadMap(m_maps[m_selectedMap].path);
    }

    ImGui::Separator();
    if (ImGui::Button("Fallback Track", ImVec2(380, 40)))
        loadFallback();

    ImGui::Separator();
    if (ImGui::Button("Quit", ImVec2(380, 30)))
        m_quit = true;

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

    float carR = m_carCfg.radius * m_camera.zoom;
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

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
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
