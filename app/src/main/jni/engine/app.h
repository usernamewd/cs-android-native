// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <memory>
#include <string>

#include "engine/camera.h"
#include "engine/input.h"
#include "engine/mesh.h"
#include "engine/shader.h"

namespace cs::game { class Game; }

namespace eng {

class App {
public:
    static App& instance();

    void on_create(const std::string& files_dir, const std::string& apk_path);
    void on_surface_created();
    void on_surface_changed(int w, int h);
    void on_draw_frame();
    void on_resume();
    void on_pause();
    void on_destroy();

    TouchInput& input() { return input_; }
    Camera&     camera() { return camera_; }
    int width()  const { return width_; }
    int height() const { return height_; }

    Shader& world_shader() { return world_shader_; }
    Shader& hud_shader()   { return hud_shader_; }
    Mesh&   unit_box()     { return unit_box_; }
    Mesh&   unit_quad()    { return unit_quad_; }

private:
    App() = default;

    void init_gl_resources();

    std::unique_ptr<cs::game::Game> game_;
    Camera camera_;
    TouchInput input_;
    Shader world_shader_;
    Shader hud_shader_;
    Mesh   unit_box_;
    Mesh   unit_quad_;
    int width_{0}, height_{0};
    long  last_ns_{0};
    bool  gl_ready_{false};
    std::string files_dir_;
    std::string apk_path_;
};

} // namespace eng
