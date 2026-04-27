// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <GLES3/gl3.h>
#include <vector>

#include "engine/math.h"

namespace eng {

struct Vertex {
    Vec3 pos;
    Vec3 normal;
    float u, v;
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& o) noexcept : vao_(o.vao_), vbo_(o.vbo_), ibo_(o.ibo_),
                              index_count_(o.index_count_) {
        o.vao_ = o.vbo_ = o.ibo_ = 0; o.index_count_ = 0;
    }
    Mesh& operator=(Mesh&& o) noexcept {
        if (this != &o) {
            this->~Mesh();
            vao_ = o.vao_; vbo_ = o.vbo_; ibo_ = o.ibo_; index_count_ = o.index_count_;
            o.vao_ = o.vbo_ = o.ibo_ = 0; o.index_count_ = 0;
        }
        return *this;
    }

    void upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
    void draw() const;

    static Mesh make_box(const Vec3& size);
    static Mesh make_quad();

private:
    GLuint vao_{0}, vbo_{0}, ibo_{0};
    GLsizei index_count_{0};
};

} // namespace eng
