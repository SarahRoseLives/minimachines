#pragma once

#include "core/game_input.h"
#include <SDL.h>
#include <vector>

namespace mm {

class InputSystem {
public:
    void init();
    void shutdown();
    void handleEvent(const SDL_Event& e);
    PlayerInput poll();

private:
    float mapAxis(int value);
    float mapTrigger(int value);

    std::vector<SDL_GameController*> m_controllers;
    bool m_keyUp = false;
    bool m_keyDown = false;
    bool m_keyLeft = false;
    bool m_keyRight = false;
    bool m_keyHandbrake = false;
};

} // namespace mm
