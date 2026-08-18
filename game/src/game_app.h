#pragma once

#include "core/car.h"
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
    void resetCar();

    void renderMenu(SDL_Renderer* renderer);
    void renderTiles(SDL_Renderer* r);
    void renderCar(SDL_Renderer* r);
    void renderHUD(SDL_Renderer* renderer);
    void generateFallbackMap();

    AppState m_state = AppState::Menu;
    bool m_quit = false;
    MapData m_map;
    CarState m_car;
    CarConfig m_carCfg;
    RaceData m_race;
    Camera m_camera;
    InputSystem m_input;
    SDL_Renderer* m_renderer = nullptr;
    float m_prevCarX = 0.0f;
    float m_prevCarY = 0.0f;

    std::vector<MapEntry> m_maps;
    int m_selectedMap = 0;
    std::string m_mapDir;
};

} // namespace mm
