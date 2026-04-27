// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "engine/shader.h"
#include "engine/asset.h"
#include "util/log.h"

namespace eng {

namespace {

GLuint compile(GLenum kind, const char* src) {
    GLuint s = glCreateShader(kind);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; GLsizei n = 0;
        glGetShaderInfoLog(s, sizeof(log), &n, log);
        LOGE("shader compile fail: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

} // namespace

Shader::~Shader() { if (prog_) glDeleteProgram(prog_); }

bool Shader::compile_from_source(const char* vs, const char* fs) {
    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) { if (v) glDeleteShader(v); if (f) glDeleteShader(f); return false; }
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; GLsizei n = 0;
        glGetProgramInfoLog(p, sizeof(log), &n, log);
        LOGE("link fail: %s", log);
        glDeleteProgram(p);
        return false;
    }
    if (prog_) glDeleteProgram(prog_);
    prog_ = p;
    return true;
}

bool Shader::compile_from_assets(const char* vs_path, const char* fs_path) {
    std::string v, f;
    if (!AssetIO::instance().load_text(vs_path, v)) return false;
    if (!AssetIO::instance().load_text(fs_path, f)) return false;
    return compile_from_source(v.c_str(), f.c_str());
}

void Shader::use() const { if (prog_) glUseProgram(prog_); }
GLint Shader::uniform(const char* n) const { return prog_ ? glGetUniformLocation(prog_, n) : -1; }

} // namespace eng
