#pragma once

#include <SDL.h>

namespace mm {

struct Camera {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float zoom = 1.0f;
    bool panning = false;
    int panStartX = 0;
    int panStartY = 0;
    float panStartOffX = 0.0f;
    float panStartOffY = 0.0f;

    void beginPan(int mx, int my);
    void updatePan(int mx, int my);
    void endPan();
    void handleZoom(int wheelY, int mx, int my);
    SDL_Point screenToWorld(int sx, int sy, int tileSize) const;
    SDL_Point worldToScreen(int wx, int wy, int tileSize) const;
};

} // namespace mm
