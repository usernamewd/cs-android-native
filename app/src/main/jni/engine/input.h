// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include "engine/math.h"

#include <unordered_map>

namespace eng {

// Two-thumb FPS scheme:
//   * Left half of the screen is a virtual joystick (move).
//   * Right half is "swipe to look" + tap-to-fire / tap-to-defuse.
class TouchInput {
public:
    enum class Action { Down, Move, Up };

    void on_touch(Action a, int pointer_id, float x, float y);
    void set_screen_size(int w, int h);
    void begin_frame();
    void end_frame();

    // Per-frame outputs used by the player controller.
    Vec2  move_axis() const { return move_axis_; }   // [-1,1]^2
    Vec2  look_delta() const { return look_delta_; } // pixels this frame
    bool  fire_pressed() const { return fire_pressed_; }
    bool  fire_held()    const { return fire_held_; }
    bool  ui_tap()       const { return ui_tap_; }
    Vec2  ui_tap_pos()   const { return ui_tap_pos_; }

private:
    struct Pointer {
        Vec2 origin;
        Vec2 last;
        bool is_left{false};
        bool consumed_as_tap{false};
        long down_time_ms{0};
    };

    std::unordered_map<int, Pointer> pointers_;
    int   left_pid_{-1};
    int   right_pid_{-1};
    int   screen_w_{1}, screen_h_{1};
    Vec2  move_axis_{};
    Vec2  look_delta_{};
    bool  fire_pressed_{false};
    bool  fire_held_{false};
    bool  ui_tap_{false};
    Vec2  ui_tap_pos_{};
};

} // namespace eng
