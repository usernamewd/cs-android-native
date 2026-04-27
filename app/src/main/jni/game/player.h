// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include "engine/math.h"
#include "game/weapon.h"

namespace cs::game {

enum class Team { CT, T };

struct Player {
    Team team{Team::CT};
    eng::Vec3 pos{};
    eng::Vec3 vel{};
    float yaw{0}, pitch{0};
    int   health{100};
    int   armor{0};
    bool  alive{true};
    bool  is_local{false};
    bool  has_bomb{false};
    bool  defusing{false};
    float plant_progress{0.f};   // T side
    float defuse_progress{0.f};  // CT side
    Weapon weapon{Weapon::pistol()};
    eng::Vec3 size{0.6f, 1.8f, 0.6f};

    eng::Vec3 forward() const {
        float cp = std::cos(pitch), sp = std::sin(pitch);
        float cy = std::cos(yaw),   sy = std::sin(yaw);
        return { -sy * cp, sp, -cy * cp };
    }

    eng::Aabb aabb_at(const eng::Vec3& p) const {
        return { { p.x - size.x * 0.5f, p.y,            p.z - size.z * 0.5f },
                 { p.x + size.x * 0.5f, p.y + size.y,   p.z + size.z * 0.5f } };
    }
};

} // namespace cs::game
