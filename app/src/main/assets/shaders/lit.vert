#version 300 es

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_nrm;

uniform mat4 u_mvp;
uniform mat4 u_m;

out vec3 v_nrm;
out vec3 v_world;

void main() {
    v_nrm   = mat3(u_m) * a_nrm;
    v_world = (u_m * vec4(a_pos, 1.0)).xyz;
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}
