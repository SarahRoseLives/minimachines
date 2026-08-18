#pragma once

#include "core/car.h"
#include "core/bot.h"
#include "core/map_data.h"
#include "core/race.h"
#include "camera.h"
#include "input.h"
#include <SDL.h>
#include <string>
#include <vector>

namespace mm {

struct MapEntry {
    std::string name;
    std::string path;
};

enum class AppState {
    Menu,
    Playing,
};

struct TrailPoint {
    float x, y;
    float age;
};

struct CarVisual {
    Uint8 r, g, b;
    std::vector<TrailPoint> trails;
};

class GameApp {
public:
    bool init(SDL_Renderer* renderer, const char* mapPath);
    void shutdown();
    void handleEvent(const SDL_Event& e);
    void update(float dt);
    void render(SDL_Renderer* renderer);
    bool wantsQuit() const { return m_quit; }

private:
    void scanMaps();
    void loadMap(const std::string& path);
    void loadFallback();
    void resetAllCars();
    void spawnBots();
    void updateTrails(float dt);

    void renderMenu(SDL_Renderer* renderer);
    void renderTiles(SDL_Renderer* r);
    void renderTrails(SDL_Renderer* r);
    void renderCars(SDL_Renderer* r);
    void renderCar(SDL_Renderer* r, int idx);
    void renderHUD(SDL_Renderer* renderer);
    void renderFinishScreen(SDL_Renderer* renderer);
    void generateFallbackMap();

    AppState m_state = AppState::Menu;
    bool m_quit = false;
    MapData m_map;
    CarConfig m_carCfg;
    RaceData m_race;
    Camera m_camera;
    InputSystem m_input;
    SDL_Renderer* m_renderer = nullptr;

    std::vector<CarState> m_cars;
    std::vector<BotConfig> m_botConfigs;
    std::vector<CarVisual> m_carVisuals;
    int m_playerIndex = 0;

    std::vector<MapEntry> m_maps;
    int m_selectedMap = 0;
    std::string m_mapDir;

    float m_checkpointFlash = 0.0f;
    int m_lastPlayerCheckpoint = 0;
};

} // namespace mm
