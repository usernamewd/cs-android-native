// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <GLES3/gl3.h>
#include <string>

namespace eng {

class Shader {
public:
    Shader() = default;
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool compile_from_assets(const char* vs_path, const char* fs_path);
    bool compile_from_source(const char* vs, const char* fs);
    void use() const;
    GLint uniform(const char* name) const;
    GLuint program() const { return prog_; }

private:
    GLuint prog_{0};
};

} // namespace eng
