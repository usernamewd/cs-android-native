// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "engine/mesh.h"

namespace eng {

Mesh::~Mesh() {
    if (ibo_) glDeleteBuffers(1, &ibo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}

void Mesh::upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices) {
    if (!vao_) glGenVertexArrays(1, &vao_);
    if (!vbo_) glGenBuffers(1, &vbo_);
    if (!ibo_) glGenBuffers(1, &ibo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                 indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, u)));
    glBindVertexArray(0);
    index_count_ = static_cast<GLsizei>(indices.size());
}

void Mesh::draw() const {
    if (!vao_) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Mesh Mesh::make_box(const Vec3& s) {
    const float hx = s.x * 0.5f, hy = s.y * 0.5f, hz = s.z * 0.5f;
    const Vec3 n[6] = {
        {0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}
    };
    const Vec3 corners[8] = {
        {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz},
        {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, {-hx,  hy, -hz}
    };
    const int faces[6][4] = {
        {0,1,2,3}, {5,4,7,6}, {1,5,6,2}, {4,0,3,7}, {3,2,6,7}, {4,5,1,0}
    };
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    v.reserve(24); idx.reserve(36);
    for (int f = 0; f < 6; ++f) {
        uint32_t base = static_cast<uint32_t>(v.size());
        v.push_back({corners[faces[f][0]], n[f], 0.f, 0.f});
        v.push_back({corners[faces[f][1]], n[f], 1.f, 0.f});
        v.push_back({corners[faces[f][2]], n[f], 1.f, 1.f});
        v.push_back({corners[faces[f][3]], n[f], 0.f, 1.f});
        idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    Mesh m;
    m.upload(v, idx);
    return m;
}

Mesh Mesh::make_quad() {
    std::vector<Vertex> v = {
        {{-1,-1,0}, {0,0,1}, 0,0},
        {{ 1,-1,0}, {0,0,1}, 1,0},
        {{ 1, 1,0}, {0,0,1}, 1,1},
        {{-1, 1,0}, {0,0,1}, 0,1},
    };
    std::vector<uint32_t> i = {0,1,2, 0,2,3};
    Mesh m;
    m.upload(v, i);
    return m;
}

} // namespace eng
