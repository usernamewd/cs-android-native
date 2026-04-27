// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "game/match.h"

#include <GLES3/gl3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

#include "engine/app.h"
#include "engine/input.h"
#include "engine/mesh.h"
#include "engine/shader.h"

namespace cs::game {

namespace {

uint32_t now_seed() {
    using namespace std::chrono;
    return static_cast<uint32_t>(steady_clock::now().time_since_epoch().count());
}

void shuffle_spawns(std::vector<Spawn>& s, std::mt19937& rng) {
    for (size_t i = s.size(); i > 1; --i) {
        std::uniform_int_distribution<size_t> d(0, i - 1);
        std::swap(s[i - 1], s[d(rng)]);
    }
}

// Convenience: fill a 4x4 column-major float array from a Mat4.
const float* mptr(const eng::Mat4& m) { return m.m; }

} // namespace

void Match::start(int max_rounds) {
    map_.load_default();
    max_rounds_ = max_rounds;
    round_ = 0;
    ct_score_ = t_score_ = 0;
    half_swapped_ = false;

    players_.clear();
    brains_.clear();

    // Index 0 is the local player (CT by default).
    Player local{};
    local.is_local = true;
    local.team = Team::CT;
    local.weapon = Weapon::pistol();
    players_.push_back(local);
    brains_.push_back({});

    auto spawn_team = [&](Team team, int count) {
        for (int i = 0; i < count; ++i) {
            Player p{};
            p.team = team;
            p.weapon = Weapon::pistol();
            BotBrain b{};
            b.rng = now_seed() + static_cast<uint32_t>(players_.size() * 0x9e3779b9u);
            b.skill = 0.5f + (i * 0.07f);
            players_.push_back(p);
            brains_.push_back(b);
        }
    };
    spawn_team(Team::CT, kBotsPerSide);     // CT teammates
    spawn_team(Team::T,  kBotsPerSide + 1); // T side gets 5 (incl. bomb carrier)

    respawn_all();
    begin_freeze();
}

void Match::respawn_all() {
    std::mt19937 rng(now_seed());
    auto cts = map_.ct_spawns();
    auto ts  = map_.t_spawns();
    shuffle_spawns(cts, rng);
    shuffle_spawns(ts,  rng);

    int ci = 0, ti = 0;
    bool gave_bomb = false;
    for (size_t i = 0; i < players_.size(); ++i) {
        auto& p = players_[i];
        p.health = 100;
        p.armor  = 0;
        p.alive  = true;
        p.has_bomb = false;
        p.defusing = false;
        p.plant_progress = 0.f;
        p.defuse_progress = 0.f;
        p.vel = {};
        p.weapon = Weapon::pistol();
        if (p.team == Team::CT) {
            const auto& s = cts[ci++ % cts.size()];
            p.pos = s.pos; p.yaw = s.yaw; p.pitch = 0.f;
        } else {
            const auto& s = ts[ti++ % ts.size()];
            p.pos = s.pos; p.yaw = s.yaw; p.pitch = 0.f;
            if (!gave_bomb) { p.has_bomb = true; gave_bomb = true; }
        }
    }

    bomb_.reset();
}

void Match::begin_freeze() {
    phase_ = Phase::Freeze;
    phase_timer_ = kFreezeDuration;
    respawn_all();
    ++round_;
}

void Match::begin_live() {
    phase_ = Phase::Live;
    phase_timer_ = kLiveDuration;
}

void Match::begin_post_round(Team winner, const char* reason) {
    phase_ = Phase::PostRound;
    phase_timer_ = kPostDuration;
    last_winner_ = winner;
    last_reason_ = reason;
    if (winner == Team::CT) ++ct_score_; else ++t_score_;
}

void Match::enter_halftime() {
    phase_ = Phase::Halftime;
    phase_timer_ = 8.f;
    half_swapped_ = true;
    for (auto& p : players_) {
        if (p.team == Team::CT) p.team = Team::T;
        else if (p.team == Team::T) p.team = Team::CT;
    }
}

void Match::try_move(int player_index, const eng::Vec3& step) {
    auto& p = players_[player_index];
    if (!p.alive) return;
    eng::Vec3 next = p.pos + step;
    if (!map_.blocked(p.aabb_at(next))) p.pos = next;
    else {
        // Try axis-separated motion so we slide along walls.
        eng::Vec3 nx = p.pos + eng::Vec3{ step.x, 0, 0 };
        if (!map_.blocked(p.aabb_at(nx))) p.pos = nx;
        eng::Vec3 nz = p.pos + eng::Vec3{ 0, 0, step.z };
        if (!map_.blocked(p.aabb_at(nz))) p.pos = nz;
    }
}

void Match::try_fire(int shooter_index) {
    auto& s = players_[shooter_index];
    if (!s.alive || !s.weapon.can_fire()) return;
    if (phase_ == Phase::Freeze || phase_ == Phase::PostRound) return;

    eng::Vec3 eye = s.pos + eng::Vec3{0, 1.6f, 0};
    eng::Vec3 dir = s.forward();
    s.weapon.fire();

    // Walls first.
    float wall_t;
    bool wall_hit = map_.raycast(eye, dir, s.weapon.range, wall_t);

    // Then players.
    int   best_idx = -1;
    float best_t   = wall_hit ? wall_t : s.weapon.range;
    for (size_t i = 0; i < players_.size(); ++i) {
        if (static_cast<int>(i) == shooter_index) continue;
        const auto& q = players_[i];
        if (!q.alive || q.team == s.team) continue;
        float t;
        eng::Aabb body = q.aabb_at(q.pos);
        if (eng::ray_aabb(eye, dir, body, best_t, t) && t < best_t && t > 0) {
            best_t = t; best_idx = static_cast<int>(i);
        }
    }
    if (best_idx >= 0) apply_damage(best_idx, s.weapon.damage, shooter_index);
}

void Match::apply_damage(int target_idx, int dmg, int attacker_idx) {
    auto& p = players_[target_idx];
    if (!p.alive) return;
    int absorbed = std::min(dmg / 2, p.armor);
    p.armor -= absorbed;
    p.health -= (dmg - absorbed);
    if (p.health <= 0) {
        p.alive = false;
        if (p.has_bomb) {
            // Drop the bomb at feet -- next teammate must pick it up. For
            // simplicity we re-assign immediately to the closest living T.
            p.has_bomb = false;
            int best = -1; float bd = 1e9f;
            for (size_t i = 0; i < players_.size(); ++i) {
                const auto& q = players_[i];
                if (!q.alive || q.team != Team::T) continue;
                float d = (q.pos - p.pos).length_sq();
                if (d < bd) { bd = d; best = static_cast<int>(i); }
            }
            if (best >= 0) players_[best].has_bomb = true;
        }
    }
    (void)attacker_idx;
}

bool Match::try_plant(int player_index) {
    auto& p = players_[player_index];
    if (!p.alive || !p.has_bomb) return false;
    if (bomb_.planted) return false;
    bool in_site = point_in(map_.site_a(), p.pos) || point_in(map_.site_b(), p.pos);
    if (!in_site) return false;
    p.plant_progress += 0.05f; // bot calls this frequently while standing in site
    if (p.plant_progress >= Bomb::kPlantSeconds) {
        bomb_.plant(p.pos);
        p.has_bomb = false;
        p.plant_progress = 0.f;
        return true;
    }
    return false;
}

bool Match::local_player_in_site_with_bomb(int player_index) const {
    const auto& p = players_[player_index];
    if (!p.has_bomb) return false;
    return point_in(map_.site_a(), p.pos) || point_in(map_.site_b(), p.pos);
}

void Match::simulate_local(float dt, const eng::TouchInput& input) {
    auto& me = players_[0];
    if (!me.alive) return;

    // Look (right thumb).
    eng::Vec2 ld = input.look_delta();
    me.yaw   -= ld.x * 0.0035f;
    me.pitch -= ld.y * 0.0035f;
    me.pitch = eng::clampf(me.pitch, -1.45f, 1.45f);

    // Move (left thumb).
    eng::Vec2 mv = input.move_axis();
    eng::Vec3 fwd = me.forward(); fwd.y = 0; fwd = fwd.normalized();
    eng::Vec3 rgt { std::cos(me.yaw), 0.f, -std::sin(me.yaw) };
    float speed = (phase_ == Phase::Freeze) ? 1.5f : 4.5f;
    eng::Vec3 wish = (fwd * mv.y + rgt * mv.x) * (speed * dt);
    try_move(0, wish);

    me.weapon.tick(dt);

    // Fire (tap right side).
    if (input.fire_pressed() && phase_ == Phase::Live) {
        try_fire(0);
    }

    // Auto-defuse if local CT and standing on bomb.
    if (me.team == Team::CT && bomb_.planted &&
        (me.pos - bomb_.pos).length() < 1.5f) {
        me.defusing = true;
    } else {
        me.defusing = false;
    }
}

void Match::check_round_end() {
    if (phase_ != Phase::Live) return;

    // T win conditions: bomb explodes OR all CTs dead (with bomb-not-defused).
    // CT win conditions: bomb defused OR all Ts dead before plant
    //                    AND time runs out before plant.
    if (bomb_.exploded) { begin_post_round(Team::T,  "Bomb detonated"); return; }
    if (bomb_.defused)  { begin_post_round(Team::CT, "Bomb defused");   return; }

    bool any_ct = false, any_t = false;
    for (const auto& p : players_) {
        if (!p.alive) continue;
        if (p.team == Team::CT) any_ct = true;
        if (p.team == Team::T)  any_t  = true;
    }
    if (!any_ct) { begin_post_round(Team::T,  "CTs eliminated"); return; }
    if (!bomb_.planted && !any_t) {
        begin_post_round(Team::CT, "Ts eliminated"); return;
    }

    if (phase_timer_ <= 0.f) {
        if (!bomb_.planted) begin_post_round(Team::CT, "Time expired");
        // If bomb is planted, time expiring no longer ends the round; bomb fuse drives it.
    }
}

void Match::update(float dt, const eng::TouchInput& input) {
    phase_timer_ -= dt;

    // Drive the local player every frame so they can look around even in freeze.
    simulate_local(dt, input);

    // Bots drive themselves.
    for (size_t i = 1; i < players_.size(); ++i) {
        if (phase_ == Phase::Freeze || phase_ == Phase::PostRound) {
            // bots can still re-aim, but no firing/plant (try_fire checks phase).
            BotBrain dummy = brains_[i];
            Bot::update(*this, static_cast<int>(i), dummy, dt * 0.25f);
            brains_[i] = dummy;
        } else {
            Bot::update(*this, static_cast<int>(i), brains_[i], dt);
        }
    }

    // Bomb tick + defuse aggregation.
    if (phase_ == Phase::Live && bomb_.planted) {
        bool being_defused = false;
        for (const auto& p : players_) {
            if (p.alive && p.team == Team::CT && p.defusing &&
                (p.pos - bomb_.pos).length() < 1.5f) {
                being_defused = true;
                break;
            }
        }
        bomb_.tick(dt, being_defused);
    }

    check_round_end();

    if (phase_timer_ <= 0.f) {
        switch (phase_) {
            case Phase::Freeze:    begin_live(); break;
            case Phase::PostRound:
                if (round_ >= max_rounds_ || ct_score_ >= max_rounds_/2 + 1
                                          || t_score_  >= max_rounds_/2 + 1) {
                    phase_ = Phase::MatchOver;
                    phase_timer_ = 1e9f;
                } else if (round_ == max_rounds_/2 && !half_swapped_) {
                    enter_halftime();
                } else {
                    begin_freeze();
                }
                break;
            case Phase::Halftime:  begin_freeze(); break;
            case Phase::Live:
                // Live ending is handled by check_round_end.
                if (phase_ == Phase::Live) phase_timer_ = 0.f;
                break;
            case Phase::MatchOver: phase_timer_ = 1e9f; break;
        }
    }
}

void Match::render(eng::App& app) {
    auto& cam = app.camera();
    const auto& me = players_[0];
    cam.position = me.pos + eng::Vec3{0, 1.65f, 0};
    cam.yaw   = me.yaw;
    cam.pitch = me.pitch;

    auto& sh = app.world_shader();
    sh.use();
    eng::Mat4 vp = cam.proj() * cam.view();

    GLint u_mvp   = sh.uniform("u_mvp");
    GLint u_m     = sh.uniform("u_m");
    GLint u_color = sh.uniform("u_color");
    GLint u_light = sh.uniform("u_light");
    glUniform3f(u_light, 0.3f, 1.0f, 0.4f);

    auto draw_box = [&](const eng::Aabb& b, float r, float g, float bl) {
        eng::Vec3 c = (b.min + b.max) * 0.5f;
        eng::Vec3 s = b.max - b.min;
        eng::Mat4 m = eng::Mat4::translation(c) * eng::Mat4::scale(s);
        eng::Mat4 mvp = vp * m;
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mptr(mvp));
        glUniformMatrix4fv(u_m,   1, GL_FALSE, mptr(m));
        glUniform3f(u_color, r, g, bl);
        app.unit_box().draw();
    };

