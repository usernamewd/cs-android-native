#version 300 es
precision mediump float;

in vec2 v_uv;
out vec4 frag;

uniform vec4 u_color;

void main() {
    frag = u_color;
}
