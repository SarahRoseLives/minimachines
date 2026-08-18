#include "camera.h"
#include <cmath>

namespace mm {

void Camera::update(float targetX, float targetY, float dt) {
    float t = 1.0f - std::exp(-smoothing * dt);
    x += (targetX - x) * t;
    y += (targetY - y) * t;
}

SDL_Point Camera::worldToScreen(float wx, float wy) const {
    int sx = static_cast<int>((wx - x) * zoom);
    int sy = static_cast<int>((wy - y) * zoom);
    return {sx, sy};
}

SDL_Point Camera::screenToWorld(int sx, int sy) const {
    float wx = sx / zoom + x;
    float wy = sy / zoom + y;
    return {static_cast<int>(std::floor(wx)), static_cast<int>(std::floor(wy))};
}

} // namespace mm