    // Floor + walls.
    for (size_t i = 0; i < map_.walls().size(); ++i) {
        const auto& w = map_.walls()[i];
        bool floor = i == 0;
        if (floor) draw_box(w.box, 0.16f, 0.18f, 0.20f);
        else       draw_box(w.box, 0.40f, 0.36f, 0.30f);
    }

    // Bombsite tints.
    eng::Aabb a = map_.site_a(); a.max.y = 0.05f; a.min.y = 0.02f;
    eng::Aabb bsite = map_.site_b(); bsite.max.y = 0.05f; bsite.min.y = 0.02f;
    draw_box(a,     0.50f, 0.10f, 0.10f);
    draw_box(bsite, 0.10f, 0.20f, 0.50f);

    // Other players.
    for (size_t i = 1; i < players_.size(); ++i) {
        const auto& p = players_[i];
        if (!p.alive) continue;
        eng::Aabb body = p.aabb_at(p.pos);
        if (p.team == Team::CT) draw_box(body, 0.20f, 0.50f, 0.95f);
        else                    draw_box(body, 0.95f, 0.55f, 0.20f);
    }

    // Bomb.
    if (bomb_.planted && !bomb_.defused) {
        eng::Aabb b{ {bomb_.pos.x - 0.2f, bomb_.pos.y, bomb_.pos.z - 0.2f},
                     {bomb_.pos.x + 0.2f, bomb_.pos.y + 0.4f, bomb_.pos.z + 0.2f} };
        draw_box(b, 1.0f, 0.4f, 0.1f);
    }
}

} // namespace cs::game
