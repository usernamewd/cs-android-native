// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <memory>

#include "game/hud.h"
#include "game/match.h"
#include "game/menu.h"

namespace cs::game {

class Game {
public:
    Game();

    void on_surface_ready();
    void on_resize(int w, int h);
    void update(float dt);
    void render();

private:
    enum class Screen { Menu, InMatch };
    Screen screen_{Screen::Menu};
    Menu menu_;
    Hud  hud_;
    std::unique_ptr<Match> match_;
};

} // namespace cs::game
