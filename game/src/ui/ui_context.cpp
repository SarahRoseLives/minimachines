#include "ui_context.h"
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace mm {

static std::string getExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string path(buf);
    auto pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : ".";
#else
    return ".";
#endif
}

bool UIContext::init(SDL_Renderer* r) {
    renderer = r;
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return false;
    }

    std::string exeDir = getExeDir();
    std::vector<fs::path> searchPaths = {
        fs::path(exeDir) / "assets" / "fonts" / "DejaVuSans.ttf",
        fs::path(exeDir) / ".." / "assets" / "fonts" / "DejaVuSans.ttf",
        fs::path(exeDir) / ".." / ".." / "assets" / "fonts" / "DejaVuSans.ttf",
        fs::path(ASSET_PATH) / "fonts" / "DejaVuSans.ttf",
    };

    std::string fontPath;
    for (auto& p : searchPaths) {
        if (fs::exists(p)) {
            fontPath = p.string();
            break;
        }
    }

    if (fontPath.empty()) {
        fprintf(stderr, "Font not found in any search path\n");
        return false;
    }

    font = TTF_OpenFont(fontPath.c_str(), 18);
    fontLarge = TTF_OpenFont(fontPath.c_str(), 28);
    fontTitle = TTF_OpenFont(fontPath.c_str(), 48);

    if (!font || !fontLarge || !fontTitle) {
        fprintf(stderr, "TTF_OpenFont failed for %s: %s\n", fontPath.c_str(), TTF_GetError());
        return false;
    }

    return true;
}

void UIContext::shutdown() {
    if (font) TTF_CloseFont(font);
    if (fontLarge) TTF_CloseFont(fontLarge);
    if (fontTitle) TTF_CloseFont(fontTitle);
    TTF_Quit();
}

void UIContext::beginFrame() {
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);
}

void UIContext::drawText(TTF_Font* f, const char* text, int x, int y, SDL_Color color) {
    if (!text || !text[0]) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, text, color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void UIContext::drawTextCentered(TTF_Font* f, const char* text, int cx, int y, SDL_Color color) {
    SDL_Point sz = textSize(f, text);
    drawText(f, text, cx - sz.x / 2, y, color);
}

SDL_Point UIContext::textSize(TTF_Font* f, const char* text) {
    SDL_Point sz = {0, 0};
    if (!text || !text[0]) return sz;
    TTF_SizeUTF8(f, text, &sz.x, &sz.y);
    return sz;
}

void UIContext::drawText(TTF_Font* f, const char* text, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    drawText(f, text, x, y, {r, g, b, a});
}

void UIContext::drawTextCentered(TTF_Font* f, const char* text, int cx, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    drawTextCentered(f, text, cx, y, {r, g, b, a});
}

} // namespace mm
