// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

namespace eng { class App; }

namespace cs::game {

class Menu {
public:
    enum class Mode { Casual = 16, Competitive = 24, Practice = 4 };

    void update(eng::App& app);
    void render(eng::App& app);

    bool start_requested() const { return start_; }
    int  rounds() const { return static_cast<int>(mode_); }
    void consume() { start_ = false; }

private:
    Mode mode_{Mode::Competitive};
    bool start_{false};
    float blink_{0.f};
};

} // namespace cs::game
