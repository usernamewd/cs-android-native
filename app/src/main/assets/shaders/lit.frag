#version 300 es
precision highp float;

in vec3 v_nrm;
in vec3 v_world;

out vec4 frag;

uniform vec3 u_color;
uniform vec3 u_light;

void main() {
    vec3 n = normalize(v_nrm);
    vec3 l = normalize(u_light);
    float d = max(dot(n, l), 0.0);
    vec3 c = u_color * (0.25 + 0.75 * d);
    frag = vec4(c, 1.0);
}
