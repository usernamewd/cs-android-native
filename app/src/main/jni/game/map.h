// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <vector>

#include "engine/math.h"

namespace cs::game {

struct Wall {
    eng::Aabb box;
};

struct Spawn {
    eng::Vec3 pos;
    float yaw;
};

// "Strike-Lite" map: simple two-bombsite layout with a long corridor and a
// short rotation. Boxy on purpose so we don't need a model loader yet.
class Map {
public:
    void load_default();

    const std::vector<Wall>&  walls() const { return walls_; }
    const std::vector<Spawn>& ct_spawns() const { return ct_spawns_; }
    const std::vector<Spawn>& t_spawns()  const { return t_spawns_; }
    const eng::Aabb& site_a() const { return site_a_; }
    const eng::Aabb& site_b() const { return site_b_; }
    const eng::Aabb& bounds() const { return bounds_; }
    const std::vector<eng::Vec3>& nav_nodes() const { return nav_; }

    // Returns nearest hit distance along ray, or t_max if no hit.
    bool raycast(const eng::Vec3& ro, const eng::Vec3& rd, float t_max, float& out_t) const;

    // True if the AABB intersects any wall.
    bool blocked(const eng::Aabb& q) const;

private:
    std::vector<Wall>  walls_;
    std::vector<Spawn> ct_spawns_;
    std::vector<Spawn> t_spawns_;
    std::vector<eng::Vec3> nav_;
    eng::Aabb site_a_{}, site_b_{}, bounds_{};
};

} // namespace cs::game
