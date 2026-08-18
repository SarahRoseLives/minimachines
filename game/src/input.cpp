#include "input.h"

namespace mm {

void InputSystem::init() {
    SDL_GameControllerEventState(SDL_ENABLE);
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* gc = SDL_GameControllerOpen(i);
            if (gc) m_controllers.push_back(gc);
        }
    }
}

void InputSystem::shutdown() {
    for (auto* gc : m_controllers)
        SDL_GameControllerClose(gc);
    m_controllers.clear();
}

void InputSystem::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_CONTROLLERDEVICEADDED) {
        SDL_GameController* gc = SDL_GameControllerOpen(e.cdevice.which);
        if (gc) m_controllers.push_back(gc);
    } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
        for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
            if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(*it)) == e.cdevice.which) {
                SDL_GameControllerClose(*it);
                m_controllers.erase(it);
                break;
            }
        }
    }

    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        bool down = (e.type == SDL_KEYDOWN);
        switch (e.key.keysym.scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:    m_keyUp = down; break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:  m_keyDown = down; break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:  m_keyLeft = down; break;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT: m_keyRight = down; break;
        case SDL_SCANCODE_SPACE: m_keyHandbrake = down; break;
        default: break;
        }
    }
}

float InputSystem::mapAxis(int value) {
    const int DEADZONE = 8000;
    if (std::abs(value) < DEADZONE) return 0.0f;
    return value / 32767.0f;
}

float InputSystem::mapTrigger(int value) {
    return value / 32767.0f;
}

PlayerInput InputSystem::poll() {
    PlayerInput in;

    in.steer = 0.0f;
    in.throttle = 0.0f;
    in.handbrake = m_keyHandbrake;

    if (m_keyRight) in.steer += 1.0f;
    if (m_keyLeft)  in.steer -= 1.0f;
    if (m_keyUp)    in.throttle += 1.0f;
    if (m_keyDown)  in.throttle -= 1.0f;

    for (auto* gc : m_controllers) {
        float stickX = mapAxis(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX));
        float rTrigger = mapTrigger(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
        float lTrigger = mapTrigger(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
        bool cross = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X);

        if (std::abs(stickX) > std::abs(in.steer))
            in.steer = stickX;

        float padThrottle = rTrigger - lTrigger;
        if (std::abs(padThrottle) > std::abs(in.throttle))
            in.throttle = padThrottle;

        if (cross) in.handbrake = true;
    }

    return in;
}

} // namespace mm
