// Copyright (c) 2026 cs-android-native contributors. MIT License.
//
// Tiny self-contained math library. We deliberately avoid GLM / Eigen so the
// final binary has no third-party symbol bloat the linker can't strip, and the
// obfuscator gets a clean shot at every operation.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace eng {

constexpr float kPi = 3.14159265358979323846f;

inline float deg2rad(float d) { return d * (kPi / 180.f); }
inline float rad2deg(float r) { return r * (180.f / kPi); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct Vec2 {
    float x{0}, y{0};
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    float length() const { return std::sqrt(x * x + y * y); }
};

struct Vec3 {
    float x{0}, y{0}, z{0};
    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float length_sq() const { return x * x + y * y + z * z; }
    Vec3 normalized() const {
        float l = length();
        if (l < 1e-6f) return {0, 0, 0};
        return {x / l, y / l, z / l};
    }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}

struct Vec4 {
    float x{0}, y{0}, z{0}, w{0};
};

// Column-major 4x4 matrix, OpenGL-friendly.
struct Mat4 {
    float m[16];

    static Mat4 identity() {
        Mat4 r{}; r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.f; return r;
    }

    static Mat4 translation(const Vec3& t) {
        Mat4 r = identity();
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }

    static Mat4 scale(const Vec3& s) {
        Mat4 r{};
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.f;
        return r;
    }

    // OpenGL-style perspective; right-handed, depth in [-1,1].
    static Mat4 perspective(float fov_y_rad, float aspect, float znear, float zfar) {
        Mat4 r{};
        const float f = 1.f / std::tan(fov_y_rad * 0.5f);
        r.m[0]  = f / aspect;
        r.m[5]  = f;
        r.m[10] = (zfar + znear) / (znear - zfar);
        r.m[11] = -1.f;
        r.m[14] = (2.f * zfar * znear) / (znear - zfar);
        return r;
    }

    static Mat4 look_at(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = cross(f, up).normalized();
        Vec3 u = cross(s, f);
        Mat4 r = identity();
        r.m[0]=s.x;  r.m[4]=s.y;  r.m[8] =s.z;
        r.m[1]=u.x;  r.m[5]=u.y;  r.m[9] =u.z;
        r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
        r.m[12] = -dot(s, eye);
        r.m[13] = -dot(u, eye);
        r.m[14] =  dot(f, eye);
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r{};
        for (int c = 0; c < 4; ++c) {
            for (int rr = 0; rr < 4; ++rr) {
                float sum = 0.f;
                for (int k = 0; k < 4; ++k) sum += m[k * 4 + rr] * b.m[c * 4 + k];
                r.m[c * 4 + rr] = sum;
            }
        }
        return r;
    }
};

// AABB in world space, used for trivial BSP-free collision.
struct Aabb {
    Vec3 min, max;
    bool contains(const Vec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }
};

// Slab ray vs AABB. Returns true and sets t if hit in [0, t_max].
inline bool ray_aabb(const Vec3& ro, const Vec3& rd, const Aabb& b, float t_max, float& t_out) {
    float tmin = 0.f, tmax = t_max;
    for (int i = 0; i < 3; ++i) {
        float o = (&ro.x)[i], d = (&rd.x)[i];
        float lo = (&b.min.x)[i], hi = (&b.max.x)[i];
        if (std::fabs(d) < 1e-6f) {
            if (o < lo || o > hi) return false;
        } else {
            float t1 = (lo - o) / d;
            float t2 = (hi - o) / d;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }
    t_out = tmin;
    return true;
}

} // namespace eng
