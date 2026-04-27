// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <cstdint>

namespace cs::game {

class Match;
struct Player;

// Compact AI state. Each bot owns one Brain; Match drives them per frame.
struct BotBrain {
    enum class Goal {
        HoldSite,    // CT default during freeze + early round
        Push,        // attackers move toward objective
        Plant,       // T carrying bomb -> nearest site
        Defuse,      // CT, bomb planted -> contest
        Engage,      // saw an enemy, prefer to shoot
        Reposition,  // got hit / lost LOS
    };

    Goal  goal{Goal::HoldSite};
    int   target_node{-1};
    int   target_player{-1};        // index into Match::players()
    float decision_cooldown{0.f};
    float fire_jitter{0.f};
    float reaction_time{0.20f};     // seconds before opening fire after spotting
    float skill{0.65f};             // 0..1, accuracy modifier
    uint32_t rng{0x9e3779b9u};
};

class Bot {
public:
    static void update(Match& m, int player_index, BotBrain& brain, float dt);
};

} // namespace cs::game
