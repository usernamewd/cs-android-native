// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include "engine/math.h"

namespace cs::game {

struct Player;

class Bomb {
public:
    static constexpr float kFuseSeconds   = 40.f;
    static constexpr float kPlantSeconds  =  3.2f;
    static constexpr float kDefuseSeconds = 10.f;

    bool      planted{false};
    eng::Vec3 pos{};
    float     fuse{kFuseSeconds};
    float     defuse_progress{0.f};
    bool      exploded{false};
    bool      defused{false};

    void plant(const eng::Vec3& at) {
        planted = true; pos = at; fuse = kFuseSeconds; defuse_progress = 0.f;
        exploded = defused = false;
    }
    void reset() { planted = exploded = defused = false; fuse = kFuseSeconds; defuse_progress = 0.f; }

    void tick(float dt, bool being_defused) {
        if (!planted || exploded || defused) return;
        if (being_defused) {
            defuse_progress += dt;
            if (defuse_progress >= kDefuseSeconds) defused = true;
        } else {
            // Defuse interrupted -> progress decays slightly so you can't
            // simply tap once and walk away.
            defuse_progress = std::max(0.f, defuse_progress - dt * 0.5f);
        }
        fuse -= dt;
        if (fuse <= 0.f) exploded = true;
    }
};

} // namespace cs::game
