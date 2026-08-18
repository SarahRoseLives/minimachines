#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

namespace mm {

struct UIContext {
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    TTF_Font* fontLarge = nullptr;
    TTF_Font* fontTitle = nullptr;
    int screenW = 0;
    int screenH = 0;

    bool init(SDL_Renderer* r);
    void shutdown();
    void beginFrame();

    void drawText(TTF_Font* f, const char* text, int x, int y, SDL_Color color);
    void drawTextCentered(TTF_Font* f, const char* text, int cx, int y, SDL_Color color);
    SDL_Point textSize(TTF_Font* f, const char* text);

    void drawText(TTF_Font* f, const char* text, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void drawTextCentered(TTF_Font* f, const char* text, int cx, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
};

} // namespace mm
