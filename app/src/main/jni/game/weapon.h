// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <algorithm>

namespace cs::game {

struct Weapon {
    enum class Kind { Pistol, Rifle, Knife };

    Kind  kind{Kind::Pistol};
    int   ammo{12};
    int   mag_max{12};
    int   reserve{60};
    float cooldown{0.f};
    float fire_interval{0.18f};
    int   damage{34};
    float range{120.f};
    float spread_rad{0.012f};

    static Weapon pistol() { return {Kind::Pistol, 12, 12, 60, 0, 0.18f, 34, 120.f, 0.012f}; }
    static Weapon rifle()  { return {Kind::Rifle,  30, 30, 90, 0, 0.10f, 36, 200.f, 0.018f}; }

    void tick(float dt) { if (cooldown > 0) cooldown -= dt; }
    bool can_fire() const { return cooldown <= 0.f && ammo > 0; }
    void fire() { if (ammo > 0) { --ammo; cooldown = fire_interval; } }
    void reload() {
        int need = mag_max - ammo;
        int take = std::min(need, reserve);
        ammo    += take;
        reserve -= take;
        cooldown = 1.0f;
    }
};

} // namespace cs::game
