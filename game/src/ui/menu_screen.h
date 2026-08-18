#pragma once

#include "ui_screen.h"
#include <string>
#include <vector>
#include <functional>

namespace mm {

struct MapEntry;

class MenuScreen : public UIScreen {
public:
    using MapLoader = std::function<void(const std::string&)>;
    using ActionCallback = std::function<void()>;

    void setMaps(const std::vector<MapEntry>& maps);
    void setCallbacks(MapLoader loadMap, ActionCallback fallback, ActionCallback random, ActionCallback multiplayer, ActionCallback quit);

    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(UIContext& ui) override;

private:
    std::vector<std::string> m_mapNames;
    std::vector<std::string> m_mapPaths;
    int m_selectedMap = 0;
    int m_focusIndex = 0;

    MapLoader m_loadMap;
    ActionCallback m_fallback;
    ActionCallback m_random;
    ActionCallback m_multiplayer;
    ActionCallback m_quit;

    UIInput m_input;
};

} // namespace mm
