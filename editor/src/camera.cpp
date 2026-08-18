#include "camera.h"
#include <cmath>

namespace mm {

void Camera::beginPan(int mx, int my) {
    panning = true;
    panStartX = mx;
    panStartY = my;
    panStartOffX = offsetX;
    panStartOffY = offsetY;
}

void Camera::updatePan(int mx, int my) {
    if (!panning) return;
    offsetX = panStartOffX - (mx - panStartX) / zoom;
    offsetY = panStartOffY - (my - panStartY) / zoom;
}

void Camera::endPan() {
    panning = false;
}

void Camera::handleZoom(int wheelY, int mx, int my) {
    float oldZoom = zoom;
    if (wheelY > 0) zoom *= 1.1f;
    if (wheelY < 0) zoom /= 1.1f;
    if (zoom < 0.1f) zoom = 0.1f;
    if (zoom > 20.0f) zoom = 20.0f;
    offsetX = mx / oldZoom + offsetX - mx / zoom;
    offsetY = my / oldZoom + offsetY - my / zoom;
}

SDL_Point Camera::screenToWorld(int sx, int sy, int tileSize) const {
    float wx = sx / zoom + offsetX;
    float wy = sy / zoom + offsetY;
    return {static_cast<int>(std::floor(wx / tileSize)),
            static_cast<int>(std::floor(wy / tileSize))};
}

SDL_Point Camera::worldToScreen(int wx, int wy, int tileSize) const {
    float px = wx * tileSize;
    float py = wy * tileSize;
    return {static_cast<int>((px - offsetX) * zoom),
            static_cast<int>((py - offsetY) * zoom)};
}

} // namespace mm
