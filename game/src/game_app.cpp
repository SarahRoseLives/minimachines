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

void GameApp::resetCar() {
    float spawnX = m_map.width() / 2.0f;
    float spawnY = m_map.height() / 2.0f;
    float spawnAngle = 0.0f;
    bool found = false;

    // Check spawn points first (they have angle data)
    if (!m_map.spawns().empty()) {
        auto& s = m_map.spawns()[0];
        spawnX = s.x + 0.5f;
        spawnY = s.y + 0.5f;
        spawnAngle = s.angle;
        found = true;
    }

    // Check objects layer for Start tile
    if (!found) {
        for (int y = 0; y < m_map.height() && !found; ++y) {
            for (int x = 0; x < m_map.width() && !found; ++x) {
                if (m_map.getObject(x, y) == TileType::Start) {
                    spawnX = x + 0.5f;
                    spawnY = y + 0.5f;
                    found = true;
                }
            }
        }
    }

    // Check ground layer for Start tile
    if (!found) {
        for (int y = 0; y < m_map.height() && !found; ++y) {
            for (int x = 0; x < m_map.width() && !found; ++x) {
                if (m_map.getGround(x, y) == TileType::Start) {
                    spawnX = x + 0.5f;
                    spawnY = y + 0.5f;
                    found = true;
                }
            }
        }
    }

    // Fall back to first Road tile
    if (!found) {
        for (int y = 0; y < m_map.height() && !found; ++y) {
            for (int x = 0; x < m_map.width() && !found; ++x) {
                if (m_map.getGround(x, y) == TileType::Road) {
                    spawnX = x + 0.5f;
                    spawnY = y + 0.5f;
                    found = true;
                }
            }
        }
    }

    m_car.x = spawnX * m_map.tileSize();
    m_car.y = spawnY * m_map.tileSize();
    m_car.heading = spawnAngle;
    m_car.vx = 0.0f;
    m_car.vy = 0.0f;
    m_car.speed = 0.0f;
    m_car.forwardSpeed = 0.0f;
    m_prevCarX = m_car.x;
    m_prevCarY = m_car.y;
}

void GameApp::loadMap(const std::string& path) {
    MapData loaded;
    if (MapSerializer::loadFromFile(loaded, path)) {
        m_map = std::move(loaded);
    } else {
        fprintf(stderr, "Failed to load map: %s\n", path.c_str());
        generateFallbackMap();
    }

    resetCar();
    raceInit(m_race, m_map);
    m_camera.x = m_car.x;
    m_camera.y = m_car.y;
    m_state = AppState::Playing;
}

void GameApp::loadFallback() {
    generateFallbackMap();
    resetCar();
    raceInit(m_race, m_map);
    m_camera.x = m_car.x;
    m_camera.y = m_car.y;
    m_state = AppState::Playing;
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
        if (e.key.keysym.sym == SDLK_r)
            resetCar();
        if (e.key.keysym.sym == SDLK_ESCAPE)
            m_state = AppState::Menu;
    }
}

void GameApp::update(float dt) {
    if (m_state == AppState::Menu) return;

    PlayerInput in = m_input.poll();
    m_prevCarX = m_car.x;
    m_prevCarY = m_car.y;

    if (m_race.state == RaceState::Racing) {
        carUpdate(m_car, in, m_carCfg, m_map, dt);
    } else if (m_race.state == RaceState::Countdown) {
        carUpdate(m_car, PlayerInput{}, m_carCfg, m_map, dt);
    }

    raceUpdate(m_race, m_car.x, m_car.y, m_map, dt, m_prevCarX, m_prevCarY);

    m_camera.zoom = 2.0f;
    m_camera.update(m_car.x, m_car.y, dt);
}

void GameApp::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
    SDL_RenderClear(renderer);

    ImGui::NewFrame();

    if (m_state == AppState::Menu) {
        renderMenu(renderer);
    } else {
        renderTiles(renderer);
        renderCar(renderer);
        renderHUD(renderer);
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

void GameApp::renderCar(SDL_Renderer* r) {
    int sw, sh;
    SDL_GetRendererOutputSize(r, &sw, &sh);

    SDL_Point sp = m_camera.worldToScreen(m_car.x, m_car.y);
    sp.x += sw / 2;
    sp.y += sh / 2;

    float carR = m_carCfg.radius * m_camera.zoom;
    float cosH = std::cos(m_car.heading);
    float sinH = std::sin(m_car.heading);

    auto rotate = [&](float lx, float ly) -> SDL_Vertex {
        SDL_Vertex v;
        v.position.x = sp.x + (lx * cosH - ly * sinH);
        v.position.y = sp.y + (lx * sinH + ly * cosH);
        v.color = {51, 153, 255, 255};
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
        ImGui::Text("Lap: %d / %d", m_race.currentLap + 1, m_race.totalLaps);
        ImGui::Text("Checkpoint: %d / %d", m_race.currentCheckpoint,
                    static_cast<int>(m_map.checkpoints().size()));
        ImGui::Text("Time: %.2f", m_race.raceTime);
        if (m_race.bestLap < 1e6f)
            ImGui::Text("Best Lap: %.2f", m_race.bestLap);
        ImGui::Text("Speed: %.0f", m_car.speed);
    } else if (m_race.state == RaceState::Finished) {
        ImGui::Text("FINISHED!");
        ImGui::Text("Total: %.2f", m_race.raceTime);
        if (m_race.bestLap < 1e6f)
            ImGui::Text("Best Lap: %.2f", m_race.bestLap);
    }

    ImGui::Separator();
    ImGui::Text("R = Reset | ESC = Menu | Arrows/WASD = Drive");

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

} // namespace mm
