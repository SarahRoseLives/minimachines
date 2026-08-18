#pragma once

#include "ui_context.h"
#include <string>
#include <vector>
#include <functional>

namespace mm {

enum class UIAction { None, Click, Confirm, Back, Up, Down };

struct UIInput {
    UIAction action = UIAction::None;
    int mouseX = 0, mouseY = 0;
    bool mouseDown = false;
    bool mouseClicked = false;
    char textChar = 0;
};

struct UIRect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

struct UIColor {
    Uint8 r, g, b, a;
    operator SDL_Color() const { return {r, g, b, a}; }
};

namespace UICol {
    constexpr UIColor bg       = {30, 30, 40, 240};
    constexpr UIColor panel    = {45, 45, 60, 240};
    constexpr UIColor btn      = {60, 60, 80, 255};
    constexpr UIColor btnHover = {80, 80, 110, 255};
    constexpr UIColor btnFocus = {70, 100, 180, 255};
    constexpr UIColor text     = {230, 230, 230, 255};
    constexpr UIColor textDim  = {150, 150, 160, 255};
    constexpr UIColor title    = {100, 180, 255, 255};
    constexpr UIColor accent   = {100, 180, 255, 255};
    constexpr UIColor inputBg  = {25, 25, 35, 255};
    constexpr UIColor listBg   = {20, 20, 30, 255};
    constexpr UIColor listSel  = {60, 80, 140, 255};
}

bool uiButton(UIContext& ui, const char* label, int x, int y, int w, int h,
              bool focused, const UIInput& input);

void uiLabel(UIContext& ui, const char* text, int x, int y, UIColor color = UICol::text);

void uiLabelCentered(UIContext& ui, const char* text, int cx, int y, UIColor color = UICol::text);

bool uiListBox(UIContext& ui, const std::vector<std::string>& items, int& selected,
               int x, int y, int w, int h, bool focused, const UIInput& input);

void uiTextInput(UIContext& ui, char* buf, int bufSize, int x, int y, int w, int h,
                 bool focused, const UIInput& input);

void uiPanel(UIContext& ui, int x, int y, int w, int h, UIColor color = UICol::panel);

} // namespace mm
