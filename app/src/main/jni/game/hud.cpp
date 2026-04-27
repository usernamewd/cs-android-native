// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "game/hud.h"

#include <GLES3/gl3.h>

#include "engine/app.h"
#include "engine/mesh.h"
#include "engine/shader.h"
#include "game/match.h"

namespace cs::game {

namespace {

void draw_rect(eng::App& app, float x, float y, float w, float h,
               float r, float g, float b, float a) {
    auto& sh = app.hud_shader();
    sh.use();
    GLint u_proj  = sh.uniform("u_proj");
    GLint u_rect  = sh.uniform("u_rect");
    GLint u_color = sh.uniform("u_color");

    // Orthographic 0..W, 0..H, top-left origin.
    float W = static_cast<float>(app.width());
    float H = static_cast<float>(app.height());
    eng::Mat4 P{};
    P.m[0]  =  2.f / W;
    P.m[5]  = -2.f / H;
    P.m[10] = -1.f;
    P.m[12] = -1.f;
    P.m[13] =  1.f;
    P.m[15] =  1.f;
    glUniformMatrix4fv(u_proj, 1, GL_FALSE, P.m);
    glUniform4f(u_rect, x, y, w, h);
    glUniform4f(u_color, r, g, b, a);
    app.unit_quad().draw();
}

} // namespace

void Hud::render(eng::App& app, const Match& m) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float W = static_cast<float>(app.width());
    const float H = static_cast<float>(app.height());

    // Top scoreboard strip.
    draw_rect(app, 0, 0, W, H * 0.06f, 0.05f, 0.06f, 0.08f, 0.85f);

    // Phase tint bar.
    float pf = m.phase_remaining();
    if (m.phase() == Phase::Freeze) {
        draw_rect(app, 0, H * 0.06f, W, 4, 0.10f, 0.65f, 1.0f, 0.9f);
    } else if (m.phase() == Phase::Live) {
        draw_rect(app, 0, H * 0.06f, W, 4, 1.0f, 0.85f, 0.30f, 0.9f);
    } else if (m.phase() == Phase::PostRound) {
        draw_rect(app, 0, H * 0.06f, W, 4, 0.95f, 0.20f, 0.20f, 0.9f);
    }
    (void)pf;

    // Score blocks.
    draw_rect(app, W * 0.40f, 0, W * 0.10f, H * 0.06f, 0.20f, 0.50f, 0.95f, 0.95f);
    draw_rect(app, W * 0.50f, 0, W * 0.10f, H * 0.06f, 0.95f, 0.55f, 0.20f, 0.95f);

    // Health pill (lower-left).
    const auto& me = m.local();
    float hp = static_cast<float>(me.health) / 100.f;
    if (hp < 0.f) hp = 0.f; if (hp > 1.f) hp = 1.f;
    draw_rect(app, W * 0.04f, H * 0.86f, W * 0.20f, H * 0.04f, 0.10f, 0.10f, 0.10f, 0.7f);
    draw_rect(app, W * 0.04f, H * 0.86f, W * 0.20f * hp, H * 0.04f,
              0.20f, 0.85f, 0.30f, 0.95f);

    // Ammo pill (lower-right).
    float fa = static_cast<float>(me.weapon.ammo) / static_cast<float>(me.weapon.mag_max);
    if (fa < 0.f) fa = 0.f; if (fa > 1.f) fa = 1.f;
    draw_rect(app, W * 0.76f, H * 0.86f, W * 0.20f, H * 0.04f, 0.10f, 0.10f, 0.10f, 0.7f);
    draw_rect(app, W * 0.76f, H * 0.86f, W * 0.20f * fa, H * 0.04f,
              0.95f, 0.80f, 0.20f, 0.95f);

    // Crosshair (center).
    draw_rect(app, W * 0.5f - 1, H * 0.5f - 6, 2,  12, 1, 1, 1, 0.9f);
    draw_rect(app, W * 0.5f - 6, H * 0.5f - 1, 12,  2, 1, 1, 1, 0.9f);

    // Virtual joystick base (left).
    float jx = W * 0.18f, jy = H * 0.74f, jr = H * 0.10f;
    draw_rect(app, jx - jr, jy - jr, jr * 2, jr * 2, 1, 1, 1, 0.08f);
    draw_rect(app, jx - jr * 0.4f, jy - jr * 0.4f, jr * 0.8f, jr * 0.8f, 1, 1, 1, 0.18f);

    // Fire button hint (right).
    draw_rect(app, W * 0.86f, H * 0.70f, H * 0.08f, H * 0.08f, 1, 0.4f, 0.2f, 0.4f);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

} // namespace cs::game
