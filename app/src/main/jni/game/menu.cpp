// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "game/menu.h"

#include <GLES3/gl3.h>

#include "engine/app.h"
#include "engine/input.h"
#include "engine/mesh.h"
#include "engine/shader.h"

namespace cs::game {

namespace {

struct Rect { float x, y, w, h; };

bool hit(const Rect& r, eng::Vec2 p) {
    return p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
}

void draw(eng::App& app, const Rect& r, float cr, float cg, float cb, float ca) {
    auto& sh = app.hud_shader();
    sh.use();
    GLint u_proj  = sh.uniform("u_proj");
    GLint u_rect  = sh.uniform("u_rect");
    GLint u_color = sh.uniform("u_color");
    float W = static_cast<float>(app.width()), H = static_cast<float>(app.height());
    eng::Mat4 P{};
    P.m[0] = 2.f/W; P.m[5] = -2.f/H; P.m[10] = -1; P.m[12] = -1; P.m[13] = 1; P.m[15] = 1;
    glUniformMatrix4fv(u_proj, 1, GL_FALSE, P.m);
    glUniform4f(u_rect, r.x, r.y, r.w, r.h);
    glUniform4f(u_color, cr, cg, cb, ca);
    app.unit_quad().draw();
}

} // namespace

void Menu::update(eng::App& app) {
    auto& in = app.input();
    if (!in.ui_tap()) return;

    float W = static_cast<float>(app.width());
    float H = static_cast<float>(app.height());
    eng::Vec2 t = in.ui_tap_pos();

    Rect competitive { W * 0.30f, H * 0.30f, W * 0.40f, H * 0.10f };
    Rect casual      { W * 0.30f, H * 0.42f, W * 0.40f, H * 0.10f };
    Rect practice    { W * 0.30f, H * 0.54f, W * 0.40f, H * 0.10f };
    Rect start_btn   { W * 0.35f, H * 0.74f, W * 0.30f, H * 0.12f };

    if (hit(competitive, t)) mode_ = Mode::Competitive;
    else if (hit(casual,   t)) mode_ = Mode::Casual;
    else if (hit(practice, t)) mode_ = Mode::Practice;
    else if (hit(start_btn, t)) start_ = true;
}

void Menu::render(eng::App& app) {
    glDisable(GL_DEPTH_TEST);
    // The HUD ortho projection flips Y (m[5] = -2/H), which inverts the unit
    // quad's effective winding from CCW to CW; with GL_CULL_FACE on, every
    // HUD quad would be back-face culled and the menu would be invisible.
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float W = static_cast<float>(app.width());
    float H = static_cast<float>(app.height());

    // Background panel.
    draw(app, {0, 0, W, H}, 0.04f, 0.05f, 0.07f, 1.0f);
    // Title bar.
    draw(app, {0, H * 0.10f, W, H * 0.08f}, 0.95f, 0.75f, 0.20f, 0.95f);

    auto button = [&](const Rect& r, bool selected, float fr, float fg, float fb) {
        draw(app, r, fr, fg, fb, selected ? 0.95f : 0.55f);
        // Inner highlight when selected.
        if (selected) {
            draw(app, {r.x, r.y, r.w, 3}, 1, 1, 1, 0.7f);
            draw(app, {r.x, r.y + r.h - 3, r.w, 3}, 1, 1, 1, 0.7f);
        }
    };

    button({W*0.30f, H*0.30f, W*0.40f, H*0.10f}, mode_ == Mode::Competitive, 0.20f, 0.50f, 0.95f);
    button({W*0.30f, H*0.42f, W*0.40f, H*0.10f}, mode_ == Mode::Casual,      0.50f, 0.95f, 0.40f);
    button({W*0.30f, H*0.54f, W*0.40f, H*0.10f}, mode_ == Mode::Practice,    0.95f, 0.50f, 0.30f);

    // START button.
    blink_ += 0.05f;
    float pulse = 0.7f + 0.3f * std::sin(blink_);
    draw(app, {W*0.35f, H*0.74f, W*0.30f, H*0.12f}, 0.95f * pulse, 0.20f, 0.20f, 0.95f);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

} // namespace cs::game
