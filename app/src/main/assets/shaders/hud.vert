#version 300 es

layout(location = 0) in vec3 a_pos;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_proj;
uniform vec4 u_rect;

out vec2 v_uv;

void main() {
    vec2 p = u_rect.xy + a_uv * u_rect.zw;
    v_uv = a_uv;
    gl_Position = u_proj * vec4(p, 0.0, 1.0);
}
