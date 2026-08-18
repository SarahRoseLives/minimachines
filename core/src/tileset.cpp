#include "core/tileset.h"
#include <SDL_image.h>

namespace mm {

bool Tileset::load(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) return false;

    m_texture = SDL_CreateTextureFromSurface(renderer, surface);
    m_tileW = surface->w;
    m_tileH = surface->h;
    SDL_FreeSurface(surface);

    buildDefault();
    return m_texture != nullptr;
}

void Tileset::buildDefault() {
    m_visuals.clear();
    for (auto& info : allTileInfos()) {
        if (info.type == TileType::Empty) continue;
        TileVisual v;
        v.type = info.type;
        v.srcRect = {0, 0, m_tileW, m_tileH};
        m_visuals.push_back(v);
    }
}

const TileVisual* Tileset::find(TileType type) const {
    for (auto& v : m_visuals) {
        if (v.type == type) return &v;
    }
    return nullptr;
}

} // namespace mm
