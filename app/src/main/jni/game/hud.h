// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

namespace eng { class App; }

namespace cs::game {

class Match;

class Hud {
public:
    void render(eng::App& app, const Match& m);
};

} // namespace cs::game
