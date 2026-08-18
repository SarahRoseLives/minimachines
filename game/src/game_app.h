#pragma once

#include "core/car.h"
#include "core/bot.h"
#include "core/map_data.h"
#include "core/race.h"
#include "camera.h"
#include "input.h"
#include "network_client.h"
#include "ui/ui_context.h"
#include "ui/menu_screen.h"
#include "ui/server_screen.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <memory>

namespace mm {

struct MapEntry {
    std::string name;
    std::string path;
};

enum class AppState {
    Menu,
    Playing,
    MultiplayerLobby,
    MultiplayerPlaying,
};

struct TrailPoint {
    float x, y;
    float age;
};

struct CarVisual {
    Uint8 r, g, b;
    std::vector<TrailPoint> trails;
    float hopScale = 1.0f;
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
    void loadRandomMap();
    void resetAllCars();
    void spawnBots();
    void updateTrails(float dt);
    void enterMenu();

    void renderTiles(SDL_Renderer* r);
    void renderTrails(SDL_Renderer* r);
    void renderCars(SDL_Renderer* r);
    void renderCar(SDL_Renderer* r, int idx);
    void renderHUD(SDL_Renderer* renderer);
    void renderMinimap(SDL_Renderer* renderer);
    void renderFinishScreen(SDL_Renderer* renderer);
    void generateFallbackMap();

    void connectToServer(const std::string& ip, int port);
    void onMapReceived(const std::string& mapData);
    void onStateReceived();

    AppState m_state = AppState::Menu;
    bool m_quit = false;
    MapData m_map;
    CarConfig m_carCfg;
    RaceData m_race;
    Camera m_camera;
    InputSystem m_input;
    SDL_Renderer* m_renderer = nullptr;

    UIContext m_ui;
    std::unique_ptr<MenuScreen> m_menuScreen;
    std::unique_ptr<ServerScreen> m_serverScreen;

    std::vector<CarState> m_cars;
    std::vector<BotConfig> m_botConfigs;
    std::vector<BotState> m_botStates;
    std::vector<CarVisual> m_carVisuals;
    int m_playerIndex = 0;

    std::vector<MapEntry> m_maps;
    std::string m_mapDir;

    float m_checkpointFlash = 0.0f;
    int m_lastPlayerCheckpoint = 0;

    NetworkClient m_net;
    std::string m_masterUrl;
    bool m_netInitialized = false;
    bool m_mpMapLoaded = false;
    uint16_t m_mpConnectedMask = 0;
};

} // namespace mm
