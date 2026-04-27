// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <vector>

#include "game/bomb.h"
#include "game/bot.h"
#include "game/map.h"
#include "game/player.h"

namespace eng { class App; class TouchInput; }

namespace cs::game {

enum class Phase {
    Freeze,    // 5s, can't shoot/move freely; pick up free pistol
    Live,      // 115s, normal play
    PostRound, // 5s end-of-round screen, then next round
    Halftime,  // teams swap, scores stay
    MatchOver
};

class Match {
public:
    static constexpr float kFreezeDuration   =  5.0f;
    static constexpr float kLiveDuration     = 115.0f;
    static constexpr float kPostDuration     =  5.0f;
    static constexpr int   kBotsPerSide      =  4;

    void start(int max_rounds);
    void update(float dt, const eng::TouchInput& input);
    void render(eng::App& app);

    // --- Game-rule API used by Bot ----------------------------------------
    void try_move(int player_index, const eng::Vec3& step);
    void try_fire(int player_index);
    bool try_plant(int player_index);
    bool local_player_in_site_with_bomb(int player_index) const;

    // --- Read-only access -------------------------------------------------
    Phase phase() const { return phase_; }
    int   round() const { return round_; }
    int   ct_score() const { return ct_score_; }
    int   t_score()  const { return t_score_; }
    int   max_rounds() const { return max_rounds_; }
    float phase_remaining() const { return phase_timer_; }

    const Player& local() const { return players_[0]; }
    Player&       local()       { return players_[0]; }
    const std::vector<Player>& players() const { return players_; }
    std::vector<Player>&       players()       { return players_; }
    const Map&  map()  const { return map_; }
    const Bomb& bomb() const { return bomb_; }
    Bomb&       bomb()       { return bomb_; }

private:
    void begin_freeze();
    void begin_live();
    void begin_post_round(Team winner, const char* reason);
    void enter_halftime();
    void respawn_all();
    void simulate_local(float dt, const eng::TouchInput& input);
    void apply_damage(int target, int dmg, int attacker);
    void check_round_end();
    bool point_in(const eng::Aabb& b, const eng::Vec3& p) const {
        return b.contains(p);
    }

    Map  map_;
    Bomb bomb_;
    std::vector<Player>   players_;
    std::vector<BotBrain> brains_;     // parallel to players_, brain.empty for index 0
    Phase phase_{Phase::Freeze};
    float phase_timer_{kFreezeDuration};
    int   round_{0};
    int   max_rounds_{24};
    int   ct_score_{0}, t_score_{0};
    bool  half_swapped_{false};
    Team  last_winner_{Team::CT};
    const char* last_reason_{""};
};

} // namespace cs::game
