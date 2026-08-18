#pragma once

#include "core/map_data.h"
#include "core/tileset.h"
#include "camera.h"
#include "tools.h"
#include "palette.h"
#include "properties.h"
#include "imgui.h"
#include <SDL.h>
#include <string>

namespace mm {

class EditorApp {
public:
    bool init(SDL_Renderer* renderer);
    void shutdown();

    void handleEvent(const SDL_Event& e);
    void update();
    void render(SDL_Renderer* renderer);

    bool wantsQuit() const { return m_quit; }

private:
    void recreateMapTexture(int w, int h);
    void renderGridToTexture();
    void drawGrid(SDL_Texture* target);
    void drawTile(SDL_Renderer* r, int x, int y, TileType type);
    void drawSpawns(SDL_Renderer* r);
    void drawCheckpoints(SDL_Renderer* r);
    void drawSpawnAim(SDL_Renderer* r);
    void drawCheckpointAim(SDL_Renderer* r);
    void beginMapWindow();
    void endMapWindow();
    void drawMenuBar();
    void drawCheckpointIndices(ImVec2 canvasPos);
    void drawMinimap(ImVec2 canvasPos, ImVec2 canvasSize);
    void doSave();
    void doLoad();
    void doNewMap(int w, int h, int tileSize);
    void showStatus(const char* msg);

    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_mapTexture = nullptr;
    int m_texW = 0;
    int m_texH = 0;

    MapData m_map;
    Tileset m_tileset;
    Camera m_camera;
    ToolState m_toolState;
    Tools m_tools;
    Palette m_palette;
    Properties m_properties;

    bool m_quit = false;
    bool m_showPalette = true;
    bool m_showProperties = true;
    bool m_gridEnabled = true;
    bool m_mapFocused = false;
    int m_hoverTileX = -1;
    int m_hoverTileY = -1;
    std::string m_currentFile;
    std::string m_statusMsg;
    float m_statusTimer = 0.0f;

    int m_newMapW = 32;
    int m_newMapH = 32;
    int m_newMapTileSize = 32;
};

} // namespace mm
