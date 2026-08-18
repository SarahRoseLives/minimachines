#pragma once

#include "ui_context.h"
#include "ui_widget.h"
#include <SDL.h>

namespace mm {

class UIScreen {
public:
    virtual ~UIScreen() = default;
    virtual void handleEvent(const SDL_Event& e) = 0;
    virtual void update(float dt) = 0;
    virtual void render(UIContext& ui) = 0;
};

} // namespace mm
