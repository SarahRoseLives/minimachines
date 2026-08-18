#pragma once

#include <SDL.h>

namespace mm {

struct Camera {
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 2.0f;
    float smoothing = 4.0f;

    void update(float targetX, float targetY, float dt);
    SDL_Point worldToScreen(float wx, float wy) const;
    SDL_Point screenToWorld(int sx, int sy) const;
};

} // namespace mm
