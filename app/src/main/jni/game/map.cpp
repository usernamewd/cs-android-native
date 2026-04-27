// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "game/map.h"

namespace cs::game {

namespace {

eng::Aabb box_centered(eng::Vec3 c, eng::Vec3 size) {
    return { { c.x - size.x * 0.5f, c.y - size.y * 0.5f, c.z - size.z * 0.5f },
             { c.x + size.x * 0.5f, c.y + size.y * 0.5f, c.z + size.z * 0.5f } };
}

} // namespace

void Map::load_default() {
    walls_.clear();
    ct_spawns_.clear();
    t_spawns_.clear();
    nav_.clear();

    // Outer bounds: 60 x 6 x 60 arena, floor at y=0.
    bounds_ = { {-30, 0, -30}, {30, 6, 30} };

    // Floor (thin AABB so raycast works even though we render it specially).
    walls_.push_back({ box_centered({0, -0.05f, 0}, {60, 0.1f, 60}) });

    // Outer perimeter walls.
    walls_.push_back({ box_centered({  0, 3,  30}, {60, 6, 1}) });
    walls_.push_back({ box_centered({  0, 3, -30}, {60, 6, 1}) });
    walls_.push_back({ box_centered({ 30, 3,   0}, {1,  6, 60}) });
    walls_.push_back({ box_centered({-30, 3,   0}, {1,  6, 60}) });

    // Mid divider (open in middle for 'mid' rotation).
    walls_.push_back({ box_centered({-3, 2.5f,  0}, {1, 5, 18}) });
    walls_.push_back({ box_centered({ 3, 2.5f,  0}, {1, 5, 18}) });

    // Cover crates near each bombsite.
    walls_.push_back({ box_centered({-18, 1.0f,  18}, {3, 2, 3}) });
    walls_.push_back({ box_centered({ 18, 1.0f,  18}, {3, 2, 3}) });
    walls_.push_back({ box_centered({-18, 1.0f, -18}, {3, 2, 3}) });
    walls_.push_back({ box_centered({ 18, 1.0f, -18}, {3, 2, 3}) });

    // Bombsites.
    site_a_ = box_centered({-22, 2,  22}, {10, 4, 10});
    site_b_ = box_centered({ 22, 2,  22}, {10, 4, 10});

    // Spawn points.
    for (int i = 0; i < 5; ++i) {
        float dz = -25 + i * 0.8f;
        ct_spawns_.push_back({ { -25, 1.0f, dz }, eng::deg2rad(90.f) });
        t_spawns_.push_back ({ {  25, 1.0f, dz }, eng::deg2rad(-90.f) });
    }

    // Navigation waypoints (used by the AI as a coarse roadmap).
    nav_ = {
        {-25,  1, -22}, {-22,  1, -10}, {-15,  1,   0}, {-15,  1,  12}, {-22,  1,  18},
        { 25,  1, -22}, { 22,  1, -10}, { 15,  1,   0}, { 15,  1,  12}, { 22,  1,  18},
        {  0,  1,   8}, {  0,  1, -8}
    };
}

bool Map::raycast(const eng::Vec3& ro, const eng::Vec3& rd, float t_max, float& out_t) const {
    float best = t_max;
    bool any = false;
    for (const auto& w : walls_) {
        float t;
        if (eng::ray_aabb(ro, rd, w.box, best, t) && t < best && t > 0) {
            best = t; any = true;
        }
    }
    out_t = best;
    return any;
}

bool Map::blocked(const eng::Aabb& q) const {
    for (const auto& w : walls_) {
        if (q.min.x <= w.box.max.x && q.max.x >= w.box.min.x &&
            q.min.y <= w.box.max.y && q.max.y >= w.box.min.y &&
            q.min.z <= w.box.max.z && q.max.z >= w.box.min.z) {
            return true;
        }
    }
    return false;
}

} // namespace cs::game
