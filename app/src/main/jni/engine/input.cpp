// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "engine/input.h"

#include <chrono>

namespace eng {

namespace {

long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

constexpr float kJoyRadiusFrac = 0.18f;     // joystick max travel = 18% of min(w,h)
constexpr long  kTapMaxDurationMs = 220;
constexpr float kTapMaxTravelFrac = 0.02f;

} // namespace

void TouchInput::set_screen_size(int w, int h) {
    if (w > 0) screen_w_ = w;
    if (h > 0) screen_h_ = h;
}

void TouchInput::begin_frame() {
    look_delta_ = {0, 0};
    fire_pressed_ = false;
    ui_tap_ = false;
}

void TouchInput::on_touch(Action a, int pid, float x, float y) {
    const float half_w = screen_w_ * 0.5f;
    switch (a) {
        case Action::Down: {
            Pointer p;
            p.origin = {x, y};
            p.last = {x, y};
            p.is_left = x < half_w;
            p.down_time_ms = now_ms();
            pointers_[pid] = p;
            if (p.is_left && left_pid_ < 0) left_pid_ = pid;
            if (!p.is_left && right_pid_ < 0) {
                right_pid_ = pid;
                fire_held_ = true;
            }
            break;
        }
        case Action::Move: {
            auto it = pointers_.find(pid);
            if (it == pointers_.end()) return;
            if (pid == right_pid_) {
                look_delta_.x += x - it->second.last.x;
                look_delta_.y += y - it->second.last.y;
            }
            it->second.last = {x, y};
            break;
        }
        case Action::Up: {
            auto it = pointers_.find(pid);
            if (it == pointers_.end()) return;
            const Pointer& p = it->second;
            float dx = p.last.x - p.origin.x;
            float dy = p.last.y - p.origin.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            long dt = now_ms() - p.down_time_ms;
            float travel = dist / static_cast<float>(std::min(screen_w_, screen_h_));
            bool was_tap = dt < kTapMaxDurationMs && travel < kTapMaxTravelFrac;
            if (pid == right_pid_) {
                if (was_tap) {
                    fire_pressed_ = true;
                    ui_tap_ = true;
                    ui_tap_pos_ = p.origin;
                }
                fire_held_ = false;
                right_pid_ = -1;
            } else if (pid == left_pid_) {
                left_pid_ = -1;
            } else if (was_tap) {
                ui_tap_ = true;
                ui_tap_pos_ = p.origin;
            }
            pointers_.erase(it);
            break;
        }
    }
}

void TouchInput::end_frame() {
    move_axis_ = {0, 0};
    if (left_pid_ >= 0) {
        auto it = pointers_.find(left_pid_);
        if (it != pointers_.end()) {
            Vec2 d = it->second.last - it->second.origin;
            float r = static_cast<float>(std::min(screen_w_, screen_h_)) * kJoyRadiusFrac;
            float ax = clampf(d.x / r, -1.f, 1.f);
            float ay = clampf(d.y / r, -1.f, 1.f);
            move_axis_ = { ax, -ay };
        }
    }
}

} // namespace eng
