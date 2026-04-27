// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "game/bot.h"

#include <algorithm>
#include <cmath>

#include "game/match.h"
#include "game/player.h"

namespace cs::game {

namespace {

uint32_t lcg(uint32_t& s) { s = s * 1664525u + 1013904223u; return s; }
float    frand(uint32_t& s) { return (lcg(s) >> 8) / 16777216.f; }

int nearest_node(const Match& m, const eng::Vec3& p) {
    const auto& nodes = m.map().nav_nodes();
    int best = 0; float bd = 1e9f;
    for (size_t i = 0; i < nodes.size(); ++i) {
        eng::Vec3 d = nodes[i] - p;
        float ds = d.length_sq();
        if (ds < bd) { bd = ds; best = static_cast<int>(i); }
    }
    return best;
}

bool has_los(const Match& m, const eng::Vec3& a, const eng::Vec3& b) {
    eng::Vec3 d = b - a;
    float dist = d.length();
    if (dist < 0.01f) return true;
    eng::Vec3 dir = d * (1.f / dist);
    float t;
    bool hit = m.map().raycast(a, dir, dist, t);
    return !hit || t >= dist - 0.01f;
}

} // namespace

void Bot::update(Match& m, int player_index, BotBrain& brain, float dt) {
    auto& self = m.players()[player_index];
    if (!self.alive) return;

    self.weapon.tick(dt);

    // 1. Pick / refresh target.
    brain.decision_cooldown -= dt;
    if (brain.decision_cooldown <= 0.f) {
        brain.decision_cooldown = 0.4f + frand(brain.rng) * 0.4f;

        // Look for the closest visible enemy.
        int best = -1; float bd = 1e9f;
        for (size_t i = 0; i < m.players().size(); ++i) {
            const auto& p = m.players()[i];
            if (!p.alive || p.team == self.team) continue;
            eng::Vec3 eye_self = self.pos + eng::Vec3{0, 1.6f, 0};
            eng::Vec3 eye_them = p.pos    + eng::Vec3{0, 1.6f, 0};
            float dist = (eye_them - eye_self).length();
            if (dist > 60.f) continue;
            if (!has_los(m, eye_self, eye_them)) continue;
            if (dist < bd) { bd = dist; best = static_cast<int>(i); }
        }
        brain.target_player = best;

        if (best >= 0) {
            brain.goal = BotBrain::Goal::Engage;
        } else if (m.bomb().planted) {
            brain.goal = (self.team == Team::CT) ? BotBrain::Goal::Defuse
                                                  : BotBrain::Goal::HoldSite;
        } else if (self.has_bomb) {
            brain.goal = BotBrain::Goal::Plant;
        } else {
            brain.goal = (self.team == Team::T) ? BotBrain::Goal::Push
                                                 : BotBrain::Goal::HoldSite;
        }
    }

    // 2. Pick a movement target based on goal.
    eng::Vec3 desired_pos = self.pos;
    bool want_move = true;

    switch (brain.goal) {
        case BotBrain::Goal::Engage: {
            if (brain.target_player >= 0) {
                const auto& p = m.players()[brain.target_player];
                eng::Vec3 to = p.pos - self.pos;
                self.yaw   = std::atan2(-to.x, -to.z);
                self.pitch = 0.f;
                // Slight strafe to look less robotic.
                float strafe = (frand(brain.rng) - 0.5f) * 1.5f;
                desired_pos = self.pos + eng::Vec3{ std::cos(self.yaw) * strafe, 0,
                                                    -std::sin(self.yaw) * strafe };
                // Open fire if cooldown elapsed and skill check passes.
                brain.fire_jitter -= dt;
                if (brain.fire_jitter <= 0.f && self.weapon.can_fire()) {
                    if (frand(brain.rng) < brain.skill) m.try_fire(player_index);
                    brain.fire_jitter = 0.05f + frand(brain.rng) * 0.10f;
                }
            }
            break;
        }
        case BotBrain::Goal::Plant: {
            const auto& site = (frand(brain.rng) > 0.5f) ? m.map().site_a() : m.map().site_b();
            desired_pos = (site.min + site.max) * 0.5f;
            if (m.local_player_in_site_with_bomb(player_index)) {
                m.try_plant(player_index);
                want_move = false;
            }
            break;
        }
        case BotBrain::Goal::Defuse: {
            desired_pos = m.bomb().pos;
            if ((self.pos - m.bomb().pos).length() < 1.5f) {
                self.defusing = true;
                want_move = false;
            } else {
                self.defusing = false;
            }
            break;
        }
        case BotBrain::Goal::Push:
        case BotBrain::Goal::HoldSite:
        case BotBrain::Goal::Reposition: {
            if (brain.target_node < 0) {
                brain.target_node = nearest_node(m, self.pos);
            }
            const auto& nodes = m.map().nav_nodes();
            if (!nodes.empty()) {
                int idx = brain.target_node % static_cast<int>(nodes.size());
                desired_pos = nodes[idx];
                if ((self.pos - desired_pos).length() < 1.5f) {
                    brain.target_node = static_cast<int>(lcg(brain.rng) % nodes.size());
                }
            }
            break;
        }
    }

    if (want_move) {
        eng::Vec3 d = desired_pos - self.pos;
        d.y = 0.f;
        float L = d.length();
        if (L > 0.05f) {
            eng::Vec3 step = d * (3.5f * dt / L);
            // Look in the direction of travel unless already aiming at an enemy.
            if (brain.goal != BotBrain::Goal::Engage) {
                self.yaw = std::atan2(-d.x, -d.z);
            }
            m.try_move(player_index, step);
        }
    }
}

} // namespace cs::game
