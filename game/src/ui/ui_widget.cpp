#include "ui_widget.h"

namespace mm {

bool uiButton(UIContext& ui, const char* label, int x, int y, int w, int h,
              bool focused, const UIInput& input) {
    UIRect r = {x, y, w, h};
    bool hover = r.contains(input.mouseX, input.mouseY);
    bool clicked = hover && input.mouseClicked;

    UIColor col = focused ? UICol::btnFocus : (hover ? UICol::btnHover : UICol::btn);
    SDL_SetRenderDrawColor(ui.renderer, col.r, col.g, col.b, col.a);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderFillRect(ui.renderer, &dst);

    if (focused) {
        SDL_SetRenderDrawColor(ui.renderer, UICol::accent.r, UICol::accent.g, UICol::accent.b, 255);
        SDL_RenderDrawRect(ui.renderer, &dst);
    }

    SDL_Point ts = ui.textSize(ui.font, label);
    ui.drawText(ui.font, label, x + (w - ts.x) / 2, y + (h - ts.y) / 2, UICol::text);

    return clicked || (focused && input.action == UIAction::Confirm);
}

void uiLabel(UIContext& ui, const char* text, int x, int y, UIColor color) {
    ui.drawText(ui.font, text, x, y, (SDL_Color)color);
}

void uiLabelCentered(UIContext& ui, const char* text, int cx, int y, UIColor color) {
    ui.drawTextCentered(ui.font, text, cx, y, (SDL_Color)color);
}

bool uiListBox(UIContext& ui, const std::vector<std::string>& items, int& selected,
               int x, int y, int w, int h, bool focused, const UIInput& input) {
    SDL_SetRenderDrawColor(ui.renderer, UICol::listBg.r, UICol::listBg.g, UICol::listBg.b, UICol::listBg.a);
    SDL_Rect bg = {x, y, w, h};
    SDL_RenderFillRect(ui.renderer, &bg);

    if (focused) {
        SDL_SetRenderDrawColor(ui.renderer, UICol::accent.r, UICol::accent.g, UICol::accent.b, 200);
        SDL_RenderDrawRect(ui.renderer, &bg);
    }

    int rowH = 28;
    int visibleRows = h / rowH;
    int scrollOffset = 0;
    if (selected >= scrollOffset + visibleRows)
        scrollOffset = selected - visibleRows + 1;
    if (selected < scrollOffset)
        scrollOffset = selected;

    bool changed = false;
    for (int i = 0; i < static_cast<int>(items.size()) && i < visibleRows; ++i) {
        int idx = i + scrollOffset;
        if (idx >= static_cast<int>(items.size())) break;

        int ry = y + i * rowH;
        bool isSelected = (idx == selected);
        bool isHover = false;

        if (input.mouseX >= x && input.mouseX < x + w &&
            input.mouseY >= ry && input.mouseY < ry + rowH) {
            isHover = true;
        }

        if (isSelected) {
            SDL_SetRenderDrawColor(ui.renderer, UICol::listSel.r, UICol::listSel.g, UICol::listSel.b, UICol::listSel.a);
            SDL_Rect selRect = {x + 2, ry, w - 4, rowH};
            SDL_RenderFillRect(ui.renderer, &selRect);
        }

        if (isHover && input.mouseClicked) {
            selected = idx;
            changed = true;
        }

        SDL_Color textColor = isSelected ? SDL_Color{255, 255, 255, 255} : UICol::text;
        ui.drawText(ui.font, items[idx].c_str(), x + 8, ry + 4, textColor);
    }

    if (focused) {
        if (input.action == UIAction::Down && selected < static_cast<int>(items.size()) - 1) {
            selected++;
            changed = true;
        }
        if (input.action == UIAction::Up && selected > 0) {
            selected--;
            changed = true;
        }
    }

    return changed;
}

void uiTextInput(UIContext& ui, char* buf, int bufSize, int x, int y, int w, int h,
                 bool focused, const UIInput& input) {
    SDL_SetRenderDrawColor(ui.renderer, UICol::inputBg.r, UICol::inputBg.g, UICol::inputBg.b, UICol::inputBg.a);
    SDL_Rect bg = {x, y, w, h};
    SDL_RenderFillRect(ui.renderer, &bg);

    if (focused) {
        SDL_SetRenderDrawColor(ui.renderer, UICol::accent.r, UICol::accent.g, UICol::accent.b, 255);
        SDL_RenderDrawRect(ui.renderer, &bg);
    } else {
        SDL_SetRenderDrawColor(ui.renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(ui.renderer, &bg);
    }

    ui.drawText(ui.font, buf, x + 6, y + (h - 18) / 2, UICol::text);

    if (focused && input.textChar) {
        int len = static_cast<int>(strlen(buf));
        if (input.textChar == '\b') {
            if (len > 0) buf[len - 1] = '\0';
        } else if (len < bufSize - 1 && input.textChar >= 32) {
            buf[len] = input.textChar;
            buf[len + 1] = '\0';
        }
    }
}

void uiPanel(UIContext& ui, int x, int y, int w, int h, UIColor color) {
    SDL_SetRenderDrawColor(ui.renderer, color.r, color.g, color.b, color.a);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderFillRect(ui.renderer, &dst);
}

} // namespace mm
