// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "engine/app.h"

#include <GLES3/gl3.h>
#include <chrono>

#include "engine/asset.h"
#include "game/game.h"
#include "hardening/antidebug.h"
#include "hardening/antitamper.h"
#include "util/log.h"

namespace eng {

namespace {

long mono_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace

App& App::instance() { static App s; return s; }

void App::on_create(const std::string& files_dir, const std::string& apk_path) {
    files_dir_ = files_dir;
    apk_path_  = apk_path;
    cs::hard::AntiDebug::instance().initialize();
    cs::hard::AntiTamper::instance().initialize(apk_path);
    game_ = std::make_unique<cs::game::Game>();
}

void App::init_gl_resources() {
    if (gl_ready_) return;
    bool ok = world_shader_.compile_from_assets("shaders/lit.vert", "shaders/lit.frag");
    if (!ok) {
        LOGE("world shader missing; using inline fallback");
        const char* vs =
            "#version 300 es\nlayout(location=0) in vec3 a_pos;\n"
            "layout(location=1) in vec3 a_nrm;\nuniform mat4 u_mvp;\nuniform mat4 u_m;\n"
            "out vec3 v_nrm; out vec3 v_world;\nvoid main(){ v_nrm=mat3(u_m)*a_nrm;"
            "v_world=(u_m*vec4(a_pos,1)).xyz; gl_Position=u_mvp*vec4(a_pos,1); }";
        const char* fs =
            "#version 300 es\nprecision highp float;in vec3 v_nrm;in vec3 v_world;"
            "out vec4 frag;uniform vec3 u_color;uniform vec3 u_light;\nvoid main(){"
            "float d=max(dot(normalize(v_nrm),normalize(u_light)),0.0);"
            "vec3 c=u_color*(0.25+0.75*d); frag=vec4(c,1); }";
        world_shader_.compile_from_source(vs, fs);
    }
    bool ok2 = hud_shader_.compile_from_assets("shaders/hud.vert", "shaders/hud.frag");
    if (!ok2) {
        const char* vs =
            "#version 300 es\nlayout(location=0) in vec3 a_pos;\nlayout(location=2) in vec2 a_uv;\n"
            "uniform mat4 u_proj; uniform vec4 u_rect;\nout vec2 v_uv;\n"
            "void main(){ vec2 p=u_rect.xy+a_uv*u_rect.zw; v_uv=a_uv;"
            "gl_Position=u_proj*vec4(p,0,1); }";
        const char* fs =
            "#version 300 es\nprecision mediump float;in vec2 v_uv;out vec4 frag;"
            "uniform vec4 u_color;\nvoid main(){ frag=u_color; }";
        hud_shader_.compile_from_source(vs, fs);
    }
    unit_box_  = Mesh::make_box({1, 1, 1});
    unit_quad_ = Mesh::make_quad();
    gl_ready_ = true;
}

void App::on_surface_created() {
    glClearColor(0.05f, 0.07f, 0.10f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    init_gl_resources();
    if (game_) game_->on_surface_ready();
}

void App::on_surface_changed(int w, int h) {
    width_ = w; height_ = h;
    glViewport(0, 0, w, h);
    camera_.aspect = static_cast<float>(w) / static_cast<float>(std::max(1, h));
    input_.set_screen_size(w, h);
    if (game_) game_->on_resize(w, h);
}

void App::on_draw_frame() {
    long now = mono_ns();
    float dt = last_ns_ ? (now - last_ns_) / 1e9f : 0.f;
    last_ns_ = now;
    if (dt > 0.1f) dt = 0.1f;

    // Touch events arrive asynchronously on the UI thread between frames.
    // end_frame() derives the joystick axis from the held-down pointer, then
    // game logic reads input, then begin_frame() resets per-frame fields
    // (look delta, fire_pressed, ui_tap) for the next accumulation window.
    input_.end_frame();
    if (game_) game_->update(dt);
    input_.begin_frame();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (game_) game_->render();
}

void App::on_resume()  { /* spin up audio etc. */ }
void App::on_pause()   { /* throttle / pause */ }
void App::on_destroy() {
    cs::hard::AntiDebug::instance().shutdown();
    cs::hard::AntiTamper::instance().shutdown();
    game_.reset();
}

} // namespace eng
