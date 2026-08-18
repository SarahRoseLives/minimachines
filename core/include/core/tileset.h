#pragma once

#include "core/tiles.h"
#include <SDL.h>
#include <string>
#include <vector>

namespace mm {

struct TileVisual {
    TileType type;
    SDL_Rect srcRect;
};

class Tileset {
public:
    bool load(SDL_Renderer* renderer, const std::string& path);
    void buildDefault();

    SDL_Texture* texture() const { return m_texture; }
    const std::vector<TileVisual>& visuals() const { return m_visuals; }

    const TileVisual* find(TileType type) const;

    int tileWidth() const { return m_tileW; }
    int tileHeight() const { return m_tileH; }

private:
    SDL_Texture* m_texture = nullptr;
    int m_tileW = 32;
    int m_tileH = 32;
    std::vector<TileVisual> m_visuals;
};

} // namespace mm
