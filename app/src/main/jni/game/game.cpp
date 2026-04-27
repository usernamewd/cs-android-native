// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "game/game.h"

#include "engine/app.h"

namespace cs::game {

Game::Game() = default;

void Game::on_surface_ready() {}

void Game::on_resize(int /*w*/, int /*h*/) {}

void Game::update(float dt) {
    auto& app = eng::App::instance();
    if (screen_ == Screen::Menu) {
        menu_.update(app);
        if (menu_.start_requested()) {
            menu_.consume();
            match_ = std::make_unique<Match>();
            match_->start(menu_.rounds());
            screen_ = Screen::InMatch;
        }
    } else if (match_) {
        match_->update(dt, app.input());
        if (match_->phase() == Phase::MatchOver &&
            match_->phase_remaining() < 1e8f) {
            screen_ = Screen::Menu;
            match_.reset();
        }
    }
}

void Game::render() {
    auto& app = eng::App::instance();
    if (screen_ == Screen::Menu) {
        menu_.render(app);
    } else if (match_) {
        match_->render(app);
        hud_.render(app, *match_);
    }
}

} // namespace cs::game
