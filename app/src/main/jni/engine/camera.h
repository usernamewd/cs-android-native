// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include "engine/math.h"

namespace eng {

class Camera {
public:
    Vec3 position{0, 1.7f, 0};
    float yaw{0.f};   // around Y axis, radians
    float pitch{0.f}; // around X axis, radians
    float fov_y{deg2rad(75.f)};
    float aspect{1.f};
    float znear{0.05f};
    float zfar{300.f};

    Vec3 forward() const {
        float cp = std::cos(pitch), sp = std::sin(pitch);
        float cy = std::cos(yaw),   sy = std::sin(yaw);
        return { -sy * cp, sp, -cy * cp };
    }
    Vec3 right() const {
        float cy = std::cos(yaw), sy = std::sin(yaw);
        return { cy, 0.f, -sy };
    }

    Mat4 view() const {
        return Mat4::look_at(position, position + forward(), {0, 1, 0});
    }
    Mat4 proj() const {
        return Mat4::perspective(fov_y, aspect, znear, zfar);
    }
};

} // namespace eng
