#include "menu_screen.h"
#include "game_app.h"

namespace mm {

void MenuScreen::setMaps(const std::vector<MapEntry>& maps) {
    m_mapNames.clear();
    m_mapPaths.clear();
    for (auto& m : maps) {
        m_mapNames.push_back(m.name);
        m_mapPaths.push_back(m.path);
    }
    m_selectedMap = 0;
}

void MenuScreen::setCallbacks(MapLoader loadMap, ActionCallback fallback,
                               ActionCallback random, ActionCallback multiplayer,
                               ActionCallback quit) {
    m_loadMap = loadMap;
    m_fallback = fallback;
    m_random = random;
    m_multiplayer = multiplayer;
    m_quit = quit;
}

void MenuScreen::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.scancode) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            m_input.action = UIAction::Up; break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            m_input.action = UIAction::Down; break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_SPACE:
            m_input.action = UIAction::Confirm; break;
        case SDL_SCANCODE_ESCAPE:
            m_input.action = UIAction::Back; break;
        default: break;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        m_input.mouseClicked = true;

    if (e.type == SDL_MOUSEMOTION) {
        m_input.mouseX = e.motion.x;
        m_input.mouseY = e.motion.y;
    }
}

void MenuScreen::update(float dt) {
    (void)dt;
}

void MenuScreen::render(UIContext& ui) {
    SDL_GetMouseState(&m_input.mouseX, &m_input.mouseY);

    int sw = ui.screenW;
    int sh = ui.screenH;
    int menuW = 500;
    int menuX = (sw - menuW) / 2;
    int menuY = sh / 2 - 250;
    int cy = menuY;

    uiPanel(ui, menuX - 20, menuY - 20, menuW + 40, 540, UICol::bg);

    ui.drawTextCentered(ui.fontTitle, "MINIMACHINES", sw / 2, cy, UICol::title);
    cy += 60;

    ui.drawTextCentered(ui.font, "Select a map:", sw / 2, cy, UICol::textDim);
    cy += 30;

    int listH = 180;
    uiListBox(ui, m_mapNames, m_selectedMap, menuX, cy, menuW, listH, m_focusIndex == 0, m_input);
    cy += listH + 15;

    if (!m_mapPaths.empty()) {
        if (uiButton(ui, "Play Selected", menuX, cy, menuW, 45, m_focusIndex == 1, m_input)) {
            if (m_loadMap) m_loadMap(m_mapPaths[m_selectedMap]);
        }
    } else {
        m_focusIndex = 1;
    }
    cy += 52;

    if (uiButton(ui, "Fallback Track", menuX, cy, menuW, 45, m_focusIndex == 2, m_input)) {
        if (m_fallback) m_fallback();
    }
    cy += 52;

    if (uiButton(ui, "Random Track", menuX, cy, menuW, 45, m_focusIndex == 3, m_input)) {
        if (m_random) m_random();
    }
    cy += 52;

    if (uiButton(ui, "Multiplayer", menuX, cy, menuW, 45, m_focusIndex == 4, m_input)) {
        if (m_multiplayer) m_multiplayer();
    }
    cy += 52;

    if (uiButton(ui, "Quit", menuX, cy, menuW, 35, m_focusIndex == 5, m_input)) {
        if (m_quit) m_quit();
    }
    cy += 45;

    ui.drawTextCentered(ui.font, "D-Pad/WASD: Navigate  |  A/Enter: Select", sw / 2, cy, UICol::textDim);

    if (m_input.action == UIAction::Down) m_focusIndex = (m_focusIndex + 1) % 6;
    if (m_input.action == UIAction::Up) m_focusIndex = (m_focusIndex + 5) % 6;

    m_input.mouseClicked = false;
    m_input.action = UIAction::None;
}

} // namespace mm
